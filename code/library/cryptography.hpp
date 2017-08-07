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

    template <unsigned p, typename TBaseType = unsigned>
    struct Z
    {
        Z() : raw_value { 0 } { }
        Z(TBaseType const &raw) : raw_value { raw % p } { }
        Z(Z &&) = default;
        Z(Z const &) = default;
        Z &operator = (Z &&) = default;
        Z &operator = (Z const &) = default;
        friend bool operator == (Z const &a, Z const &b)
        {
            return a.raw_value == b.raw_value;
        }
        friend bool operator != (Z const &a, Z const &b)
        {
            return a.raw_value != b.raw_value;
        }
        operator TBaseType const () const
        {
            return raw_value;
        }
        friend Z &operator += (Z &lhs, Z const &rhs)
        {
            return lhs = lhs + rhs;
        }
        friend Z &operator -= (Z &lhs, Z const &rhs)
        {
            return lhs = lhs - rhs;
        }
        friend Z &operator *= (Z &lhs, Z const &rhs)
        {
            return lhs = lhs * rhs;
        }
        friend Z const operator + (Z const &a, Z const &b)
        {
            return a.raw_value + b.raw_value;
        }
        friend Z const operator - (Z const &a)
        {
            return p - a.raw_value;
        }
        friend Z const operator - (Z const &a, Z const &b)
        {
            return p - b.raw_value + a.raw_value;
        }
        friend Z const operator * (Z const &a, Z const &b)
        {
            return a.raw_value * b.raw_value;
        }
    private:
        TBaseType raw_value;
    };

}

#endif // CRYPTOGRAPHY_HPP_
