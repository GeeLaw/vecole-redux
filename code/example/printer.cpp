#define _CRT_SECURE_NO_WARNINGS

#include"../library/cryptography.hpp"
#include"../library/arithmetic_circuits.hpp"
#include"../library/garbled_circuits.hpp"

#include<cstdio>

using namespace Cryptography;
using namespace Cryptography::ArithmeticCircuits;
using namespace Cryptography::ArithmeticCircuits::Garbled;

typedef unsigned Precedence;
constexpr Precedence HighestPrecedence = 0x0;
constexpr Precedence VariablePrecedence = 0x10;
constexpr Precedence NegationPrecedence = 0x20;
constexpr Precedence MultiplicationPrecedence = 0x30;
constexpr Precedence SubtractionPrecedence = 0x40;
constexpr Precedence AdditionPrecedence = 0x50;
constexpr Precedence LowestPrecedence = 0u - 1u;

template <typename TContainer>
struct ExpressionPrinter
	: GateVisitorCRTP<ExpressionPrinter<TContainer>, void(Precedence)>
{
	ExpressionPrinter(TContainer &cont)
		: container(cont)
	{ }
	void Print(GateHandle handle)
	{
		this->VisitDispatcher(&container[handle], LowestPrecedence);
	}
private:
	TContainer &container;
	friend class GateVisitorCRTP<ExpressionPrinter<TContainer>, void(Precedence)>;
	void VisitUnmatched(Gate *, Precedence)
	{
		fputs("\nFATAL ERROR: Unknown Gate.\n", stderr);
	}
	void VisitConstZero(Gate *, Precedence)
	{
		putchar('0');
	}
	void VisitConstOne(Gate *, Precedence)
	{
		putchar('1');
	}
	void VisitConstMinusOne(Gate *, Precedence outer)
	{
		if (outer != LowestPrecedence)
			putchar('(');
		putchar('-');
		putchar('1');
		if (outer != LowestPrecedence)
			putchar(')');
	}
	void VisitInputGate(Gate *that, Precedence)
	{
		auto &g = that->AsInputGate;
		char const *owner = "I";
		if (g.Agent == AgentFlag::None)
			owner = "X";
		if (g.Agent == AgentFlag::Alice)
			owner = "A";
		if (g.Agent == AgentFlag::Bob)
			owner = "B";
		if (g.Agent == AgentFlag::Random)
			owner = "R";
		printf("%s[%u][%u]", owner, g.MajorIndex, g.MinorIndex);
	}
	void VisitAdditionGate(Gate *that, Precedence outer)
	{
		auto &g = that->AsAdditionGate;
		if (outer < AdditionPrecedence)
			putchar('(');
		this->VisitDispatcher(&container[g.Augend], AdditionPrecedence);
		putchar(' '); putchar('+'); putchar(' ');
		this->VisitDispatcher(&container[g.Addend], AdditionPrecedence);
		if (outer < AdditionPrecedence)
			putchar(')');
	}
	void VisitNegationGate(Gate *that, Precedence outer)
	{
		auto &g = that->AsNegationGate;
		if (outer != LowestPrecedence)
			putchar('(');
		putchar('-');
		this->VisitDispatcher(&container[g.Target], NegationPrecedence);
		if (outer != LowestPrecedence)
			putchar(')');
	}
	void VisitSubtractionGate(Gate *that, Precedence outer)
	{
		auto &g = that->AsSubtractionGate;
		if (outer <= SubtractionPrecedence)
			putchar('(');
		/* The first term hasn't met a minus sign.
		* The SubtractionPrecedence is used to indicate
		* the presence of a minus sign so that further
		* addition/subtraction are grouped properly.
		*/
		this->VisitDispatcher(&container[g.Minuend], AdditionPrecedence);
		putchar(' '); putchar('-'); putchar(' ');
		this->VisitDispatcher(&container[g.Subtrahend], SubtractionPrecedence);
		if (outer <= SubtractionPrecedence)
			putchar(')');
	}
	void VisitMultiplicationGate(Gate *that, Precedence outer)
	{
		auto &g = that->AsMultiplicationGate;
		if (outer < MultiplicationPrecedence)
			putchar('(');
		this->VisitDispatcher(&container[g.Multiplier], MultiplicationPrecedence);
		putchar(' '); putchar('*'); putchar(' ');
		this->VisitDispatcher(&container[g.Multiplicand], MultiplicationPrecedence);
		if (outer < MultiplicationPrecedence)
			putchar(')');
	}
};

template <typename TContainer>
ExpressionPrinter<TContainer> MakePrinter(TContainer &cont)
{
	return cont;
}

