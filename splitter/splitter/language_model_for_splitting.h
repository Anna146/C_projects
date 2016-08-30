#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// language_model_for_splitting.h
//
// Declares TLanguageModelForSplitting, that answers language model GetWeight and
// GetConditionalWeight qureies, but ones in a form, specific for splitting problem: input phrase is
// specified by indices in the joined text, rather than by VectorWStrok.
// A separate TLanguageModelForSplitting should be constructed for every incoming split query, and
// constructor parameters are bigrams, some tweaks and a text, from which subwords, specified by
// indices, will be taken.
//
// For implementation conventions (specifically, string position conventions) see
// >>> viterbi_solver_base.h <<<

#include <dict/dictutil/dictutil.h>
#include <util/draft/matrix.h>

class TWord2Grams;

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TLanguageModelForSplitting
//

// Answers language model-ish GetWordWeight and GetCondinonalWordWeight queries; but those queries
// come in form, specific for splitting task.
//
// WARNING: if the word is longer than MaxWordLength, it will be assigned a weight (or conditional
// weight), equal to OutOfVocabularyWeight regardless of what bigrams tell.
//
// With queries having this specific form (a pair of indices for each word), we can efficiently
// cache unigram frequencies (those also give us knowledge of whether a certain subword is OOV - out
// of the vocabulary), and quickly back off to unigrams in case one of the two words in a bigram
// is OOV. Since a significant part of substrings of a query are OOV words, that gives us
// performance gain.
//
// Along with unigram frequencies, we can also cache unigram hints. Hints are magic pointers, that
// allow us to do a quick start, when looking up a bigram.
//
class TLanguageModelForSplitting {
    DECLARE_NOCOPY(TLanguageModelForSplitting);

// Model parameters //////////////////////////////////////////////////////////////////////////////

public:
    struct TParameters {
        // Length of the longest word, which will be queried from model.
        // If set to 0, it will be reset to JoinedText.length().
        //
        // If the word is longer than MaxWordLength, it will be assigned a weight (or conditional
        // weight), equal to OutOfVocabularyWeight regardless of what bigrams tell.
        //
        int MaxWordLength;

        // A weight, assigned to an OOV word or bigram (if no backoffs are active).
        //
        double OutOfVocabularyWeight;

        // When DoNGramBackoff is true && bigram <W1, W2> is OOV, a weight
        // "weight(W2) + BigramToUnigramBackoff" is returned as conditional weight(W2 | W1).
        // Similarly for trigrams and bigrams.
        //
        bool DoNGramBackoff;
        double BigramToUnigramBackoff;
        double TrigramToBigramBackoff;

        // If a number (dots in the middle are allowed) is OOV && DoNumberToLengthBackoff is true &&
        // number's length is greater than or equal to MinNumberLengthToBackOff, then the number
        // gets weight, equal to NumberToLengthBackoff + log2(10) * Length.
        //
        // Works only for numbers, not for all words.
        //
        bool DoNumberToLengthBackoff;
        int MinNumberLengthToBackOff;
        double NumberToLengthBackoff;

        // If a subword is OOV, but it corresponds to a whole word in original query (and
        // DoBackoffForQueryWords is on), we consider subword "not so bad", and it gets weight,
        // equal to OOVQueryWordWeight instead of OutOfVocabularyWeight.
        //
        // EXAMPLE:
        //   Consider a query "Knights who say ni"
        //
        //   Joined text: k  n  i  g  h  t  s  w  h  o  s  a  y  n  i
        //   Index:       0  1  2  3  4  5  6  7  8  9  10 11 12 13 14
        //
        //   - Subword at [1,5] ("night") DOES NOT correspond to a query word;
        //   - Subword at [13,14] ("ni") DOES correspond to a query word;
        //   - Subword at [1,2] ("ni" again) DOES NOT correspond to a query word.
        //
        bool DoBackoffForQueryWords;
        double OOVQueryWordWeight;

