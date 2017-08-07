#ifndef GARBLED_CIRCUITS_HPP_
#define GARBLED_CIRCUITS_HPP_

#include<cstdlib>
#include<vector>
#include"cryptography.hpp"
#include"arithmetic_circuits.hpp"

#define THREE_TYPENAMES_ \
    typename TCAllocGate, \
    typename TCAllocHandle, \
    typename TEAllocGate, \
    typename TEAllocKP, \
    typename TEAllocKPVec, \
    typename TEAllocHandle, \
    typename TDAllocGate, \
    typename TDAllocHandle, \
    typename TDAllocHandleVec

#define THREE_TYPENAME_ARGS_ \
    TCAllocGate, \
    TCAllocHandle, \
    TEAllocGate, \
    TEAllocKP, \
    TEAllocKPVec, \
    TEAllocHandle, \
    TDAllocGate, \
    TDAllocHandle, \
    TDAllocHandleVec

#define TWO_PARTY_CIRCUIT_TYPE_ \
    TwoPartyCircuit<TCAllocGate, TCAllocHandle>
#define ENCODING_CIRCUIT_TYPE_ \
    EncodingCircuit<TEAllocGate, TEAllocKP, TEAllocKPVec, TEAllocHandle>
#define DECODING_CIRCUIT_TYPE_ \
    DecodingCircuit<TDAllocGate, TDAllocHandle, TDAllocHandleVec>

namespace Cryptography
{
namespace ArithmeticCircuits
{
namespace Garbled
{
	struct KeyPair
	{
		GateHandle Coefficient, Intercept;
	};

	template
	<
		typename TEAllocGate = std::allocator<Gate>,
		typename TEAllocKP = std::allocator<KeyPair>,
		typename TEAllocKPVec = std::allocator<std::vector<KeyPair, TEAllocKP>>,
		typename TEAllocHandle = std::allocator<GateHandle>
	>
	struct EncodingCircuit : CircuitCRTP<ENCODING_CIRCUIT_TYPE_>
	{
		typedef std::vector<Gate, TEAllocGate> GateVec;
		typedef std::vector<KeyPair, TEAllocKP> KPVec;
		typedef std::vector<KPVec, TEAllocKPVec> KPVec2;
		typedef std::vector<GateHandle, TEAllocHandle> HandleVec;

		GateVec Gates;
		HandleVec Randomness;
		HandleVec OfflineEncoding;
		KPVec2 AliceEncoding;
		KPVec2 BobEncoding;

		GateHandle InsertRandomGate()
		{
			auto const randomHandle
				= this->InsertGate(InputGateData{ AgentFlag::Random,
					Randomness.size(), 0u });
			Randomness.push_back(randomHandle);
			return randomHandle;
		}
	};

	template
	<
		typename TDAllocGate = std::allocator<Gate>,
		typename TDAllocHandle = std::allocator<GateHandle>,
		typename TDAllocHandleVec = std::allocator<std::vector<GateHandle, TDAllocHandle>>
	>
	struct DecodingCircuit : CircuitCRTP<DECODING_CIRCUIT_TYPE_>
	{
		typedef std::vector<Gate, TDAllocGate> GateVec;
		typedef std::vector<GateHandle, TDAllocHandle> HandleVec;
		typedef std::vector<HandleVec, TDAllocHandleVec> HandleVec2;

		GateVec Gates;
		HandleVec OfflineEncoding;
		HandleVec2 AliceEncoding;
		HandleVec2 BobEncoding;
		HandleVec AliceOutput;
	};

