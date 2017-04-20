/*******************************************************************************
 * src/sequential/bingmann-sample_sort_common.hpp
 *
 * Experiments with sequential Super Scalar String Sample-Sort (S^5).
 *
 *******************************************************************************
 * Copyright (C) 2013-2017 Timo Bingmann <tb@panthema.net>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef PSS_SRC_SEQUENTIAL_BINGMANN_SAMPLE_SORT_COMMON_HEADER
#define PSS_SRC_SEQUENTIAL_BINGMANN_SAMPLE_SORT_COMMON_HEADER

#include "bingmann-sample_sort.hpp"

#include <tlx/string/hexdump.hpp>

namespace bingmann_sample_sort {

//! Recursive TreeBuilder for full-descent and unrolled variants, constructs a
//! binary tree and the plain splitter array.
template <size_t numsplitters>
class TreeBuilderFullDescent
{
public:
    key_type* m_splitter;
    key_type* m_tree;
    unsigned char* m_lcp_iter;
    const key_type* m_samples;

    // build tree: splitter[numsplitters], splitter_tree[numsplitters + 1], and
    // splitter_lcp[numsplitters + 1].
    TreeBuilderFullDescent(
        key_type* splitter, key_type* splitter_tree,
        unsigned char* splitter_lcp,
        const key_type* samples, size_t samplesize)
        : m_splitter(splitter),
          m_tree(splitter_tree),
          m_lcp_iter(splitter_lcp),
          m_samples(samples)
    {
        key_type sentinel = 0;
        recurse(samples, samples + samplesize, 1, sentinel);

        assert(m_splitter == splitter + numsplitters);
        assert(m_lcp_iter == splitter_lcp + numsplitters);
        // overwrite sentinel lcp for first < everything bucket
        splitter_lcp[0] &= 0x80;
        // sentinel for > everything bucket
        splitter_lcp[numsplitters] = 0;
    }

    ptrdiff_t snum(const key_type* s) const
    {
        return (ptrdiff_t)(s - m_samples);
    }

    key_type recurse(const key_type* lo, const key_type* hi,
                     unsigned int treeidx, key_type& rec_prevkey)
    {
        LOGC(debug_splitter)
            << "rec_buildtree(" << snum(lo) << "," << snum(hi)
            << ", treeidx=" << treeidx << ")";

        // pick middle element as splitter
        const key_type* mid = lo + (ptrdiff_t)(hi - lo) / 2;

        LOGC(debug_splitter)
            << "tree[" << treeidx << "] = samples[" << snum(mid) << "] = "
            << tlx::hexdump_type(*mid);

        key_type mykey = m_tree[treeidx] = *mid;
#if 1
        const key_type* midlo = mid;
        while (lo < midlo && *(midlo - 1) == mykey) midlo--;

        const key_type* midhi = mid;
        while (midhi + 1 < hi && *midhi == mykey) midhi++;

        if (midhi - midlo > 1)
            DBG(0, "key range = [" << snum(midlo) << "," << snum(midhi) << ")");
#else
        const key_type* midlo = mid, * midhi = mid + 1;
#endif
        if (2 * treeidx < numsplitters)
        {
            key_type prevkey = recurse(lo, midlo, 2 * treeidx + 0, rec_prevkey);

            key_type xorSplit = prevkey ^ mykey;

            LOGC(debug_splitter)
                << "    lcp: " << tlx::hexdump_type(prevkey)
                << " XOR " << tlx::hexdump_type(mykey)
                << " = " << tlx::hexdump_type(xorSplit)
                << " - " << count_high_zero_bits(xorSplit)
                << " bits = " << count_high_zero_bits(xorSplit) / 8
                << " chars lcp";

            *m_splitter++ = mykey;

            // marker for done splitters
            *m_lcp_iter++ =
                (count_high_zero_bits(xorSplit) / 8) |
                // marker for done splitters
                ((mykey & 0xFF) ? 0 : 0x80);

            return recurse(midhi, hi, 2 * treeidx + 1, mykey);
        }
        else
        {
            key_type xorSplit = rec_prevkey ^ mykey;

            LOGC(debug_splitter)
                << "    lcp: " << tlx::hexdump_type(rec_prevkey)
                << " XOR " << tlx::hexdump_type(mykey)
                << " = " << tlx::hexdump_type(xorSplit)
                << " - " << count_high_zero_bits(xorSplit)
                << " bits = " << count_high_zero_bits(xorSplit) / 8
                << " chars lcp";

            *m_splitter++ = mykey;

            // marker for done splitters
            *m_lcp_iter++ =
                (count_high_zero_bits(xorSplit) / 8) |
                // marker for done splitters
                ((mykey & 0xFF) ? 0 : 0x80);

            return mykey;
        }
    }
};

//! Recursive TreeBuilder for full-descent and unrolled variants, constructs
//! only a binary tree
template <size_t numsplitters>
class TreeBuilderFullDescentTreeCalc
{
public:
    key_type* m_tree;
    unsigned char* m_lcp_iter;
    const key_type* m_samples;

    //! build tree, sizes: splitter_tree[numsplitters + 1] and
    TreeBuilderFullDescentTreeCalc(
        key_type* splitter_tree, unsigned char* splitter_lcp,
        const key_type* samples, size_t samplesize)
        : m_tree(splitter_tree),
          m_lcp_iter(splitter_lcp),
          m_samples(samples)
    {
        key_type sentinel = 0;
        recurse(samples, samples + samplesize, 1, sentinel);

        assert(m_lcp_iter == splitter_lcp + numsplitters);
        // overwrite sentinel lcp for first < everything bucket
        splitter_lcp[0] &= 0x80;
        // sentinel for > everything bucket
        splitter_lcp[numsplitters] = 0;
    }

    ptrdiff_t snum(const key_type* s) const
    {
        return (ptrdiff_t)(s - m_samples);
    }

    key_type recurse(const key_type* lo, const key_type* hi,
                     unsigned int treeidx, key_type& rec_prevkey)
    {
        LOGC(debug_splitter)
            << "rec_buildtree(" << snum(lo) << "," << snum(hi)
            << ", treeidx=" << treeidx << ")";

        // pick middle element as splitter
        const key_type* mid = lo + (ptrdiff_t)(hi - lo) / 2;

        LOGC(debug_splitter)
            << "tree[" << treeidx << "] = samples[" << snum(mid) << "] = "
            << tlx::hexdump_type(*mid);

        key_type mykey = m_tree[treeidx] = *mid;
#if 1
        const key_type* midlo = mid;
        while (lo < midlo && *(midlo - 1) == mykey) midlo--;

        const key_type* midhi = mid;
        while (midhi + 1 < hi && *midhi == mykey) midhi++;

        if (midhi - midlo > 1)
            LOG0 << "key range = [" << snum(midlo) << "," << snum(midhi) << ")";
#else
        const key_type* midlo = mid, * midhi = mid + 1;
#endif
        if (2 * treeidx < numsplitters)
        {
            key_type prevkey = recurse(lo, midlo, 2 * treeidx + 0, rec_prevkey);

            key_type xorSplit = prevkey ^ mykey;

            LOGC(debug_splitter)
                << "    lcp: " << tlx::hexdump_type(prevkey)
                << " XOR " << tlx::hexdump_type(mykey)
                << " = " << tlx::hexdump_type(xorSplit)
                << " - " << count_high_zero_bits(xorSplit)
                << " bits = " << count_high_zero_bits(xorSplit) / 8
                << " chars lcp";

            *m_lcp_iter++ =
                (count_high_zero_bits(xorSplit) / 8) |
                // marker for done splitters
                ((mykey & 0xFF) ? 0 : 0x80);

            return recurse(midhi, hi, 2 * treeidx + 1, mykey);
        }
        else
        {
            key_type xorSplit = rec_prevkey ^ mykey;

            LOGC(debug_splitter)
                << "    lcp: " << tlx::hexdump_type(rec_prevkey)
                << " XOR " << tlx::hexdump_type(mykey)
                << " = " << tlx::hexdump_type(xorSplit)
                << " - " << count_high_zero_bits(xorSplit)
                << " bits = " << count_high_zero_bits(xorSplit) / 8
                << " chars lcp";

            *m_lcp_iter++ =
                (count_high_zero_bits(xorSplit) / 8) |
                // marker for done splitters
                ((mykey & 0xFF) ? 0 : 0x80);

            return mykey;
        }
    }
};

} // namespace bingmann_sample_sort

#endif // !PSS_SRC_SEQUENTIAL_BINGMANN_SAMPLE_SORT_COMMON_HEADER

/******************************************************************************/