        // MULTIPLE BACKOFFS:
        //   In a query
        //
        //      число 420984029285104720987498297849 очень длинное
        //
        //   a subword "420984029285104720987498297849" is both a number and a word of original
        //   query, so it is good for "number to length" backoff, as well as for "query word"
        //   backoff. The general rule is that any subword gets the smallest possible weight, i.e.
        //   "420984029285104720987498297849" will get weight, equal to
        //
        //      Min(OutOfVocabularyWeight,
        //          OOVQueryWordWeight,
        //          NumberToLengthBackoff + log2(10) * 30 (its length))
    };

// Model itself //////////////////////////////////////////////////////////////////////////////////

public:
    TLanguageModelForSplitting(const TWord2Grams* bigrams,
                               const VectorWStrok& query,
                               const TParameters& parameters);
    const TParameters& GetParameters() const { return Parameters; }
    WStroka WordAt(int start, int end) const;

    // WARNING: if the word is longer than MaxWordLength, it will be assigned a weight (or
    // conditional weight), received from GetWeightForOOVWord regardless of what bigrams tell.

    double GetWordWeight(int start, int end) const;

    // start1 = "start of the first word", start2 = "start of the second word",
    // end2 = "end of the second word". Alike variable names are used further in the code.
    //
    double GetConditionalWordWeight(int start1, int start2, int end2) const;
    double GetConditionalWordWeight(int start1, int start2, int start3, int end3) const;

    // Join("", phrase) should be equal to JoinedText
    //
    double GetPhraseWeightWithBigrams(const VectorWStrok& phrase) const;

    // Calls GetPhraseWeightWithBigrams if order of n-grams, from which model was constructed,
    // is less than 3
    //
    double GetPhraseWeightWithTrigrams(const VectorWStrok& phrase) const;

private:
    // Get key, equivalent to Bigrams->MakeKey(WordAt(start, end)), but do it faster - using
    // KeyForJoinedText.substr
    //
    Stroka KeyForWordAt(int start, int end) const;

    void CacheUnigram(int start, int end);         // Memorize unigram frequency and hint.
    bool IsOOV(int start, int end) const;
    bool BeginsNoBigram(int start, int end) const; // Let W be the word at [start, end];
                                                   // Returns true, if no bigram <W, W'>
                                                   // exist in the model, for any W'.

    // This function can work with words, longer than MaxWordLength (in some cases):
    // if DoNumberToLengthBackoff is on, and the subword [start, end] is a number, then
    // a number backoff will be applied, despite the length of the word is greater, than
    // MaxWordLength
    //
    double GetWeightForOOVWord(int start, int end) const;

    bool IsNumber(int start, int end) const;       // Returns, whether subword is a number, and
                                                   // we should do special backoff for it.

    bool IsQueryWord(int start, int end) const;    // Returns, whether subword corresponds to a
                                                   // word of original query, and we should do
                                                   // special backoff for it

private:
    const TWord2Grams* Bigrams;
    WStroka JoinedText;
    Stroka KeyForJoinedText;
    int N; // length of JoinedText
    TParameters Parameters;

    i64 NumUnigramsInModel;
    TMatrix<ui32> CachedUnigramFreqs;
    TMatrix<const void*> CachedUnigramHints;

    // Used to quickly determine, whether a subword is a number.
    // Contains cumulative sums, here's an example:
    //
    //   JoinedText:                          0 0 1 0 1 0 1 д в о и ч н ы й к о д
    //   IsNumberCharacter(JoinedText[pos]):  1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0
    //   NumDigitsInPrefix:                   1 2 3 4 5 6 7 7 7 7 7 7 7 7 7 7 7 7
    //
    // Despite a name says "number of digits", it's actually a "number of 'number characters'"
    //
    NStl::vector<int> NumDigitsInPrefix;

    // Used to quickly determine, whether a subword is a word of original query.
    // If original query was "0010101 двоичный код", then:
    //                                        0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 2 2 2
    NStl::vector<int> WordIndex;
};
