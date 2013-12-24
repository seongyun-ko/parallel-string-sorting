#ifndef BINGMANN_PARALLEL_SAMPLE_SORT_H_
#define BINGMANN_PARALLEL_SAMPLE_SORT_H_

#include "../tools/stringtools.h"

namespace bingmann_parallel_sample_sort {

void
parallel_sample_sort_numa(const stringtools::StringPtrNoLcpCalc& strptr, int numaNode, int numberOfThreads);

void
parallel_sample_sort_numa(const stringtools::StringPtrOutNoLcpCalc& strptr, int numaNode, int numberOfThreads);


} // namespace bingmann_parallel_sample_sort

namespace bingmann_parallel_sample_sort_lcp {

void
parallel_sample_sort_numa(const stringtools::StringPtr& strptr, int numaNode, int numberOfThreads);

void
parallel_sample_sort_numa(const stringtools::StringPtrOut& strptr, int numaNode, int numberOfThreads);


} // namespace bingmann_parallel_sample_sort_lcp

#endif // BINGMANN_PARALLEL_SAMPLE_SORT_H_
