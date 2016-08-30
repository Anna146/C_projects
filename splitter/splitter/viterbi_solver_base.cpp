//////////////////////////////////////////////////////////////////////////////////////////////////
//
// viterbi_solver_base.cpp - a base for TViterbiSolver2 and TViterbiSolver3
//
// See >>> viterbi_solver_base.h <<< for details on what's going on here.

#include "viterbi_solver_base.h"

#include <dict/dictutil/exceptions.h>
#include <dict/dictutil/str.h>
#include <util/generic/algorithm.h>
#include <iomanip>
#include <sstream>

using NStl::swap;

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tweaks
//

// Words in resulting split will be no longer than this.
// Numbers in resulting split will be no longer than this, if possible (see 'Splitting'
// tweaks section below).
//
static const int MAX_WORD_LENGTH = 27;

// Language model parameters /////////////////////////////////////////////////////////////////////

// See comments in TLanguageModelForSplitting::TParameters

static const double OUT_OF_VOCABULARY_WEIGHT = 10000;

static const bool DO_NGRAM_BACKOFF = true;
static const double BIGRAM_TO_UNIGRAM_BACKOFF = 0.0;
static const double TRIGRAM_TO_BIGRAM_BACKOFF = 0.0;

static const bool DO_NUMBER_TO_LENGTH_BACKOFF = true;
static const double NUMBER_TO_LENGTH_BACKOFF = 10.0;
static const int MIN_NUMBER_LENGTH_TO_BACK_OFF = 1;

static const bool DO_BACKOFF_FOR_QUERY_WORDS = true;
static const double OOV_QUERY_WORD_WEIGHT = 500.0;

// Splitting /////////////////////////////////////////////////////////////////////////////////////

// Forbid breaks between digits in the result: all adjacent numbers in incoming query
// will be joined into one long number in the result, if this is set to true.
//
static const bool DONT_SPLIT_NUMBERS = true;

// Force a break between two words, first one ending in a digit, second one
// starting in a digit (in original query).
//
// Overpowers DONT_SPLIT_NUMBERS: if
//
//    DONT_SPLIT_NUMBERS == true;
//    DONT_JOIN_NUMBERS == true,
//
// then TViterbiSolverBase won't touch numbers in the query at all: they will be included in the
// result as-is.
//
static const bool DONT_JOIN_NUMBERS = true;

// If the second best variant's weight by the trigram model will be greater than the best variant's
// weight + SECOND_BEST_HANDICAP, then second best variant will not be written to splitResults (to
// save performance of the query optimizer, that works after splitter).
//
static const double SECOND_BEST_HANDICAP = 6.0;

// Viterbi solvers expect, that the weight of the worst generated variant will be no larger than the
// weight of the original query plus this constant.
//
// WARNING: be careful not to set this very low, when DONT_SPLIT_NUMBERS == true and
// DONT_JOIN_NUMBERS == false. See also TViterbiSolverBase comments.
//
static const double WEIGHT_HANDICAP = 0.0 + SECOND_BEST_HANDICAP;

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TViterbiSolverBase
//

// Construction //////////////////////////////////////////////////////////////////////////////////

const double TViterbiSolverBase::INF = 1e+12;

static const TLanguageModelForSplitting::TParameters MODEL_PARAMETERS = {
    MAX_WORD_LENGTH,
    OUT_OF_VOCABULARY_WEIGHT,
    DO_NGRAM_BACKOFF,
    BIGRAM_TO_UNIGRAM_BACKOFF,
    TRIGRAM_TO_BIGRAM_BACKOFF,
    DO_NUMBER_TO_LENGTH_BACKOFF,
    MIN_NUMBER_LENGTH_TO_BACK_OFF,
    NUMBER_TO_LENGTH_BACKOFF,
    DO_BACKOFF_FOR_QUERY_WORDS,
    OOV_QUERY_WORD_WEIGHT
};

TViterbiSolverBase::TViterbiSolverBase(const TWord2Grams* bigrams, const VectorWStrok& words)
    : Query(words)
    , JoinedText(Join(S::Empty, words))
    , N(JoinedText.length())
    , L(Min(N, MAX_WORD_LENGTH))
    , Model(bigrams, words, MODEL_PARAMETERS)
    , WeightToGiveUp(Model.GetPhraseWeightWithBigrams(words) + WEIGHT_HANDICAP)
    , SplitAllowedAfter(), SplitMandatoryAfter()
    , BestTwoWeightsAtPos(N)
{
    FillForcedAndForbiddenBreakPositions();
}

