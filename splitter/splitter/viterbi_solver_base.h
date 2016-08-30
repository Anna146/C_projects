#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////////
//
// viterbi_solver_base.h - a base for TViterbiSolver2 and TViterbiSolver3
//
// Establishes an algorithmical base for viterbi solvers, which are the main components of
// TWordbreaker2 and TWordbreaker3. Solvers split a phrase in optimal way using Viterbi algorithm.
//
// In comments below, "viterbi solvers" means "TViterbiSolver2 and TViterbiSolver3", not
// "viterbi solvers in general" :)

// VITERBI SOLVERS BEHAVIOUR
//
//   1) Solvers are based on TWordbreaker from >>> dict/proof/wordbreaker/lib/wordbreaker.h <<<.
//      Unlike TWordbreaker, viterbi solvers limit the length of a single word in a phrase,
//      reducing execution time on long queries to an acceptable amount.
//
//      TWordbreaker{2,3} works in O(N * MAX_WORD_LENGTH^{2,3}) time.
//      TWordbreaker works in O(N^3) time.
//
//      MAX_WORD_LENGTH and other tweaking constants are defined in viterbi_solver_base.cpp.
//
//   2) Some heuristics are implemented to improve splitting quality (eg. backoff for numbers and
//      restrictions on possible split variants - see viterbi_solver_base.cpp for details).
//
//   3) Currently, solvers use TLanguageModelForSplitting as language model (which is based on
//      raw 2-grams, like TWordbreaker, or raw 3-grams; that's one more reason for names being
//      the way they are: "2" or "3" in "TWordbreaker{2,3}"  mean "bigrams" or "trigrams"
//      respectively).

// VITERBI SOLVERS IMPLEMENTATION
//
// 1) Viterbi solvers share a common base - TViterbiSolverBase, that contains an infrastructure,
//    useful for both TViterbiSolver2 and TViterbiSolver3: query without spaces (JoinedText),
//    its length (N), constructed instance of TLanguageModelForSplitting (Model), SaveResults
//    method, etc. Solvers follow the given scenario:
//
//    a) Inherit publicly from TViterbiSolverBase and gain access to the "infrastructure".
//
//    b) On construction, solvers compute (by their respective models) for each prefix of
//       JoinedText, which minimal weight would it have as a phrase, if you split it properly, and
//       store results in BestTwoWeightsAtPos (an array of TBestTwoWeights).
//
//       BestTwoWeightsAtPos[pos].Arg[Second]Best should be the length of the last word in the
//       (second) best corresponding split minus 1.
//
//    c) When someone calls SaveResults on TViterbiSolver{2,3}, TViterbiSolverBase uses
//       BestTwoWeightsAtPos, filled by descendant on construction to greedily generate two (or one)
//       split variants.
//
//    d) CreateWordbreaker{2,3} create an instance of TWordbreakerT<TViterbiSolver{2,3}>.
//       TWordbreakerT is a template wrapper (implementing ISplitter), that constructs specified
//       solver for every incoming query.
//
// 2) String positions convention, common along TLanguageModelForSplitting and viterbi solvers:
//
//    - Positions are 0-based indices.
//    - When cutting subwords out of the joined query, starting and ending positions are inclusive:
//      S[start:end] is a string consisting of S[start], S[start+1], ..., S[end-1], S[end].
//
// 3) "Weight" means "weight by the bigram model" for TViterbiSolver2 and "weight by the trigram
//    model" for TViterbiSolver3, until explicitly noted otherwise.

// HEURISTICS SUMMARY
//
// 1) Word length in resulting split is limited.
// 2) A break doesn't occur between two adjacent digits if they are within the same word in original
//    query, and a join of two digits is not considered, if they were in different words in the
//    original query.
// 3) Weights are computed with Viterbi algorithm, but answer is written out greedily.
// 4) First, both greedy variants are generated (it's cheap), then thay are evaluated by trigram
//    model, and (if only one variant is queried) only the best by the trigram model is returned -
//    yielding both generated variants to query optimizer is not so cheap.
// 5) If second variant's weight by the trigram model is significantly worse, than weight of
//    the first variant, it's not written out at all.
// 6) Safety splits are done for long numbers.
// 7) Language model backoffs are made for OOV numbers and OOV words, that correspond to words in
//    original query.

#include "splitter.h"
#include "language_model_for_splitting.h"
#include "result_iterators.h"

#include <dict/dictutil/dictutil.h>
#include <dict/dictutil/exceptions.h>
#include <util/generic/stroka.h>
#include <util/generic/vector.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TWordbreakerT
//

// A thin wrapper, that constructs TViterbiSolver for every incoming query.
// Instantiate with TViterbiSolver2 or TViterbiSolver3.
//
template <typename TViterbiSolver>
class TWordbreakerT: public ISplitter {
public:
    explicit TWordbreakerT(const TWord2Grams* bigrams);

// ISplitter Methods
public:
    //  WARNING: solver can generate only one or two variants, so maxVariants > 2 is meaningless
    //
    virtual ISplitResultsIterator* Split(const VectorWStrok& queryWords, int maxVariants) const;

private:
    // No copies allowed
    //
    TWordbreakerT<TViterbiSolver>& operator =(const TWordbreakerT<TViterbiSolver>&);

