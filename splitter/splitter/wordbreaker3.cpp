//////////////////////////////////////////////////////////////////////////////////////////////////
//
// wordbreaker3.cpp - a wordbreaker, based on Viterbi algorithm; uses trigram language model
//
// Very similar to TViterbiSolver2/TWordbreaker2 (except trigram model); the main difference
// at the moment being is that safety split points are not implemented in TViterbiSolver3.
//
// See >>> viterbi_solver_base.h <<< for details on what's going on here.

#include "viterbi_solver_base.h"
#include <dict/dictutil/array.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//                 ~~~ Module purpose is to implement this function ~~~
//                               (declared in splitter.h)

ISplitter* CreateWordbreaker3(const TWord2Grams* bigrams);

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TViterbiSolver3
//

namespace {

class TViterbiSolver3: public TViterbiSolverBase {
    DECLARE_NOCOPY(TViterbiSolver3);

public:
    TViterbiSolver3(const TWord2Grams* bigrams, const VectorWStrok& words);

private:
    void ComputeBestWeights();

    // Underlying algorithm outer (by end3) loop invariant is the following:
    //
    //   BestWeights(pos, K1 - 1, ) is the weight of the best phrase, that a prefix JoinedText[0:pos]
    //   can be split into, given the last word of that phrase is K characters long
    //   (i.e. is JoinedText[pos-K:pos])
    //
    NDict::NUtil::TArray3D<double> BestWeights;

    // One more outer loop invariant: BestTwoWeightsAtPos[pos] is correct for any pos < end3, i.e.
    // contains minumum and second best minimum among
    //
    //   1) BestWeights(pos, 0..L-1, 0..L-1) -- weights of all possible "correct" (where last two
    //      words are no longer than MAX_WORD_LENGTH) splits with two or more words;
    //   2) Weight of the single possible correct split with one word -- JoinedText[0..pos], for
    //      pos < L;
    //   3) Weights of splits done with safety split points.
    //
    // ...and Arg[Second]Best, is [the length of the last word - 1] in a corresponding split.
};

// Implementation ////////////////////////////////////////////////////////////////////////////////

TViterbiSolver3::TViterbiSolver3(const TWord2Grams* bigrams, const VectorWStrok& words)
    : TViterbiSolverBase(bigrams, words)
    , BestWeights(N, L, L)
{
    ComputeBestWeights();
}

void TViterbiSolver3::ComputeBestWeights() {
    BestWeights.Fill(INF);

    // Fill in unigram weights
    //
    for (int end1 = 0; end1 < L; ++end1) {
        BestTwoWeightsAtPos[end1].Push(Model.GetWordWeight(0, end1), end1);
        if (SplitMandatoryAfter[end1])
            break;
    }

    // Fill in bigram weights
    //
    for (int start2 = 1; start2 < Min(L + 1, N); ++start2) {
        if (!SplitAllowedAfter[start2 - 1])
            continue;

        for (int end2 = start2; end2 < Min(start2 + L, N); ++end2) {
            if (!SplitAllowedAfter[end2])
                continue;

            int length1 = start2;
            int length2 = end2 - start2 + 1;
            double bigramWeight = Model.GetWordWeight(0, start2 - 1) +
                                  Model.GetConditionalWordWeight(0, start2, end2);
            BestWeights(end2, length2 - 1, length1 - 1) = bigramWeight;
            BestTwoWeightsAtPos[end2].Push(bigramWeight, length2 - 1);

            if (SplitMandatoryAfter[end2])
                break;
        }

        if (SplitMandatoryAfter[start2 - 1])
            break;
    }

    // Proceed to trigrams
    //
    for (int end3 = 0; end3 < N; ++end3) {
        if (!SplitAllowedAfter[end3])
            continue;

        for (int start3 = end3; start3 >= Max(1, end3 + 1 - L); --start3) {
            if (!SplitAllowedAfter[start3 - 1])
                continue;
            int length3 = end3 - start3 + 1;

            for (int start2 = start3 - 1; start2 >= Max(1, start3 - L); --start2) {
                if (!SplitAllowedAfter[start2 - 1])
                    continue;
                int length2 = start3 - start2;

                for (int start1 = start2 - 1; start1 >= Max(0, start2 - L); --start1) {
                    if (start1 > 0 && !SplitAllowedAfter[start1 - 1])
                        continue;

                    int length1 = start2 - start1;
                    double prevPhraseWeight = BestWeights(start3 - 1, length2 - 1, length1 - 1);
                    if (prevPhraseWeight > WeightToGiveUp)
                        continue;

                    double weight3 = Model.GetConditionalWordWeight(start1, start2, start3, end3);
                    BestWeights(end3, length3 - 1, length2 - 1) = Min(
                            BestWeights(end3, length3 - 1, length2 - 1),
                            prevPhraseWeight + weight3);

                    if (start1 > 0 && SplitMandatoryAfter[start1 - 1])
                        break;
                }

                BestTwoWeightsAtPos[end3].Push(BestWeights(end3, length3 - 1, length2 - 1),
                                               length3 - 1);

                if (SplitMandatoryAfter[start2 - 1])
                    break;
            }

            if (SplitMandatoryAfter[start3 - 1])
                break;
        }
    }

    // Safety measure (in case of long numbers - in absence of safety split points)
    //
    for (int pos = 0; pos < N; ++pos) {
        if (BestTwoWeightsAtPos[pos].ArgBest == -1)
            BestTwoWeightsAtPos[pos].ArgBest = pos;
    }
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// CreateWordbreaker3
//

ISplitter* CreateWordbreaker3(const TWord2Grams* bigrams) {
    return new TWordbreakerT<TViterbiSolver3>(bigrams);
}