void TViterbiSolverBase::FillForcedAndForbiddenBreakPositions() {
    SplitAllowedAfter.resize(N, true);
    SplitMandatoryAfter.resize(N, false);

    if (DONT_SPLIT_NUMBERS) {
        // Forbid breaks between digits
        //
        for (int pos = 1; pos < N; ++pos) {
            SplitAllowedAfter[pos - 1] &= !(IsDigit(JoinedText[pos]) &&
                                            IsDigit(JoinedText[pos - 1]));
        }
    }

    if (DONT_JOIN_NUMBERS) {
        // Original query must be respected to a certain extent.
        // Force breaks between a word, ending in a digit and a word, starting in a digit
        // (in original query), if DONT_JOIN_NUMBERS is true.
        //
        int totalLength = (int)Query[0].length();
        for (int i = 1; i < (int)Query.size(); ++i) {
            if (DONT_JOIN_NUMBERS) {
                SplitMandatoryAfter[totalLength - 1] |= IsDigit(Query[i][0]) &&
                                                        IsDigit(Query[i - 1].back());
            }
            totalLength += (int)Query[i].length();
        }
    }

    // Enforce, that SplitMandatoryAfter[pos] implies SplitAllowedAfter[pos]
    //
    for (int pos = 0; pos < N; ++pos)
        SplitAllowedAfter[pos] |= SplitMandatoryAfter[pos];
}

// Generating variants ///////////////////////////////////////////////////////////////////////////

NStl::vector<VectorWStrok> TViterbiSolverBase::GetResults(int maxVariants) const {
    // Generate both variants
    //
    VectorWStrok variant1 = GetBestGreedyVariant(N);
    VectorWStrok variant2 = GetSecondBestGreedyVariant(variant1);

    // Evaluate both variants by the trigram model
    //
    double weight1 = Model.GetPhraseWeightWithTrigrams(variant1);
    double weight2 = OUT_OF_VOCABULARY_WEIGHT;

    if (!variant2.empty()) {
        weight2 = Model.GetPhraseWeightWithTrigrams(variant2);
        if (weight2 < weight1) {
            variant1.swap(variant2);
            swap(weight1, weight2);
        }
    }

    // Write results
    //
    NStl::vector<VectorWStrok> results;
    results.clear();
    results.push_back(variant1);
    if (maxVariants >= 2 && (weight2 - weight1) < SECOND_BEST_HANDICAP)
        results.push_back(variant2);

    return results;
}

VectorWStrok TViterbiSolverBase::GetBestGreedyVariant(int prefixLen) const {
    VectorWStrok result;

    int end = prefixLen - 1;
    while (end >= 0) {
        int bestLength = BestTwoWeightsAtPos[end].ArgBest + 1;
        assert(bestLength > 0);
        result.push_back(Model.WordAt(end - bestLength + 1, end));
        end -= bestLength;
    }
    assert(end == -1);

    NStl::reverse(result.begin(), result.end());
    return result;
}

VectorWStrok TViterbiSolverBase::
GetSecondBestGreedyVariant(const VectorWStrok& bestGreedyVariant) const {
    assert(Join(S::Empty, bestGreedyVariant) == JoinedText);

    VectorWStrok result;

    // Find the best detour
    //
    double bestDetourPenalty = Max<double>();
    int bestDetourPos = -1;
    int bestDetourWordNumber = -1;

    int totalLength = 0;
    for (size_t word = 0; word < bestGreedyVariant.size(); ++word) {

        totalLength += bestGreedyVariant[word].length();
        int pos = totalLength - 1;

        if (BestTwoWeightsAtPos[pos].ArgSecondBest == -1)
            continue;

        double detourPenalty = BestTwoWeightsAtPos[pos].SecondBest -
                               BestTwoWeightsAtPos[pos].Best;

        if (detourPenalty < bestDetourPenalty) {
            bestDetourPenalty = detourPenalty;
            bestDetourPos = pos;
            bestDetourWordNumber = word;
        }
    }

    if (bestDetourPos == -1)
        return VectorWStrok();

    // Write out best detour in reverse order.
    // 1) Common part
    //
    for (int i = (int)bestGreedyVariant.size() - 1; i > bestDetourWordNumber; --i)
        result.push_back(bestGreedyVariant[i]);

    // 2) Detour word
    //
    int detourWordLen = BestTwoWeightsAtPos[bestDetourPos].ArgSecondBest + 1;
    result.push_back(Model.WordAt(bestDetourPos - detourWordLen + 1, bestDetourPos));

    // 3) Prefix of detour: a best greedy path ending at the beginning of detour word
    //
    int detourPrefixLen = bestDetourPos - detourWordLen + 1;
    if (detourPrefixLen > 0) {
        VectorWStrok detourPrefix = GetBestGreedyVariant(detourPrefixLen);
        for (int i = (int)detourPrefix.size() - 1; i >= 0; --i)
            result.push_back(detourPrefix[i]);
    }

    NStl::reverse(result.begin(), result.end());
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TBestTwoWeights
//

TViterbiSolverBase::TBestTwoWeights::TBestTwoWeights()
    : Best(-1)
    , SecondBest(-1)
    , ArgBest(-1)
    , ArgSecondBest(-1)
{}

void TViterbiSolverBase::TBestTwoWeights::Push(double weight, int position) {
    if (weight < Best || ArgBest < 0) {
        SecondBest = Best;
        ArgSecondBest = ArgBest;
        Best = weight;
        ArgBest = position;
    }
    else if (weight < SecondBest || ArgSecondBest < 0) {
        SecondBest = weight;
        ArgSecondBest = position;
    }
}