    const TWord2Grams* Bigrams;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TViterbiSolverBase
//

// USAGE: a recipy given above, at the "viterbi solvers implementation" remark.
// Inherit properly, construct inherited class from bigrams and a vector of words, then call
// SaveResults().
//
// All the heavy weight-counting and model-querying work is done in the constructor (that way, we
// don't need a state machine: at the moment SaveResults are called, all the necessary weights are
// computed).
//
// IMPLEMENTATION DETAILS:
//   1) Every solver first looks for the best split variant. One split variant is already known:
//      it's the initial query. Query's weight + WEIGHT_HANDICAP is computed and stored into
//      WeightToGiveUp field, and when Viterbi algorithm dynamically looks through variants, it can
//      skip some splits, that won't give a result, better than original (splits with a beginning
//      bad enough it already has greater weight).
//
//      This might impose a threat of losing the best variant at all, in case, eg., when
//      DONT_SPLIT_NUMBERS is set to true, DONT_JOIN_NUMBERS is set to false, and
//      original query contains numbers. In that case, solvers (should) operate in such way, that
//      variants that don't satisfy given constraints, are not considered as a split result, and
//      weights for them are not computed (i.e. outer loop invariant doesn't hold in some places).
//      Variants with weight, larger than original query's weight plus WEIGHT_HANDICAP, are also
//      ignored. These two constraints together might result in a situation, when TViterbiSolverBase
//      doesn't properly compute a weight for ANY of the variants; in such situation, result will
//      be undefined (but still a valid split of the original query).
//
//      A safety split points mechanism guarantees an acceptable result in such cases. Such
//      mechanism is implemented in TViterbiSolver2 (not in TViterbiSolver3).
//
//   2) There are two boolean arrays: SplitAllowedAfter[pos] and SplitMandatoryAfter[pos].
//      If SplitAllowedAfter[p] is false, then a break after the position p will (should) not occur
//      in the result (most likely; see note 1). If SplitMandatoryAfter[p] is true, a break after
//      the position p will (should) most definitely be in the result.
//
//      SplitMandatoryAfter implies (i.e. overpowers) SplitAllowedAfter.
//
//   3) TViterbiSolverBase descendants compute weights in accordance with bigram or trigram model,
//      but TViterbiSolverBase writes split result out _greedily_: first it finds the best word to
//      cut off the end of the JoinedText, and cuts it off. Then it finds the next best candidate to
//      cut off the end of what's left (without looking at the last word), and so on. Described
//      greedy manner, strictly speaking, is not a correct Viterbi algorithm, but it worked better
//      in practice (on data, that was available at the moment).
//
class TViterbiSolverBase {
    DECLARE_NOCOPY(TViterbiSolverBase);

// Public interface
public:
    // WARNING: solver can generate only one or two variants, so maxVariants > 2 is meaningless
    //
    NStl::vector<VectorWStrok> GetResults(int maxVariants) const;

// Types
protected:
    // A structure to keep track on the two smallest weights, that had ever been fed
    // to Push() method.
    //
    struct TBestTwoWeights {
        TBestTwoWeights();
        void Push(double weight, int position);

        double Best;
        double SecondBest;
        int ArgBest;
        int ArgSecondBest;
    };

// Helper methods
private:
    // Fill SplitAllowedAfter and SplitMandatoryAfter, based on DONT_SPLIT_NUMBERS and
    // DONT_JOIN_NUMBERS tweaks (see >>> viterbi_solver_base.cpp <<<).
    //
    void FillForcedAndForbiddenBreakPositions();

// Protected interface
protected:
    TViterbiSolverBase(const TWord2Grams* bigrams, const VectorWStrok& words);

    // Generates the best (greedy) split for JoinedText[0..prefixLen-1]
    //
    VectorWStrok GetBestGreedyVariant(int prefixLen) const;

    // Requires the best greedy variant, generated via GetBestGreedyVariant(N) - to find
    // the best detour with the least penalty
    //
    VectorWStrok GetSecondBestGreedyVariant(const VectorWStrok& bestGreedyVariant) const;

protected:
    // All these fields should be used by the descendants
    // in their quest for the holy grail^U to fill BestTwoWeightsAtPos

    VectorWStrok Query;
    WStroka JoinedText;
    int N; // length of JoinedText
    int L; // Min(N, MAX_WORD_LENGTH). Used instead of MAX_WORD_LENGTH inside TViterbiSolverBase.

    TLanguageModelForSplitting Model;
    double WeightToGiveUp;

    NStl::vector<bool> SplitAllowedAfter;
    NStl::vector<bool> SplitMandatoryAfter;

    // The main field, which descendants should fill at construction.
    //
    // - After TViterbiSolverBase is constructed, has the length of N.
    //
    // - Since viterbi solvers are expected to have an ability to generate two variants,
    //   not only the weight of the best split variant for every prefix should be filled, but
    //   also the weight of the second best split variant. Viterbi algorithm allows to do this
    //   easily.
    //
    NStl::vector<TBestTwoWeights> BestTwoWeightsAtPos;

protected:
    static const double INF; // a constant for representing infinite weight
};

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TWordbreakerT implementation
//

template <typename TViterbiSolver>
TWordbreakerT<TViterbiSolver>::TWordbreakerT(const TWord2Grams* bigrams)
    : Bigrams(bigrams)
{
    CHECK_ARG_NULL(bigrams, "bigrams");
}

template <typename TViterbiSolver>
ISplitResultsIterator*
TWordbreakerT<TViterbiSolver>::Split(const VectorWStrok& queryWords, int maxVariants) const {
    if (queryWords.size() == 0)
        return YieldNoResults();

    for (size_t i = 0; i < queryWords.size(); ++i)
        CHECK_ARG(queryWords[i].length() > 0, "words");
    CHECK_ARG(maxVariants > 0, "maxVariants");

    TViterbiSolver actualSplitter(Bigrams, queryWords);
    ISplitResultsIterator* allResults = YieldFromVector(actualSplitter.GetResults(maxVariants));
    return FilterBadVariants(NStl::auto_ptr<ISplitResultsIterator>(allResults), queryWords);
}
