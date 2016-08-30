#include "wordbreaker_splitter.h"
#include "result_iterators.h"

#include <dict/dictutil/dictutil.h>
#include <dict/dictutil/str.h>
#include <dict/proof/wordbreaker/lib/wordbreaker.h>
#include <dict/libs/langmodels/word_2grams.h>
#include <dict/libs/langmodels/word_ngrams.h>
#include <dict/libs/langmodels/lang_model.h>

ISplitter* CreateWordbreaker(const TWord2Grams* bigrams, ECharset cp) {
    return new TWordbreakerSplitter(bigrams, cp);
}

TWordbreakerSplitter::TWordbreakerSplitter(const TWord2Grams* bigrams, ECharset cp)
  : CP(cp)
  , NGrams(0)
  , Splitter(0)
{
    const TLangModelParams params(2, TLangModelParams::ZeroBackoff, 10000.0);
    NGrams = CreateNGrams(bigrams, params);
    Splitter = CreateWordBreaker(NGrams.Get());
}

ISplitResultsIterator* TWordbreakerSplitter::Split(const VectorWStrok& queryWords, int maxVariants) const {
    NStl::vector<VectorWStrok> results;
    results.clear();

    TWordBreakerResponse response;
    Stroka text = WideToChar(CP, Join("", queryWords));
    TWordBreakerParams params;
    params.MaxTotalVariants = maxVariants;
    params.MaxHandicap = 11;
    params.ForceBestOnly = true;
    Splitter->Break(text, params, &response);
    for (int i = 0; i < response.ysize(); ++i) {
        results.push_back(TextParts(text, response[i].Delimiters));
    }

    return YieldFromVector(results);
}

VectorWStrok TWordbreakerSplitter::TextParts(const Stroka& text, const yvector<int>& delimiters) const {
    VectorWStrok result;
    const WStroka wideText = CharToWide(CP, text);
    int previousDelimiter = 0;
    for (int i = 0; i < delimiters.ysize(); ++i) {
        int delimiter = delimiters[i];
        result.push_back(wideText.substr(previousDelimiter, delimiter - previousDelimiter));
        previousDelimiter = delimiter;
    }
    result.push_back(wideText.substr(previousDelimiter, static_cast<int>(wideText.length()) - previousDelimiter));
    return result;
}
