//////////////////////////////////////////////////////////////////////////////////////////////////
//
// wordbreaker2.cpp - a wordbreaker, based on Viterbi algorithm; uses bigram language model
//
// See >>> viterbi_solver_base.h <<< for details on what's going on here.

#include "viterbi_solver_base.h"

#include <dict/dictutil/format.h>
#include <util/draft/matrix.h>
#include <util/stream/output.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//                 ~~~ Module purpose is to implement this function ~~~
//                               (declared in splitter.h)

ISplitter* CreateWordbreaker2(const TWord2Grams* bigrams);

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TViterbiSolver2
//

namespace {

class TViterbiSolver2: public TViterbiSolverBase {
public:
    TViterbiSolver2(const TWord2Grams* bigrams, const VectorWStrok& words);
    void LogGreedyVariantsTo(TOutputStream* out) const;

private:
    void ComputeBestWeights();

    // Underlying algorithm outer (by end2) loop invariant is the following:
    //
    //   BestWeights[pos][K - 1] is the weight of the best phrase, that a prefix JoinedText[0:pos]
    //   can be split into, given the last word of that phrase is K characters long
    //   (i.e. is JoinedText[pos-K:pos])
    //
    TMatrix<double> BestWeights;

    // One more outer loop invariant: BestTwoWeightsAtPos[pos] is correct for any pos < end2, i.e.
    // contains minumum and second best minimum among
    //
    //   1) BestWeights[pos][0..L-1] -- weights of all possible "correct" splits (where last word is
    //      no longer than MAX_WORD_LENGTH);
    //   2) Weights of splits done with safety split points.
    //
    // ...and Arg[Second]Best is [the length of the last word - 1] in a corresponding split.
};

// Algorithm /////////////////////////////////////////////////////////////////////////////////////

TViterbiSolver2::TViterbiSolver2(const TWord2Grams* bigrams, const VectorWStrok& words)
    : TViterbiSolverBase(bigrams, words)
    , BestWeights(N, L)
{
    ComputeBestWeights();
}

void TViterbiSolver2::ComputeBestWeights() {
    BestWeights.Fill(INF);

    // Fill in unigram weights
    //
    for (int end1 = 0; end1 < L; ++end1) {
        BestWeights[end1][end1] = Model.GetWordWeight(0, end1);
        BestTwoWeightsAtPos[end1].Push(BestWeights[end1][end1], end1);
        if (SplitMandatoryAfter[end1])
            break;
    }

    // Safety split points are introduced to deal with long numbers: they allow to look more than
    // MAX_WORD_LENGTH chars back: to the first position, where split was allowed.
    //
    int safetySplitPosition = 0;

    // Proceed to bigrams
    //
    for (int end2 = 1; end2 < N; ++end2) {
        if (!SplitAllowedAfter[end2])
            continue;

        for (int start2 = end2; start2 >= Max(1, end2 + 1 - L); --start2) {
            if (!SplitAllowedAfter[start2 - 1])
                continue;

            int length2 = end2 - start2 + 1;

            // Normal splits
            //
            for (int start1 = start2 - 1; start1 >= Max(0, start2 - L); --start1) {
                if (start1 > 0 && !SplitAllowedAfter[start1 - 1])
                    continue;

                int length1 = start2 - start1;
                double prevPhraseWeight = BestWeights[start2 - 1][length1 - 1];

                // Our goal is to get final variant weight, that is less than or equal to
                // WeightToGiveUp. If prevPhraseWeight is already larger, we can stop
                // building current variant and go try smth. else.
                //
                if (prevPhraseWeight > WeightToGiveUp)
                    continue;

                double lastWordWeight = Model.GetConditionalWordWeight(start1, start2, end2);
                BestWeights[end2][length2 - 1] = Min(BestWeights[end2][length2 - 1],
                                                     prevPhraseWeight + lastWordWeight);

                if (start1 > 0 && SplitMandatoryAfter[start1 - 1])
                    break;
            } // start1

            // Do a safety split of the first word
            //
            if (BestWeights[end2][length2 - 1] == INF) {
                BestWeights[end2][length2 - 1] =
                    BestTwoWeightsAtPos[start2 - 1].Best + Model.GetWordWeight(start2, end2);
            }
            BestTwoWeightsAtPos[end2].Push(BestWeights[end2][length2 - 1], length2 - 1);

            if (SplitMandatoryAfter[start2 - 1])
                break;
        } // start2

        // Do a safety split of the second word
        //
        if (BestTwoWeightsAtPos[end2].ArgBest < 0) {
            double backoff = 0.0;
            if (safetySplitPosition > 0)
                backoff = Model.GetParameters().BigramToUnigramBackoff;

            double safetyWeight = BestTwoWeightsAtPos[end2].Best +
                                  Model.GetWordWeight(safetySplitPosition, end2) +
                                  backoff;

            BestTwoWeightsAtPos[end2].Push(safetyWeight, end2 - safetySplitPosition);
        }

        // Safety split position is the last position, where split was allowed
        //
        safetySplitPosition = end2 + 1;
    } // end2
}

// Logging ///////////////////////////////////////////////////////////////////////////////////////

void TViterbiSolver2::LogGreedyVariantsTo(TOutputStream* out) const {
    // Result for a query will look like this:
    //
    // -- example start ---------------------------------------------------------
    //  H            i            j            a            c            k
    // *13.41(H)     21.04(i)     28.97(j)     36.41(a)     36.87(c)     48.65(k)
    //  1e+12       *15.78(H)     34.97(i)     33.04(j)     41.49(a)     42.66(c)
    //  1e+12        1e+12       *26.05(H)     38.47(i)    *35.98(j)     45.90(a)
    //  1e+12        1e+12        1e+12       *25.25(H)     10013(i)    !23.56(j)
    //  1e+12        1e+12        1e+12        1e+12        10000(H)     10013(i)
    //  1e+12        1e+12        1e+12        1e+12        1e+12       *22.23(H)
    //
    // BEST GREEDY VARIANT (greedy: 22.23; trigram: 22.23):
    //  Hijack
    //
    // SECOND BEST GREEDY VARIANT (greedy: +1.330; trigram: 23.56):
    //  Hi
    //  jack
    //
    //
    // ----------------------------------------------------------- example end --

    TOutputStream& log = *out;

    // Get variants; it's fairly cheap, once weights are computed.
    //
    VectorWStrok greedyVariant = GetBestGreedyVariant(N);
    VectorWStrok secondGreedyVariant = GetSecondBestGreedyVariant(greedyVariant);

    // Determine detour position
    //
    int detourPos = -1;
    int detourLastWordLength = -1;
    double detourPenalty = -1;

    if (!secondGreedyVariant.empty()) {
        assert(secondGreedyVariant != greedyVariant);

        detourPos = N - 1;
        int word1 = greedyVariant.size() - 1;
        int word2 = secondGreedyVariant.size() - 1;

        while (greedyVariant[word1] == secondGreedyVariant[word2]) {
            detourPos -= greedyVariant[word1].length();
            --word1;
            --word2;
        }

        detourLastWordLength = secondGreedyVariant[word2].length();
        detourPenalty = BestTwoWeightsAtPos[detourPos].SecondBest -
                        BestTwoWeightsAtPos[detourPos].Best;
    }

    // Write JoinedText' letters at the top
    //
    for (int pos = 0; pos < N; ++pos) {
        // Each JoinedText position takes 13 chars of width on output:
        // 1 for marker (see below), 5 for numeric value, 3 for corresponding letter and
        // parentheses, 4 spaces.
        //
        log << " " << JoinedText.substr(pos, 1) << "           ";
    }
    log << "\n";

    // Write weights and corresponding letters
    //
    for (int length = 1; length <= L; ++length) {
        for (int pos = 0; pos < N; ++pos) {          // 'length' is row index, 'pos' is column index

            double weight = BestWeights[pos][length - 1];

            // Print marker: "*" if weight is the best possible for current column
            //               "!" for the detour of the second-best variant
            const char* marker = " ";
            if (weight == BestTwoWeightsAtPos[pos].Best)
                marker = "*";
            else if (pos == detourPos && length == detourLastWordLength)
                marker = "!";

            log << marker;

            // Weight itself
            //
            log << PadL(Fmt6(weight), 5);

            // Print corresponding letter
            //
            int letterPos = pos - length + 1;
            if (letterPos >= 0)
                log << "(" << JoinedText.substr(letterPos, 1) << ")";
            else
                log << "   ";

            // Space
            //
            log << "    ";
        }
        log << "\n";
    }
    log << "\n";

    // Write best split result
    //
    double weight1 = Model.GetPhraseWeightWithTrigrams(greedyVariant);
    log << "BEST GREEDY VARIANT (greedy: " << Fmt6(BestTwoWeightsAtPos[N-1].Best) << "; "
                            << "trigram: " << Fmt6(weight1) << "):\n";
    for (int i = 0; i < (int)greedyVariant.size(); ++i)
        log << " " << greedyVariant[i] << "\n";

    // Write second best result
    //
    if (!secondGreedyVariant.empty()) {
        double weight2 = Model.GetPhraseWeightWithTrigrams(secondGreedyVariant);
        log << "\nSECOND BEST GREEDY VARIANT (greedy: +" << Fmt6(detourPenalty) << "; "
                                          << "trigram: " << Fmt6(weight2) << "):\n";
        for (int i = 0; i < (int)secondGreedyVariant.size(); ++i)
                log << " " << secondGreedyVariant[i] << "\n";
    }

    // Vertical space for the next query
    //
    log << "\n\n";
    log << Flush;
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// CreateWordbreaker2
//

ISplitter* CreateWordbreaker2(const TWord2Grams* bigrams) {
    return new TWordbreakerT<TViterbiSolver2>(bigrams);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Wordbreaker2SplitAndLog
//

// Used in >> junk/esgv/splitter/test_wb2/main.cpp <<
// Not present in any .h-file in accordance with Occam principle.
//
void Wordbreaker2SplitAndLog(const TWord2Grams* bigrams,
                             const VectorWStrok& queryWords,
                             TOutputStream* log)
{
    TViterbiSolver2 actualSplitter(bigrams, queryWords);
    actualSplitter.LogGreedyVariantsTo(log);
}
