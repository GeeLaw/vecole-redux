#ifndef BIGUINT_HPP_
#define BIGUINT_HPP_

#include<cstring>

namespace Math
{
namespace BigUInt
{
    typedef uint32_t DigitType;
    typedef uint64_t PromotedDigitType;
    constexpr DigitType HighMask = ((DigitType)1) << 31;
    constexpr DigitType GetHighFromPromoted(PromotedDigitType promoted)
    {
        return (DigitType)(promoted >> 32);
    }
    constexpr DigitType GetLowFromPromoted(PromotedDigitType promoted)
    {
        return (DigitType)(promoted & ~(DigitType)0);
    }

    template <unsigned limbs>
    struct UInt
    {
        DigitType Storage[limbs];
        void Reset()
        {
            std::memset(Storage, 0, sizeof Storage);
        }
        bool IsZero() const
        {
            for (unsigned i = 0u; i != limbs; ++i)
                if (Storage[i] != (DigitType)0)
                    return false;
            return true;
        }
        bool ShiftLeft1()
        {
            bool carried = false;
            for (unsigned i = 0u; i != limbs; ++i)
            {
                bool newCarried = ((Storage[i] & HighMask) != 0);
                Storage[i] <<= 1;
                if (carried)
                    Storage[i] |= 1;
                carried = newCarried;
            }
            return carried;
        }
        DigitType ShiftLeftDigit()
        {
            DigitType thrown = Storage[limbs - 1u];
            for (unsigned i = limbs - 1u; i != 0u; --i)
                Storage[i] = Storage[i - 1u];
            Storage[0] = 0;
            return thrown;
        }
        DigitType ShiftRightDigit()
        {
            DigitType thrown = Storage[0];
            for (unsigned i = 0u; i + 1u != limbs; ++i)
                Storage[i] = Storage[i + 1u];
            Storage[limbs - 1u] = 0;
            return thrown;
        }
        bool Add(UInt const &other)
        {
            if (this == &other)
                return ShiftLeft1();
            bool carried = false;
            for (unsigned i = 0u; i != limbs; ++i)
            {
                if (carried && ++Storage[i] != (DigitType)0)
                    carried = false;
                auto const old = Storage[i];
                Storage[i] += other.Storage[i];
                if (Storage[i] < old)
                    carried = true;
            }
            return carried;
        }
        bool Subtract(UInt const &other)
        {
            if (this == &other)
            {
                Reset();
                return false;
            }
            bool borrowed = false;
            for (unsigned i = 0u; i != limbs; ++i)
            {
                if (borrowed && --Storage[i] != -(DigitType)1)
                    borrowed = false;
                auto const old = Storage[i];
                Storage[i] -= other.Storage[i];
                if (Storage[i] > old)
                    borrowed = true;
            }
            return borrowed;
        }
        static int Compare(UInt const &a, UInt const &b)
        {
            if (&a == &b)
                return 0;
            for (unsigned i = limbs - 1u; i != 0u - 1u; --i)
                if (a.Storage[i] > b.Storage[i])
                    return 1;
                else if (a.Storage[i] < b.Storage[i])
                    return -1;
            return 0;
        }
        bool AddProductNonself(UInt const &a, UInt const &b)
        {
            bool carried = false;
            for (unsigned i = 0u; i != limbs; ++i)
            {
                DigitType carry = 0;
                for (unsigned j = 0u; i + j != limbs; ++j)
                {
                    /* Careful analysis shows that carry
                     * will never overflow here.
                     */
                    DigitType &target = Storage[i + j];
                    DigitType old = target;
                    carry = ((target += carry) < old ? 1 : 0);
                    PromotedDigitType promoted = a.Storage[i];
                    promoted *= b.Storage[j];
                    carry += GetHighFromPromoted(promoted);
                    old = target;
                    if ((target += GetLowFromPromoted(promoted)) < old)
                        ++carry;
                }
                if (carry != (DigitType)0)
                    carried = true;
            }
            return carried;
        }
        bool Multiply(DigitType const digit)
        {
            DigitType carry = 0;
            for (unsigned i = 0u; i != limbs; ++i)
            {
                DigitType oldCarry = carry;
                DigitType &target = Storage[i];
                PromotedDigitType promoted = target;
                promoted *= digit;
                carry = GetHighFromPromoted(promoted);
                DigitType old;
                target = old = GetLowFromPromoted(promoted);
                if ((target += oldCarry) < old)
                    ++carry;
            }
            return carry != (DigitType)0;
        }
        static bool Divide(UInt &numerator, UInt &denominator, UInt &quotient)
        {
            unsigned denoLimb = limbs;
            for (; denoLimb != 0u
                && denominator[denoLimb - 1u] == (DigitType)0;
                --denoLimb)
                ;
            if (denoLimb == 0u)
                return false;
            quotient.reset();
            unsigned numLimb = limbs;
            for (; numLimb != 0u
                && numerator[numLimb - 1u] == (DigitType)0;
                --numLimb)
                ;
            if (numLimb < denoLimb)
                return true;
            denominator.ShiftLeftDigit(numLimb - denoLimb);
            for (unsigned i = numLimb - denoLimb; i != 0u - 1u; --i)
            {
                SolveDivideDigit(numberator, denominator, quotient.Storage[i]);
                denominator.ShiftRightDigit(1);
            }
            return true;
        }
    private:
        static void SolveDivideDigit(UInt &numerator,
            UInt const &denominator,
            DigitType &target)
        {
            UInt helper;
            DigitType left = 0, right = -(DigitType)1;
            while (right - left > (DigitType)1)
            {
                DigitType mid = ((left + right) >> 1);
                helper = denominator;
                if (helper.Multiply(mid)
                    || Compare(numerator, helper) < 0)
                    right = mid - 1u;
                else
                    left = mid;
            }
            helper = denominator;
            if (helper.Multiply(right)
                || Compare(numerator, helper) < 0)
            {
                target = left;
                helper = denominator;
                helper.Multiply(left);
                numerator.Subtract(helper);
            }
            else
            {
                target = right;
                numerator.Subtract(helper);
            }
        }
    };
}
}


#endif // BIGUINT_HPP_
