#include "standard_splitter.h"
#include <dict/dictutil/db.h>
#include <dict/dictutil/exceptions.h>
#include <dict/dictutil/str.h>
#include <dict/libs/langmodels/word_2grams.h>
#include <util/generic/ymath.h>

////////////////////////////////////////////////////////////////////////////////
//
// StandardSplitter.Request
//

static const double MAX_LOG_P = 100000;

class TStandardSplitter::TRequest : public ISplitResultsIterator {
    DECLARE_NOCOPY(TRequest);

// Constructor/Destructor
public:
    TRequest(const TStandardSplitter* parent);
    ~TRequest();

// ISplitResultsIterator methods
public:
    virtual bool Next(VectorWStrok* result);

// Methods
public:
    void Run(const VectorWStrok& words, int maxVariants);

// Helper methods
private:
    bool Init(const VectorWStrok& words);
    void Prepare();
    void SetBreaks();
    void SetPoints();
    void Forward();
    void Backward();
    void BuildResult();

// Types
private:
    enum {
        MAX_W_LEN = 25 // = "человеконенавистнического" (must be <= 31)
    };

    struct TPoint {
        struct TRef {
            int    Len; // word length (back from this point)
            double LogP; // probability
        };

        int    Index; // word number (0-based)
        int    Off;   // word offset
        ui32   Mask;  // mask of possible breaks
        int    ForwardRef;
        double P;
        i32    Freq; // freq for best variant
        int    RefCount;
        TRef   Refs[3]; // back refs

        void InitRef(int len, double logP = MAX_LOG_P) {
            SetRef(0, len, logP);
        }

        void SetRef(int index, int len, double logP) {
            assert(index < ARRAY_SSIZE(Refs));

            if (RefCount > 0 && Refs[0].LogP > MAX_LOG_P - 1) {
                RefCount = 0;
                index = 0;
            }
            if (RefCount >= ARRAY_SSIZE(Refs))
                --RefCount;
            memmove(Refs + index + 1, Refs + index, (RefCount - index) * sizeof(TRef));
            Refs[index].Len = len;
            Refs[index].LogP = logP;
            ++RefCount;
        }
    };

// Fields
private:
    const TStandardSplitter* This;
    TPoint* Points;
    WStroka Text;
    int N; // Text.length()
    VectorWStrok Words;
    VectorWStrok Result;
    bool Failed; // "true" if "best" variant can't be found
};

TStandardSplitter::TRequest::TRequest(const TStandardSplitter* parent)
  : This(parent)
  , Points(0)
  , Text(), N(0)
  , Words(), Result()
  , Failed(false)
{}

TStandardSplitter::TRequest::~TRequest() {
    delete [] Points;
}

bool TStandardSplitter::TRequest::Next(VectorWStrok* result) {
    *result = Result;
    Result.clear();
    return !result->empty();
}

void TStandardSplitter::TRequest::Run(const VectorWStrok& words, int /* maxVariants */) {
    if (!Init(words))
        return;

    Prepare();
    Forward();
    Backward();
    BuildResult();
}

bool TStandardSplitter::TRequest::Init(const VectorWStrok& words) {
    Words = words;
    Text = Join(S::Empty, Words);
    N = (int)Text.length(); // text length (without spaces)
    Points = new TPoint[N + 1];
    memset(Points, 0, (N + 1) * sizeof(TPoint));
    for (int i = 0; i <= N; ++i)
        Points[i].InitRef(0);
    return true;
}

void TStandardSplitter::TRequest::Prepare() {
    SetBreaks();
    SetPoints();
}

void TStandardSplitter::TRequest::SetBreaks() {
    // Search substrings in 2-grams
    std::auto_ptr<IDbRecordSet> recordSet(This->BiGrams->GetRecordSet());
    Stroka textKey = This->BiGrams->MakeKey(Text);
    for (int i = 0; i < N; ++i) {
        Stroka key = textKey.substr(i);
        recordSet->Seek(key, DBSEEK_BEFORE_EQ);
        Stroka found;
        recordSet->GetKey(found);
        int len = (int)CommonPrefix(key, found);
        len = Min(len, (int)MAX_W_LEN);
        for (int k = 1; k <= len; ++k)
            Points[i + k].Mask |= (1 << k);
    }
}

void TStandardSplitter::TRequest::SetPoints() {
    // Preprocessing: fill "Points" array
    bool prevDigit = false;
    bool prevNumber = false;
    Points[0].InitRef(0, 0.0);
    int prevBreak = 0;
    for (int i = 0, pos = 0; i < (int)Words.size(); ++i) {
        WStroka w = Words[i];
        // TODO: prefer breaks on word boundaries (breaks[pos] = BREAK_GOOD)
        Points[pos + w.length()].InitRef((int)w.length());

        bool isNumber = IsNumber(w);
        if (prevNumber && isNumber)
            prevBreak = pos;
        for (int j = 0; j < (int)w.length(); ++j, ++pos) {
            bool isDigit = IsDigit(w[j]);
            if (prevDigit && isDigit) {
                if (j != 0) // not 1-st character of word?
                    Points[pos].Mask = 0; // break not possible here
            }
            Points[pos].Index = i;
            Points[pos].Off = j;
            // do not join 2 numbers:
            // in "canon ip 4200 15 раз мигает" do not join "4200+15"
            if (pos + 1 - prevBreak < 31) // 31 = 8 * sizeof(ui32) - 1
                Points[pos + 1].Mask &= ((1 << (pos + 2 - prevBreak)) - 1);
            prevDigit = isDigit;
        }
        prevNumber = isNumber;
    }
}

