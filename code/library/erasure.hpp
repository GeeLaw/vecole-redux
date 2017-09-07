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
        size_t count,
        TRandomGenerator &next
    )
    {
        std::uniform_int_distribution<size_t> iDist
            (0, (size_t)(notErasedEnd - notErasedBegin) - 1);
        while (count)
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
