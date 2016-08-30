#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// result_iterators.h - functions for helping common splitters implement ISplitResultsIterator
//
// A splitter, for which it's cheap to save all variants to the vector at once, should do so and
// use these helpers, like this:
//
//          VectorWStrok query = MakeSomethingUp();
//          NStl::vector<VectorWStrok> results = ReceiveSplitVariantsFromOuterSpace(query);
//          return FilterBadVariants(YieldResultsFromVector(results), query, true);

#include "splitter.h"

// Returns iterator, that replaces variants, matching queryWords with empty phrases (as
// required by ISplitter/ISplitResultsIterator interfaces).
//
ISplitResultsIterator* FilterBadVariants(NStl::auto_ptr<ISplitResultsIterator> iterator,
                                         const VectorWStrok& queryWords);

ISplitResultsIterator* YieldNoResults(); // return empty iterator
ISplitResultsIterator* YieldFromVector(const NStl::vector<VectorWStrok>& results);
