#include "language_model_for_splitting.h"

#include <dict/dictutil/db.h>
#include <dict/dictutil/dictutil.h>
#include <dict/dictutil/str.h>
#include <dict/libs/langmodels/word_2grams.h>
#include <util/generic/ymath.h>

TLanguageModelForSplitting::TLanguageModelForSplitting(const TWord2Grams* bigrams,
                                                       const VectorWStrok& query,
                                                       const TParameters& parameters)
    : Bigrams(bigrams)
    , JoinedText(Join(S::Empty, query))
    , KeyForJoinedText(Bigrams->MakeKey(JoinedText))
    , N(JoinedText.length())
    , Parameters(parameters)
    , NumUnigramsInModel(Bigrams->MaxFreq())
    , NumDigitsInPrefix()
{
    // If this fails, then bigrams use multibyte encoding for their keys.
    // If so, KeyForWordAt should construct keys with WordAt and Bigrams->MakeKey, rather than
    // KeyForJoinedText.substr().
    //
    assert(JoinedText.length() == KeyForJoinedText.length());

    if (Parameters.MaxWordLength <= 0)
        Parameters.MaxWordLength = JoinedText.length();

    CachedUnigramFreqs.ReDim(N, Parameters.MaxWordLength);
    CachedUnigramFreqs.Zero();
    CachedUnigramHints.ReDim(N, Parameters.MaxWordLength);
    CachedUnigramHints.Zero();

    // Turns out, it's more efficient to cache all unigrams at the start, rather than to
    // query them on-demand.
    //
    // To decrease count of LM requests: for each substring "query[start:]"
    // we find longest prefix in language model.
    //
    std::auto_ptr<IDbRecordSet> recordSet(Bigrams->GetRecordSet());
    for (int start = 0; start < N; ++start) {
        const Stroka key = KeyForWordAt(start, N - 1);
        recordSet->Seek(key, DBSEEK_BEFORE_EQ);
        Stroka found;
        recordSet->GetKey(found);
        int len = (int)CommonPrefix(key, found);
        len = min3(len, Parameters.MaxWordLength, N - start);

        for (int end = start; end < start + len; ++end)
            CacheUnigram(start, end);
    }

    // Compute number of digits in prefixes
    //
    if (Parameters.DoNumberToLengthBackoff) {
        NumDigitsInPrefix.resize(N);
        int numDigitsSoFar = 0;
        for (int pos = 0; pos < N; ++pos) {
            if (IsDigit(JoinedText[pos]) || JoinedText[pos] == '.')
                ++numDigitsSoFar;
            NumDigitsInPrefix[pos] = numDigitsSoFar;
        }
    }

    // Fill word indices
    //
    if (Parameters.DoBackoffForQueryWords) {
        WordIndex.resize(N);
        int wordStart = 0;
        for (size_t wordIndex = 0; wordIndex < query.size(); ++wordIndex) {
            for (int pos = wordStart; pos < wordStart + (int)query[wordIndex].length(); ++pos)
                WordIndex[pos] = wordIndex;
            wordStart += query[wordIndex].length();
        }
    }
}

WStroka TLanguageModelForSplitting::WordAt(int start, int end) const {
    return JoinedText.substr(start, end - start + 1);
}

double TLanguageModelForSplitting::GetWordWeight(int start, int end) const {
    if (IsOOV(start, end))
        return GetWeightForOOVWord(start, end);

    double unigramFreq = CachedUnigramFreqs[start][end - start];
    return Log2(double(NumUnigramsInModel) / unigramFreq);
}

double
TLanguageModelForSplitting::
GetConditionalWordWeight(int start1, int start2, int end2) const {
    double backoffWeight = (Parameters.DoNGramBackoff)
                           ? Parameters.BigramToUnigramBackoff + GetWordWeight(start2, end2)
                           : Parameters.OutOfVocabularyWeight;

    if (IsOOV(start1, start2 - 1) ||
        IsOOV(start2, end2) ||
        BeginsNoBigram(start1, start2 - 1))
    {
        return backoffWeight;
    }

    int length1 = start2 - start1;
    ui32 firstFreq = CachedUnigramFreqs[start1][length1 - 1];
    ui32 bigramFreq = Bigrams->KeyFreq(CachedUnigramHints[start1][length1 - 1],
                                       KeyForWordAt(start2, end2));

    return (bigramFreq == 0)
           ? backoffWeight
           : Log2(double(firstFreq) / bigramFreq);
}