void TStandardSplitter::TRequest::Forward() {
    const double maxFreq = double(This->MaxFreq); // TODO: common constant
    // Check split/join variants by 2grams
    int indexJ[32];
    for (int i = 1; i <= N; ++i) {
        TPoint* ptI = &Points[i];
        ui32 mask = ptI->Mask;
        mask >>= 1;

        int countJ = 0;
        for (int j = i - 1; mask; --j, mask >>= 1) {
            if ((mask & 1) == 0)
                continue;
            indexJ[countJ++] = j;
        }

        // Check all indexJ positions, starting from MINIMAL indexJ value.
        // It's important due to "couldn't increase probability" logic below.
        while (countJ) {
            int j = indexJ[--countJ];
            WStroka word(Text, j, i - j);
            i32 formFreq = This->BiGrams->Freq(0, word);
            if (formFreq <= This->MinFreq)
                continue;
            double logP = Log2(maxFreq / formFreq);
            logP += Points[j].Refs[0].LogP + logP;
            int count = ptI->RefCount;
            if (!count)
                count = 1;
            int k = 0;
            for (k = 0; k < count; ++k) {
                if (logP < ptI->Refs[k].LogP)
                    break;
            }
            if (k == count) {
                // couldn't increase probability. add as alt-variant?
                if (ptI->Freq >= This->GoodFreq)
                    continue; // for "good" words we do not add alt-variants
                if (k == ARRAY_SIZE(ptI->Refs))
                    continue; // array is full (no space to add to)
                const TPoint* ptJ = &Points[j];
                if (formFreq <= ptI->Freq || ptJ->Freq <= ptI->Freq)
                    continue;
            }
            if (k == 0)
                ptI->Freq = formFreq;
            ptI->SetRef(k, (int)word.length(), logP);
        }
    }
}

void TStandardSplitter::TRequest::Backward() {
    const double maxFreq = double(This->MaxFreq);
    Points[N].P = 1.0;
    Points[N].ForwardRef = N;
    int minPos = N;
    double maxP = Points[N].P;
    for (int i = N; i != 0;) {
        TPoint* ptI = &Points[i];
        if (ptI->P < maxP) // probability is worse already
            ptI->RefCount = 0; // no chance to improve -> clear Refs
        for (int k = 0; k < ptI->RefCount; ++k) {
            int len = ptI->Refs[k].Len;
            int j = i - len;
            TPoint* ptJ = &Points[j];
            WStroka word(Text, j, len);
            WStroka next(Text, i, ptI->ForwardRef);
            double p = 0.0;
            const void* wordContext = 0;
            const i32 wordFreq = This->BiGrams->Freq(0, word, &wordContext);
            if (wordContext && !next.empty() && ptI->P != 1.0) {
                i32 frNext = This->BiGrams->Freq(0, next);
                if (frNext != 0) {
                    p = (double)This->BiGrams->Freq(wordContext, next) / frNext;
                }
            }
            if (p == 0)
                p = Max(This->MinFreq, wordFreq) / maxFreq;
            p *= ptI->P;
            if (ptJ->ForwardRef == 0 || p > ptJ->P) {
                ptJ->ForwardRef = len;
                ptJ->P = p;
                if (j <= minPos) {
                    minPos = j;
                    maxP = ptJ->P;
                }
            }
        }
        for (--i, --ptI; i > minPos; --i, --ptI) {
            if (ptI->ForwardRef != 0)
                break;
        }
        if (i == minPos)
            ptI->P = maxP = 1.0;
    }
}

void TStandardSplitter::TRequest::BuildResult() {
    if (Failed)
        return;

    // Build final result
    bool changed = false;
    for (int i = 0; i != N; ) {
        const TPoint* pt = &Points[i];
        int len = pt->ForwardRef;
        if (len == 0 || i + len > N) {
            assert(false);
            Result.clear();
            return;
        }

        WStroka word(Text, i, len);
        Result.push_back(word);
        if (Points[i].Off != 0)
            changed = true;

        i += len;
    }
    if (Result.size() != Words.size())
        changed = true;
    if (!changed)
        Result.clear();
}

////////////////////////////////////////////////////////////////////////////////
//
// StandardSplitter
//

ISplitter* CreateStandardSplitter(const TWord2Grams* bigrams) {
    return new TStandardSplitter(bigrams);
}

TStandardSplitter::TStandardSplitter(const TWord2Grams* biGrams)
  : BiGrams(biGrams)
  , MaxFreq(0), MinFreq(0), GoodFreq(0)
{
    CHECK_ARG_NULL(biGrams, "biGrams");
    MaxFreq = BiGrams->MaxFreq();
    MinFreq = i32(MaxFreq >> 27);
    GoodFreq = i32(MaxFreq >> 15);
}

ISplitResultsIterator*
TStandardSplitter::Split(const VectorWStrok& words, int maxVariants) const {
    CHECK_ARG(maxVariants >= 0, "maxVariants");

    std::auto_ptr<TRequest> request(new TRequest(this));
    request->Run(words, maxVariants);
    return request.release();
}
