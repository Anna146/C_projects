#pragma once

#include "splitter.h"

////////////////////////////////////////////////////////////////////////////////
//
// StandardSplitter
//

class TStandardSplitter : public ISplitter {
    DECLARE_NOCOPY(TStandardSplitter);

// Constructor
public:
    TStandardSplitter(const TWord2Grams* biGrams);

// Methods
public:
    virtual ISplitResultsIterator* Split(const VectorWStrok& words, int maxVariants) const;

// Types
private:
    class TRequest;

// Fields
private:
    const TWord2Grams* BiGrams;
    i64 MaxFreq;
    i32 MinFreq;
    i32 GoodFreq;
};
