#pragma once

#include "splitter.h"
#include <util/generic/ptr.h>

class TWordNGrams;
class IWordBreaker;

class TWordbreakerSplitter : public ISplitter {
    DECLARE_NOCOPY(TWordbreakerSplitter);
public:
    TWordbreakerSplitter(const TWord2Grams* bigrams, ECharset cp);

    // ISplitter methods.
    virtual ISplitResultsIterator* Split(const VectorWStrok& queryWords, int maxVariants) const;

private:
    ECharset CP;
    TAutoPtr<const TWordNGrams> NGrams;
    TAutoPtr<IWordBreaker> Splitter;

private:
    VectorWStrok TextParts(const Stroka& text, const yvector<int>& delimiters) const;
};
