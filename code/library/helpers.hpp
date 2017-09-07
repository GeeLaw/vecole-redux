#ifndef HELPERS_HPP_
#define HELPERS_HPP_

#include<cstdio>

namespace Helpers
{
    template <typename TIt>
    void SaveSizeTRange(TIt begin, TIt end, FILE *fp)
    {
        if (begin == end)
            return;
        fprintf(fp, "%zu", *begin);
        for (++begin; begin != end; ++begin)
            fprintf(fp, " %zu", *begin);
        fputc('\n', fp);
    }

    template <typename TIt>
    bool LoadSizeTRange(TIt begin, TIt end, FILE *fp)
    {
        for (; begin != end; ++begin)
        {
            size_t zu;
            if (fscanf(fp, "%zu", &zu) != 1)
                return false;
            *begin = zu;
        }
        return true;
    }
}

#endif // HELPERS_HPP_
