#pragma once

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// splitter.h - main splitter interface
//

#include <util/charset/doccodes.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>

typedef Wtroka WStroka;
typedef yvector<WStroka> VectorWStrok;
class TWord2Grams;

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// ISplitter
//

class ISplitResultsIterator;

class ISplitter {
public:
    virtual ~ISplitter() {}

    // RULES (that descendants should follow):
    //
    //   1) It should be guaranteed, that among the results, yielded by returned iterator, there
    //      will no phrases, exactly matching original queryWords.
    //
    //   2) Empty phrase returned from iterator means "same as original queryWords". When caller
    //      receives an empty phrase, it means (see rule 3) that all subsequent variants are worse
    //      than original queryWords (from the splitter's point of view).
    //
    //   3) Variants should be yielded from the iterator in order of 'goodness': best
    //      variants first. Splitter might not yield the empty phrase (that means 'original
    //      variant') if it is the worst variant among others (see rule 2).
    //
    //   4) There should be no identical variants.
    //
    //   5) Resulting pointer should be non-null. If a splitter wishes to return no results,
    //      it should return an iterator, Next method of which returns false after first call
    //      (see also >>> result_iterators.h <<< for iterator utilities).
    //
    // THREAD SAFETY:
    //     Implementors should design Split function to be REENTRANT, so that many threads would
    //     be able to call it simultaneously.
    //
    // Returned ISplitResultsIterator* should be DELETED BY CALLER.
    // SEE ALSO: rules at ISplitResultsIterator.
    //
    virtual ISplitResultsIterator* Split(const VectorWStrok& queryWords, int maxVariants) const = 0;
};

// See README for splitters comparison
//
ISplitter* CreateStandardSplitter(const TWord2Grams* bigrams);
ISplitter* CreateWordbreaker(const TWord2Grams* bigrams, ECharset cp);
ISplitter* CreateWordbreaker2(const TWord2Grams* bigrams);
ISplitter* CreateWordbreaker3(const TWord2Grams* bigrams);

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// ISplitResultsIterator
//

class ISplitResultsIterator {
public:
    virtual ~ISplitResultsIterator() {}

    // RULES (that descendants should follow):
    //
    //   1) Next should write successive result to "result" vector and return true, OR, if
    //      there are no more results left, clear "result" vector and return false.
    //
    //   2) After Next first time returned false, subsequent calls to Next should also yield
    //      nothing: return false and clear "result" vector.
    //
    virtual bool Next(VectorWStrok* result) = 0;
};