int main()
{
	/* Creates and compiles a one-shot OLE circuit. */
	TwoPartyCircuit<> circuit;
	auto const aliceX = circuit.InsertGate(InputGateData{ AgentFlag::Alice, 0, 0 });
	auto const bobA = circuit.InsertGate(InputGateData{ AgentFlag::Bob, 0, 0 });
	auto const bobB = circuit.InsertGate(InputGateData{ AgentFlag::Bob, 1, 0 });
	circuit.AliceInputBegin = aliceX;
	circuit.AliceInputEnd = aliceX + 1;
	circuit.BobInputBegin = bobA;
	circuit.BobInputEnd = bobB + 1;
	auto const AX = circuit.InsertGate(MultiplicationGateData{ bobA, aliceX });
	auto const AXB = circuit.InsertGate(AdditionGateData{ AX, bobB });
	circuit.AliceOutput.push_back(AXB);
	Garbled::EncodingCircuit<> encoderStorage;
	Garbled::DecodingCircuit<> decoderStorage;
	CompileToDare(circuit, encoderStorage, decoderStorage);
    /* Prints the encoding circuit. */
	do
	{
		auto &encoder = encoderStorage;
		auto encoderPrinter = MakePrinter(encoder.Gates);
		printf("EncodingCircuit contains %d gate(s).\n",
			(int)encoder.Gates.size());
		for (int i = 0; i != (int)encoder.Gates.size(); ++i)
		{
			encoderPrinter.Print(i);
			putchar('\n');
		}
		printf("EncodingCircuit contains %d offline encoding(s).\n",
			(int)encoder.OfflineEncoding.size());
		for (auto offline : encoder.OfflineEncoding)
		{
			putchar('\t');
			encoderPrinter.Print(offline);
			putchar('\n');
		}
		printf("EncodingCircuit contains %d Alice input(s).\n",
			(int)encoder.AliceEncoding.size());
		for (auto &alice : encoder.AliceEncoding)
		{
			puts("----------------------------------------");
			for (auto const &ak : alice)
			{
				puts("--------------------");
				encoderPrinter.Print(ak.Coefficient);
				putchar('\n');
				encoderPrinter.Print(ak.Intercept);
				putchar('\n');
			}
			puts("--------------------");
		}
		puts("----------------------------------------");
		printf("EncodingCircuit contains %d Bob input(s).\n",
			(int)encoder.BobEncoding.size());
		for (auto &bob : encoder.BobEncoding)
		{
			puts("----------------------------------------");
			for (auto const &bk : bob)
			{
				puts("--------------------");
				encoderPrinter.Print(bk.Coefficient);
				putchar('\n');
				encoderPrinter.Print(bk.Intercept);
				putchar('\n');
			}
			puts("--------------------");
		}
		puts("----------------------------------------");
	} while (false);
    /* Prints the decoding circuit. */
	do
	{
		auto &decoder = decoderStorage;
		auto decoderPrinter = MakePrinter(decoder.Gates);
		printf("DecodingCircuit contains %d gate(s).\n",
			(int)decoder.Gates.size());
		for (int i = 0; i != (int)decoder.Gates.size(); ++i)
		{
			decoderPrinter.Print(i);
			putchar('\n');
		}
		printf("DecodingCircuit contains %d offline encoding(s).\n",
			(int)decoder.OfflineEncoding.size());
		for (auto offline : decoder.OfflineEncoding)
		{
			putchar('\t');
			decoderPrinter.Print(offline);
			putchar('\n');
		}
		printf("DecodingCircuit contains %d Alice input(s).\n",
			(int)decoder.AliceEncoding.size());
		for (auto &alice : decoder.AliceEncoding)
		{
			putchar('\t');
			bool isFirst = true;
			for (auto const ak : alice)
			{
				if (!isFirst)
					putchar(','), putchar(' ');
				isFirst = false;
				decoderPrinter.Print(ak);
			}
			putchar('\n');
		}
		printf("DecodingCircuit contains %d Bob input(s).\n",
			(int)decoder.BobEncoding.size());
		for (auto &bob : decoder.BobEncoding)
		{
			putchar('\t');
			bool isFirst = true;
			for (auto const bk : bob)
			{
				if (!isFirst)
					putchar(','), putchar(' ');
				isFirst = false;
				decoderPrinter.Print(bk);
			}
			putchar('\n');
		}
		printf("DecodingCircuit contains %d Alice output(s).\n",
			(int)decoder.AliceOutput.size());
		for (auto const ao : decoder.AliceOutput)
		{
			putchar('\t');
			decoderPrinter.Print(ao);
			putchar('\n');
		}
	} while (false);
	return 0;
}
