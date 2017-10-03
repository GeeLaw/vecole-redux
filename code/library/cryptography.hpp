#ifndef CRYPTOGRAPHY_HPP_
#define CRYPTOGRAPHY_HPP_

#include<utility>
#include<cstddef>
#include<cstdint>

namespace Cryptography
{
    namespace AgentFlag
    {
        typedef size_t Type;
        constexpr Type None = (Type)0;
        constexpr Type Alice = (Type)1 << 0;
        constexpr Type Bob = (Type)1 << 1;
        constexpr Type Random = (Type)1 << 17;
    }

    namespace ExtendedEuclideanImpl_
    {
        template <typename T>
        struct DefaultDivider
        {
            void operator () (T &q, T &r, T &&y, T const &tmp) const
            {
                q = y / tmp;
                r = std::move(y) % tmp;
            }
        };

        /* Computes ax=by+1 where 0<=a<=y, 0<=b<=x, 0<=x<y.
         * Based on euclidean algorithm without backtracing.
         * Consider a'(y-kx)=b'x+1.
         * Then -a=ka'+b',-b=a'.
         * Let [a(0);b(0)]=(-1)^t[a11,a12;a21,a22][a(t);b(t)]
         * Then [a(t),b(t)]=(-1)[k,1;1,0][a(t+1);b(t+1)].
         * Composite the transformation as the algorithm runs.
         *   Trick 1: keep the first row only!
         *   Trick 2: the intermediate results are within the range!
         */
        template <typename TUInt, typename TDivider>
        bool EEInv(TUInt x, TUInt const &oldY, TUInt &result, TDivider divider)
        {
            TUInt y = oldY;
            TUInt k, tmp, a11 = 1, a12 = 0;
            while (true)
            {
                /* shouldNotInvertBegin: */
                if (!x)
                    return false;
                if (x == 1)
                {
                    result = std::move(a11);
                    return true;
                }
                tmp = std::move(x);
                divider(k, x, std::move(y), tmp);
                y = std::move(tmp);
                tmp = a11;
                a11 *= k;
                a11 += std::move(a12);
                a12 = std::move(tmp);
                /* goto shouldInvertBegin; */
                /* shouldInvertBegin: */
                if (!x)
                    return false;
                if (x == 1)
                {
                    result = oldY - std::move(a11);
                    return true;
                }
                tmp = std::move(x);
                divider(k, x, std::move(y), tmp);
                y = std::move(tmp);
                tmp = a11;
                a11 *= k;
                a11 += std::move(a12);
                a12 = std::move(tmp);
                /* goto shouldNotInvertBegin; */
            }
        }

        template <typename TUInt>
        bool EEInv(TUInt x, TUInt const &oldY, TUInt &result)
        {
            return EEInv(std::move(x), oldY, result, DefaultDivider<TUInt>());
        }
    }

    template <uint32_t p, typename TBaseType = uint32_t, typename TPromotedType = uint64_t>
    struct Z
    {
        constexpr Z() : raw_value(0) { }
        constexpr Z(TPromotedType raw) : raw_value((TBaseType)(raw % p)) { }
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
        friend constexpr bool operator == (Z a, TBaseType b)
        {
            return a.raw_value == b % p;
        }
        friend constexpr bool operator != (Z a, TBaseType b)
        {
            return a.raw_value != b % p;
        }
        friend constexpr bool operator == (TBaseType b, Z a)
        {
            return a.raw_value == b % p;
        }
        friend constexpr bool operator != (TBaseType b, Z a)
        {
            return a.raw_value != b % p;
        }
        constexpr operator TBaseType () const
        {
            return raw_value;
        }
        constexpr operator bool () const
        {
            return raw_value;
        }
        constexpr bool operator ! () const
        {
            return !raw_value;
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
        friend constexpr Z operator + (Z const a, Z const b)
        {
            return (TPromotedType)a.raw_value + b.raw_value;
        }
        friend constexpr Z operator - (Z const a)
        {
            return p - a.raw_value;
        }
        friend constexpr Z operator - (Z const a, Z const b)
        {
            return (TPromotedType)p - b.raw_value + a.raw_value;
        }
        friend constexpr Z operator * (Z const a, Z const b)
        {
            return (TPromotedType)a.raw_value * b.raw_value;
        }
        Z Inverse() const
        {
            TBaseType inv = 0;
            ExtendedEuclideanImpl_::EEInv(raw_value, p, inv);
            return inv;
        }
        friend Z operator / (Z const a, Z const b)
        {
            return a * b.Inverse();
        }
    private:
        TBaseType raw_value;
    };

}

#endif // CRYPTOGRAPHY_HPP_
