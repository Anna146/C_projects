#include "result_iterators.h"

#include <dict/dictutil/exceptions.h>

namespace {

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TFilteringIterator
//

class TFilteringIterator: public ISplitResultsIterator {
    DECLARE_NOCOPY(TFilteringIterator);

public:
    TFilteringIterator(NStl::auto_ptr<ISplitResultsIterator> iterator,
                       const VectorWStrok& queryWords);

    virtual bool Next(VectorWStrok* result);

private:
    NStl::auto_ptr<ISplitResultsIterator> Iterator;
    VectorWStrok QueryWords;
};


TFilteringIterator::TFilteringIterator(NStl::auto_ptr<ISplitResultsIterator> iterator,
                                       const VectorWStrok& queryWords)
    : Iterator(iterator)
    , QueryWords(queryWords)
{}

bool TFilteringIterator::Next(VectorWStrok* result) {
    CHECK_ARG_NULL(result, "result");

    if (!Iterator->Next(result))
        return false;
    if (*result == QueryWords)
        result->clear();
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TVectorIterator
//

class TVectorIterator: public ISplitResultsIterator {
    DECLARE_NOCOPY(TVectorIterator);

public:
    TVectorIterator(const NStl::vector<VectorWStrok>& results);
    virtual bool Next(VectorWStrok* result);

private:
    NStl::vector<VectorWStrok> Results;
    size_t Index;
};

TVectorIterator::TVectorIterator(const NStl::vector<VectorWStrok>& results)
    : Results(results)
    , Index(0)
{}

bool TVectorIterator::Next(VectorWStrok* result) {
    CHECK_ARG_NULL(result, "result");

    result->clear();
    if (Index >= Results.size())
        return false;
    *result = Results[Index++];
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TEmptyIterator
//

class TEmptyIterator: public ISplitResultsIterator {
    DECLARE_NOCOPY(TEmptyIterator);

public:
    TEmptyIterator() {}
    virtual bool Next(VectorWStrok* result);
};

bool TEmptyIterator::Next(VectorWStrok* result) {
    CHECK_ARG_NULL(result, "result");
    result->clear();
    return false;
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions
//

ISplitResultsIterator* FilterBadVariants(NStl::auto_ptr<ISplitResultsIterator> iterator,
                                         const VectorWStrok& queryWords)
{
    return new TFilteringIterator(iterator, queryWords);
}

ISplitResultsIterator* YieldNoResults() {
    return new TEmptyIterator;
}

ISplitResultsIterator* YieldFromVector(const NStl::vector<VectorWStrok>& results) {
    return new TVectorIterator(results);
}

