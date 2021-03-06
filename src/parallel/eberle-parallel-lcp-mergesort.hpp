/*******************************************************************************
 * src/parallel/eberle-parallel-lcp-mergesort.hpp
 *
 * Parallel LCP aware merge sort.
 *
 *******************************************************************************
 * Copyright (C) 2014 Andreas Eberle <email@andreas-eberle.com>
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

#ifndef PSS_SRC_PARALLEL_EBERLE_PARALLEL_LCP_MERGESORT_HEADER
#define PSS_SRC_PARALLEL_EBERLE_PARALLEL_LCP_MERGESORT_HEADER

#include <iostream>
#include <utility>

#include "eberle-parallel-lcp-merge-lcp-splitting.hpp"
#include "eberle-parallel-lcp-merge-standard-splitting.hpp"
#include "eberle-parallel-lcp-merge-binary-splitting.hpp"
#include "../sequential/bingmann-lcp_mergesort_kway.hpp"

#include "../tools/jobqueue.hpp"
#include "../tools/stringtools.hpp"

namespace eberle_parallel_lcp_mergesort {

using std::numeric_limits;

using namespace jobqueue;
using namespace stringtools;

//due to ambigous symbol
using stringtools::string;

//debugging constants
static const bool debug_toplevel_merge_duration = true;

//constants
static const unsigned MERGESORT_BRANCHES = 64;

//method definitions

static inline void
eberle_parallel_lcp_mergesort(string* strings, size_t n, void (* parallelMerge)(const LcpCacheStringPtr*, unsigned, string*, size_t))
{
    int realNumaNodes = numa_num_configured_nodes();
    if (realNumaNodes < 1) realNumaNodes = 1;
    const unsigned topLevelBranches = omp_get_max_threads();

    unsigned threadsPerNode = ceil(float(omp_get_max_threads()) / realNumaNodes);
    if (threadsPerNode < 1) threadsPerNode = 1;

    // calculate ranges
    std::pair<size_t, size_t> ranges[topLevelBranches];
    calculateRanges(ranges, topLevelBranches, n);

    LcpCacheStringPtr stringPtr[topLevelBranches];

#pragma omp parallel for
    for (unsigned k = 0; k < topLevelBranches; k++)
    {
        const size_t length = ranges[k].second;
        const unsigned numaNode = k / threadsPerNode;

        // tie thread to a NUMA node
        numa_run_on_node(numaNode);
        numa_set_preferred(numaNode);

        // allocate memory on numa node
        stringPtr[k].allocateNumaMemory(numaNode, length);

        // execute mergesort
        bingmann::lcp_mergesort_cache_kway<MERGESORT_BRANCHES>(
            strings + ranges[k].first, stringPtr[k]);
    }

    // do top level merge
    parallelMerge(stringPtr, topLevelBranches, strings, n);

    // free memory
    for (unsigned k = 0; k < topLevelBranches; k++)
    {
        stringPtr[k].freeNumaMemory();
    }
}

void eberle_parallel_lcp_mergesort_lcp_splitting(string* strings, size_t n)
{
    eberle_parallel_lcp_mergesort(strings, n, eberle_parallel_lcp_merge::parallelLcpMerge);
}

void eberle_parallel_lcp_mergesort_standard_splitting(string* strings, size_t n)
{
    eberle_parallel_lcp_mergesort(strings, n, eberle_parallel_lcp_merge::parallelLcpMergeStandardSplitting);
}

void eberle_parallel_lcp_mergesort_binary_splitting(string* strings, size_t n)
{
    eberle_parallel_lcp_mergesort(strings, n, eberle_parallel_lcp_merge::parallelLcpMergeBinarySplitting);
}

PSS_CONTESTANT_PARALLEL(eberle_parallel_lcp_mergesort_lcp_splitting,
                        "eberle/parallel-lcp-mergesort-lcp-splitting",
                        "parallel LCP aware mergesort by Andreas Eberle")

PSS_CONTESTANT_PARALLEL(eberle_parallel_lcp_mergesort_standard_splitting,
                        "eberle/parallel-lcp-mergesort-standard-splitting",
                        "parallel LCP aware mergesort by Andreas Eberle")

PSS_CONTESTANT_PARALLEL(eberle_parallel_lcp_mergesort_binary_splitting,
                        "eberle/parallel-lcp-mergesort-binary-splitting",
                        "parallel LCP aware mergesort by Andreas Eberle")

} // namespace eberle_parallel_lcp_mergesort

#endif // !PSS_SRC_PARALLEL_EBERLE_PARALLEL_LCP_MERGESORT_HEADER

/******************************************************************************/
