#ifndef ERASURE_HPP_
#define ERASURE_HPP_

#include<random>

namespace Encoding
{
namespace Erasure
{
    template
    <
        typename TRandomAccessInputOutputIt,
        typename TRandomGenerator
    >
    void EraseSubsetExact
    (
        TRandomAccessInputOutputIt const notErasedBegin,
        TRandomAccessInputOutputIt const notErasedEnd,
        unsigned count,
        TRandomGenerator &next
    )
    {
        std::uniform_int_distribution<unsigned> iDist
            (0u, (unsigned)(notErasedEnd - notErasedBegin) - 1u);
        while (count != 0u)
        {
            auto toErase = iDist(next);
            if (notErasedBegin[toErase])
            {
                notErasedBegin[toErase] = false;
                --count;
            }
        }
    }
}
}

#endif // ERASURE_HPP_
