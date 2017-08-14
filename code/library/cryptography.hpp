#ifndef CRYPTOGRAPHY_HPP_
#define CRYPTOGRAPHY_HPP_

namespace Cryptography
{
	namespace AgentFlag
	{
		typedef unsigned Type;
		constexpr Type None = 0u;
		constexpr Type Alice = 1u << 0;
		constexpr Type Bob = 1u << 1;
		constexpr Type Random = 1u << 17;
	}

    template <uint32_t p, typename TBaseType = uint32_t, typename TPromotedType = uint64_t>
    struct Z
    {
        constexpr Z() : raw_value(0) { }
        constexpr Z(TPromotedType const &raw) : raw_value((TBaseType)(raw % p)) { }
        constexpr Z(Z &&) = default;
        constexpr Z(Z const &) = default;
        Z &operator = (Z &&) = default;
        Z &operator = (Z const &) = default;
        friend constexpr bool operator == (Z a, Z b)
        {
            return a.raw_value == b.raw_value;
        }
        friend constexpr bool operator != (Z a, Z b)
        {
            return a.raw_value != b.raw_value;
        }
        constexpr operator TBaseType const () const
        {
            return raw_value;
        }
        friend Z &operator += (Z &lhs, Z const rhs)
        {
            return lhs = lhs + rhs;
        }
        friend Z &operator -= (Z &lhs, Z const rhs)
        {
            return lhs = lhs - rhs;
        }
        friend Z &operator *= (Z &lhs, Z const rhs)
        {
            return lhs = lhs * rhs;
        }
        friend constexpr Z const operator + (Z const a, Z const b)
        {
            return (TPromotedType)a.raw_value + b.raw_value;
        }
        friend constexpr Z const operator - (Z const a)
        {
            return p - a.raw_value;
        }
        friend constexpr Z const operator - (Z const a, Z const b)
        {
            return (TPromotedType)p - b.raw_value + a.raw_value;
        }
        friend constexpr Z const operator * (Z const a, Z const b)
        {
            return (TPromotedType)a.raw_value * b.raw_value;
        }
    private:
        TBaseType raw_value;
    };

}

#endif // CRYPTOGRAPHY_HPP_