double TLanguageModelForSplitting::
GetConditionalWordWeight(int start1, int start2, int start3, int end3) const {
    double backoffWeight = (Parameters.DoNGramBackoff)
                           ? Parameters.TrigramToBigramBackoff +
                             GetConditionalWordWeight(start2, start3, end3)
                           : Parameters.OutOfVocabularyWeight;

    if (Bigrams->GetOrder() < 3 ||
        IsOOV(start1, start2 - 1) ||
        IsOOV(start2, start3 - 1) ||
        IsOOV(start3, end3) ||
        BeginsNoBigram(start1, start2 - 1))
    {
        return backoffWeight;
    }

    ui32 bigramFreq = Bigrams->KeyFreq(CachedUnigramHints[start1][start2 - start1 - 1],
                                       KeyForWordAt(start2, start3 - 1));
    if (bigramFreq == 0)
        return backoffWeight;

    Stroka key23 = KeyForWordAt(start2, start3 - 1) + S::Space + KeyForWordAt(start3, end3);
    ui32 trigramFreq = Bigrams->KeyFreq(CachedUnigramHints[start1][start2 - start1 - 1], key23);

    return (trigramFreq == 0)
           ? backoffWeight
           : Log2(double(bigramFreq) / trigramFreq);
}

double TLanguageModelForSplitting::GetPhraseWeightWithBigrams(const VectorWStrok& phrase) const {
    assert(Join(S::Empty, phrase) == JoinedText);

    double result = 0;
    int totalLength = 0;
    int start2 = 0;
    int start1 = 0;

    // count first word
    //
    totalLength += phrase[0].length();
    result += GetWordWeight(0, totalLength - 1);

    // count all other words
    //
    for (size_t i = 1; i < phrase.size(); ++i) {
        start1 = start2;
        start2 = totalLength;

        totalLength += phrase[i].length();
        result += GetConditionalWordWeight(start1, start2, totalLength - 1);
    }

    return result;
}

double TLanguageModelForSplitting::GetPhraseWeightWithTrigrams(const VectorWStrok& phrase) const {
    assert(Join(S::Empty, phrase) == JoinedText);

    if (Bigrams->GetOrder() < 3)
        return GetPhraseWeightWithBigrams(phrase);

    double result = 0;
    int totalLength = 0;

    // count first and second words
    //
    if (phrase.size() == 1)
        return GetWordWeight(0, (int)JoinedText.length() - 1);

    int start1 = 0;
    int start2 = phrase[0].length();
    int start3 = start2 + phrase[1].length();
    totalLength = start3;
    result += GetWordWeight(start1, start2 - 1) +
              GetConditionalWordWeight(start1, start2, start3 - 1);

    // count all other words
    //
    for (size_t i = 2; i < phrase.size(); ++i) {
        totalLength += phrase[i].length();
        result += GetConditionalWordWeight(start1, start2, start3, totalLength - 1);
        start1 = start2;
        start2 = start3;
        start3 = totalLength;
    }

    return result;
}

Stroka TLanguageModelForSplitting::KeyForWordAt(int start, int end) const {
    assert(end + 1 <= N);
    return KeyForJoinedText.substr(start, end - start + 1);
}

void TLanguageModelForSplitting::CacheUnigram(int start, int end) {
    int length = end - start + 1;

    CachedUnigramFreqs[start][length - 1] = Bigrams->KeyFreq(
        0,
        KeyForWordAt(start, end),
        &CachedUnigramHints[start][length - 1]);
}

bool TLanguageModelForSplitting::IsOOV(int start, int end) const {
    return end - start + 1 > Parameters.MaxWordLength ||
           CachedUnigramFreqs[start][end - start] == 0;
}

bool TLanguageModelForSplitting::BeginsNoBigram(int start, int end) const {
    return !CachedUnigramHints[start][end - start];
}

double TLanguageModelForSplitting::GetWeightForOOVWord(int start, int end) const {
    double result = Parameters.OutOfVocabularyWeight;

    // Compute backoff weight for word as if it is a number
    //
    if (Parameters.DoNumberToLengthBackoff) {
        int length = end - start + 1;
        if (IsNumber(start, end) && length >= Parameters.MinNumberLengthToBackOff)
            result = Min(result, Parameters.NumberToLengthBackoff + M_LOG2_10 * length);
    }

    // Compute backoff weight for word as if it is a query word
    //
    if (Parameters.DoBackoffForQueryWords && IsQueryWord(start, end))
        result = Min(result, Parameters.OOVQueryWordWeight);

    return result;
}

bool TLanguageModelForSplitting::IsNumber(int start, int end) const {
    return NumDigitsInPrefix[end] - NumDigitsInPrefix[start] == end - start &&
           IsDigit(JoinedText[start]) &&
           IsDigit(JoinedText[end]);
}

bool TLanguageModelForSplitting::IsQueryWord(int start, int end) const {
    return WordIndex[start] == WordIndex[end] &&
           (start == 0 || WordIndex[start - 1] != WordIndex[start]) &&
           (end == N - 1 || WordIndex[end + 1] != WordIndex[end]);
};