	namespace _CompilerImpl
	{
		template <THREE_TYPENAMES_>
		struct CircuitGarbler
			: GateVisitorCRTP<CircuitGarbler<THREE_TYPENAME_ARGS_>,
				GateHandle(GateHandle, GateHandle)>
		{
			typedef TWO_PARTY_CIRCUIT_TYPE_ TwoPartyCircuitType;
			typedef ENCODING_CIRCUIT_TYPE_ EncodingCircuitType;
			typedef DECODING_CIRCUIT_TYPE_ DecodingCircuitType;

			GateHandle zeroGate, oneGate;
			TwoPartyCircuitType &circuit;
			EncodingCircuitType &encoder;
			DecodingCircuitType &decoder;

			CircuitGarbler(
				TwoPartyCircuitType &tpc,
				EncodingCircuitType &enc,
				DecodingCircuitType &dec
			) :
				zeroGate(::Cryptography::ArithmeticCircuits::InvalidGateHandle),
				oneGate(::Cryptography::ArithmeticCircuits::InvalidGateHandle),
				circuit(tpc), encoder(enc), decoder(dec)
			{
				encoder.Gates.clear();
				encoder.Randomness.clear();
				encoder.OfflineEncoding.clear();
				encoder.AliceEncoding.clear();
				encoder.BobEncoding.clear();
				decoder.Gates.clear();
				decoder.OfflineEncoding.clear();
				decoder.AliceEncoding.clear();
				decoder.BobEncoding.clear();
				decoder.AliceOutput.clear();
				auto const alice = circuit.AliceInputEnd - circuit.AliceInputBegin;
				auto const bob = circuit.BobInputEnd - circuit.BobInputBegin;
				encoder.AliceEncoding.resize(alice);
				encoder.BobEncoding.resize(bob);
				decoder.AliceEncoding.resize(alice);
				decoder.BobEncoding.resize(bob);
				decoder.AliceOutput.reserve(circuit.AliceOutput.size());
				/* Basic gates used as the starting
				 * point of encoding process.
				 */
				zeroGate = encoder.InsertGate(ConstZeroData());
				oneGate = encoder.InsertGate(ConstOneData());
				for (auto const ao : circuit.AliceOutput)
					decoder.AliceOutput.push_back(this->VisitDispatcher(
						&circuit.Gates[ao], oneGate, zeroGate));
			}

		private:
			friend class GateVisitorCRTP
				<CircuitGarbler<THREE_TYPENAME_ARGS_>,
					GateHandle(GateHandle, GateHandle)>;

			/** Convention: for v = VisitBlah(g, k, b)
			***     - g is a Gate in circuit
			***     - k is a GateHandle in encoder
			***     - b is a GateHandle in encoder
			***     - v is a GateHandle in decoder,
			***       which should produce kg + b
			***/
			GateHandle VisitUnmatched(Gate *, GateHandle, GateHandle)
			{
				std::exit(-99);
				return ::Cryptography::ArithmeticCircuits::InvalidGateHandle;
			}

			/* k0 + b = b */
			GateHandle VisitConstZero(Gate *, GateHandle, GateHandle b)
			{
				encoder.OfflineEncoding.push_back(b);
				auto const decodingHandle
					= decoder.InsertGate(
						InputGateData{ AgentFlag::None, decoder.OfflineEncoding.size(), 0u });
				decoder.OfflineEncoding.push_back(decodingHandle);
				return decodingHandle;
			}

			/* k1 + b = k + b */
			GateHandle VisitConstOne(Gate *, GateHandle k, GateHandle b)
			{
				auto const encodingHandle
					= encoder.InsertGate(AdditionGateData{ k, b });
				encoder.OfflineEncoding.push_back(encodingHandle);
				auto const decodingHandle
					= decoder.InsertGate(
						InputGateData{ AgentFlag::None, decoder.OfflineEncoding.size(), 0u });
				decoder.OfflineEncoding.push_back(decodingHandle);
				return decodingHandle;
			}

			/* k(-1) + b = b - k */
			GateHandle VisitConstMinusOne(Gate *, GateHandle k, GateHandle b)
			{
				auto const encodingHandle
					= encoder.InsertGate(SubtractionGateData{ b, k });
				encoder.OfflineEncoding.push_back(encodingHandle);
				auto const decodingHandle
					= decoder.InsertGate(
						InputGateData{ AgentFlag::None, decoder.OfflineEncoding.size(), 0u });
				decoder.OfflineEncoding.push_back(decodingHandle);
				return decodingHandle;
			}

			/* Stores k, b as encoding key pair and creates
			 * the corresponding decoding input gate.
			 */
			GateHandle VisitInputGate(Gate *that, GateHandle k, GateHandle b)
			{
				auto const &g = that->AsInputGate;
				typename EncodingCircuitType::KPVec *encodingTarget;
				typename DecodingCircuitType::HandleVec *decodingTarget;
				auto const index = g.MajorIndex;
				auto const owner = g.Agent;
				if (owner == AgentFlag::Alice)
				{
					encodingTarget = &encoder.AliceEncoding[index];
					decodingTarget = &decoder.AliceEncoding[index];
				}
				else if (owner == AgentFlag::Bob)
				{
					encodingTarget = &encoder.BobEncoding[index];
					decodingTarget = &decoder.BobEncoding[index];
				}
				else
					exit(-99);
				encodingTarget->push_back({ k, b });
				auto const decodingHandle
					= decoder.InsertGate(
						InputGateData{ owner, index, decodingTarget->size() });
				decodingTarget->push_back(decodingHandle);
				return decodingHandle;
			}

			/* k(g1 + g2) + b = (kg1 + r) + (kg2 + (b - r)) */
			GateHandle VisitAdditionGate(Gate *that, GateHandle k, GateHandle b)
			{
				auto const &g = that->AsAdditionGate;
				auto const r = encoder.InsertRandomGate();
				auto const bMinusR = encoder.InsertGate(SubtractionGateData{ b, r });
				auto const g1 = this->VisitDispatcher(&circuit.Gates[g.Augend], k, r);
				auto const g2 = this->VisitDispatcher(&circuit.Gates[g.Addend], k, bMinusR);
				return decoder.InsertGate(AdditionGateData{ g1, g2 });
			}

			/* k(-g) + b = (-k)g + b */
			GateHandle VisitNegationGate(Gate *that, GateHandle k, GateHandle b)
			{
				auto const &g = that->AsNegationGate;
				auto const minusK = encoder.InsertGate(NegationGateData{ k });
				return this->VisitDispatcher(&circuit.Gates[g.Target], minusK, b);
			}

			/* k(g1 - g2) + b = (kg1 + (b - r)) + (kg2 + r) */
			GateHandle VisitSubtractionGate(Gate *that, GateHandle k, GateHandle b)
			{
				auto const &g = that->AsSubtractionGate;
				auto const r = encoder.InsertRandomGate();
				auto const bPlusR
					= encoder.InsertGate(AdditionGateData{ b, r });
				auto const g1 = this->VisitDispatcher(&circuit.Gates[g.Minuend], k, bPlusR);
				auto const g2 = this->VisitDispatcher(&circuit.Gates[g.Subtrahend], k, r);
				return decoder.InsertGate(SubtractionGateData{ g1, g2 });
			}

			/** To compile k(g1g2)+b:
			***     x1 = kg1-r1
			***     x2 = g2-r2
			***     x3 = kr2g1+r3
			***     x4 = r1g2+b-(r1r2+r3)
			***/
			GateHandle VisitMultiplicationGate(Gate *that, GateHandle k, GateHandle b)
			{
				auto const &g = that->AsMultiplicationGate;
				auto const r1 = encoder.InsertRandomGate();
				auto const r2 = encoder.InsertRandomGate();
				auto const r3 = encoder.InsertRandomGate();
				auto const minusR1 = encoder.InsertGate(NegationGateData{ r1 });
				auto const minusR2 = encoder.InsertGate(NegationGateData{ r2 });
				auto const kr2 = encoder.InsertGate(MultiplicationGateData{ k, r2 });
				auto const r1r2 = encoder.InsertGate(MultiplicationGateData{ r1, r2 });
				auto const r1r2r3 = encoder.InsertGate(AdditionGateData{ r1r2, r3 });
				auto const br1r2r3 = encoder.InsertGate(SubtractionGateData{ b, r1r2r3 });
				auto const g1 = &circuit.Gates[g.Multiplier];
				auto const g2 = &circuit.Gates[g.Multiplicand];
				auto const x1 = this->VisitDispatcher(g1, k, minusR1);
				auto const x2 = this->VisitDispatcher(g2, oneGate, minusR2);
				auto const x3 = this->VisitDispatcher(g1, kr2, r3);
				auto const x4 = this->VisitDispatcher(g2, r1, br1r2r3);
				auto const x1x2 = decoder.InsertGate(MultiplicationGateData{ x1, x2 });
				auto const x3x4 = decoder.InsertGate(AdditionGateData{ x3, x4 });
				return decoder.InsertGate(AdditionGateData{ x1x2, x3x4 });
			}
		};
	}

	template <THREE_TYPENAMES_>
	void CompileToDare
	(
		TWO_PARTY_CIRCUIT_TYPE_ &circuit,
		ENCODING_CIRCUIT_TYPE_ &encoder,
		DECODING_CIRCUIT_TYPE_ &decoder
	)
	{
		_CompilerImpl::CircuitGarbler<THREE_TYPENAME_ARGS_>
			(circuit, encoder, decoder);
	}

}
}
}

#undef THREE_TYPENAMES_
#undef THREE_TYPENAME_ARGS_
#undef TWO_PARTY_CIRCUIT_TYPE_
#undef ENCODING_CIRCUIT_TYPE_
#undef DECODING_CIRCUIT_TYPE_

#endif // GARBLED_CIRCUITS_HPP_
