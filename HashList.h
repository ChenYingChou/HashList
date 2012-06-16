// HashList.h
// vim: set ts=4 sw=4 et:

#ifndef HashList_H_
#define HashList_H_ 1

#include <string>
#include <limits>
#include <stdarg.h>
#include <stdexcept>
#include <assert.h>

size_t Primes[] = {
    127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 131071,
    262139, 524287, 1048573, 2097143, 4194301, 8388593, 16777213, 33554393,
    67108859, 134217689, 268435399, 536870909, 1073741789, 2147483647
};

extern "C" size_t ToPrime(const int Value)
{
    for (int i = 0; i < sizeof(Primes)/sizeof(size_t); i++) {
        size_t Result = Primes[i];
        if (Value <= Result) return Result;
    }
    return (Value + Value / 2) | 1;
}

std::string Format(const char *fmt, ...)
{
    char *s;
    va_list ap;

    va_start(ap, fmt);
    if (vasprintf(&s, fmt, ap) == -1) {
        std::string Result(fmt);
        Result.append(" -- Out of memory.");
        return Result;
    }
    va_end(ap);

    std::string Result(s);
    free(s);

    return Result;
}

//--------------------------------------------------------------------
#if 0       // compiler fails for GCC 3.4.6

namespace std {

const // It is a const object...
class nullptr_t 
{
  public:
    template<class T>
    inline operator T*() const // convertible to any type of null non-member pointer...
    { return 0; }
 
    template<class C, class T>
    inline operator T C::*() const   // or any type of null member pointer...
    { return 0; }
 
  private:
    void operator&() const;  // Can't take address of nullptr
 
} nullptr = {};

}

#else

  #define nullptr       NULL

#endif
//--------------------------------------------------------------------

namespace tony {

using namespace std;

//typedef unsigned int uint;

template <typename _Tp>
_Tp Empty_()
{
    static _Tp _EmptyItem;
    return _EmptyItem;
}

template <>
char* Empty_<char*>()
{
    return nullptr;
}

template <typename T>
struct TBucket {
    TBucket*    Link;       // Single-Linked List
    TBucket*    Next;       // Double-Linked List: Next^
    TBucket*    Prev;       // Double-Linked List: Prev^
    size_t      HitCount;
    string      Key;
    T           Value;
    void SetValue(const T& V) { Value = V; }
    void ClearValue() { Value = Empty_<T>(); }
};

// Default disable support THashList<char*> to auto allocate memory
#if defined(SUPPORT_HASHLIST_CHARPTR_STRDUP)
#warning THashList<char*>::operator[] LHS may be leak memory!
template <>
struct TBucket<char*> {
    TBucket*    Link;
    TBucket*    Next;
    TBucket*    Prev;
    size_t      HitCount;
    string      Key;
    char*       Value;
    ~TBucket() { 
        if (Value != nullptr) free(Value);
    }
    void SetValue(const char* V) {
        Value = (V == nullptr || *V == 0) ? nullptr : strdup(V);
    }
    void ClearValue() {
        if (Value != nullptr) free(Value);
        Value = nullptr;
    }
};
#endif

template <>
struct TBucket<string> {
    TBucket*    Link;
    TBucket*    Next;
    TBucket*    Prev;
    size_t      HitCount;
    string      Key;
    string      Value;
    void SetValue(const string& V) { Value = V; }
    void ClearValue() { Value.clear(); }
};

template <typename _Tp>
class THashList {
    public:
        bool MRUFirst;      // Most-Recently-Used: moved to front of each HashList[]

        THashList(size_t HashSize=0, size_t LimitCount=0);
        THashList(const THashList& Source);
        ~THashList();
        void Assign(THashList& Source);
        void Clear() { ReleaseList(false); }
        void RemoveUseless();
        void GetStatistics(double& density, double& AvgDeeps, int& MaxDeeps) const;
        bool Add(const string& Key, const _Tp& Value);
        bool Delete(const string& Key);
        bool Delete(int Index) { return Delete(GetBucket(Index)->Key); }
        bool Find(const string& Key);
        bool Find(const string& Key, _Tp& Value);
        int IndexOf(const string& Key);
        bool Resize(size_t HashSize);
        const string Keys(int Index) { return GetBucket(Index)->Key; }
        const _Tp Values(int Index) { return GetBucket(Index)->Value; }
        size_t Count() const { return FCount; }
        size_t LimitCount() const { return FLimitCount; }
        size_t HashSize() const { return FHashSize; }
        
        // c++11 compatiable
        THashList& operator=(const THashList& Source);
        _Tp& operator[](const string& Key);
        void clear() { Clear(); }
        bool empty() const { return size() == 0; }
        bool rehash(size_t HashSize) { return Resize(HashSize); }
        size_t bucket_count() const { return FHashSize; }
        size_t size() const { return FCount; }
        double load_factor() const;
        double max_load_factor() const { return FMaxLoadFactor; }
        void max_load_factor(double factor, double avgDeeps=0, int maxDeeps=0);
    protected:
        virtual size_t HashKey(const string& Key) const;
    private:
        typedef TBucket<_Tp>*   PBucket;
        typedef PBucket*        ZBucket;

        ZBucket FList;          // HashList: PBucket[] (array of Single-Linked list)
        PBucket FFree;          // FreeList: TBucket List (Single-Linked list)
        PBucket FActive;        // ActiveList: TBucket Double-Linked list (Prev/Next)
        size_t  FHashSize;      // length of HashList[]
        size_t  FCount;         // length of ActiveList
        size_t  FLimitCount;    // when FCount > FLimitCount then RemoveUseless()
        size_t  FBucketLoad;    // Count of HashList[] <> nullptr

        int     FLastIndex;     // Cache for IndexOf ...
        PBucket FLastBucket;

        // Resize/rehash hints
        double  FMaxLoadFactor; // 0 ~ 1: zero means no auto-resize
        int     FMaxBucketLoad; // = (int)(FHashSize * FMaxLoadFactor)
        double  FAvgDeeps;      // used by Add()
        int     FMaxDeeps;      // used by !Find0() to set FOverMaxDeeps
        bool    FOverMaxDeeps;  // set by !Find0() and used by Add()

        void ReleaseBucket(PBucket Bucket);
        void ReleaseList(bool FreeNow=true);
        bool Find0(const string& Key, size_t& nth, PBucket& Last, PBucket& Curr);
        //PBucket NewBucket();
        //PBucket GetBucket(const int Index);

PBucket NewBucket()
{
    // FreeList: Single-Linked list
    PBucket Curr = FFree;
    if (Curr == nullptr) {
        Curr = new TBucket<_Tp>();
    } else {
        FFree = Curr->Link;
    }

    // ActiveList: Double-Linked list
    if (FActive == nullptr) {
        // First bucket
        FActive = Curr;
        Curr->Next = Curr;
        Curr->Prev = Curr;
    } else {
        // Append Curr to last of ActiveList
        PBucket Head = FActive;
        PBucket Tail = Head->Prev;
        Curr->Next = Head;
        Curr->Prev = Tail;
        Tail->Next = Curr;
        Head->Prev = Curr;
    }
    FCount++;

    return Curr;
}

PBucket GetBucket(const int Index)
{
    int n;

    if (Index < 0 || Index >= FCount) {
        throw new runtime_error(Format("THashList.GetBucket> Index out of bounds (%d).",Index));
    }

    // FActive: ActiveList is Double-Linked list
    PBucket Result;
    if (FLastIndex >= 0) {
        // using caches?
        Result = FLastBucket;
        n = Index - FLastIndex;
        if (n == 0) return Result;

        int absN = abs(n);
        if (absN <= Index && absN <= (FCount-Index)) {
            // Forward via caches
            while (n > 0) {
                Result = Result->Next;
                n--;
            }
            // Backward via cache
            while (n < 0) {
                Result = Result->Prev;
                n++;
            }
            // Caches result
            FLastIndex = Index;
            FLastBucket = Result;
            return Result;
        }
    }

    Result = FActive;
    if (Index <= FCount / 2) {
        // Forward from Root(FActive)
        for (n = 1; n <= Index; n++) {
            Result = Result->Next;
        }
    } else {
        // Backward from Root(FActive)
        for (n = FCount-Index; n >= 1; n--) {
            Result = Result->Prev;
        }
    }

    // Caches result
    FLastIndex = Index;
    FLastBucket = Result;
    return Result;
}

};  // class THashList {...}

//==========================================================
// THashList -- Implement
//==========================================================

const int RESERVED_BUCKET_SIZE = 64;

template <typename _Tp>
THashList<_Tp>::THashList(size_t HashSize, size_t LimitCount)
    :   FHashSize(ToPrime(HashSize)),
        FLimitCount(LimitCount)
{
    FList = new PBucket[FHashSize];
    memset(FList,0,FHashSize*sizeof(PBucket));
    FBucketLoad = 0;

    FFree = nullptr;
    FActive = nullptr;
    FCount = 0;

    MRUFirst = false;
    FMaxLoadFactor = 1;
    FMaxBucketLoad = FHashSize;
    FMaxDeeps = std::numeric_limits<int>::max();
    FAvgDeeps = FMaxDeeps;

    // Caches
    FLastIndex = -1;
    FLastBucket = nullptr;
}

template <typename _Tp>
THashList<_Tp>::THashList(const THashList& Source)
    :   FHashSize(Source.FHashSize),
        FLimitCount(Source.FLimitCount)
{
    FList = new PBucket[FHashSize];
    memset(FList,0,FHashSize*sizeof(PBucket));
    FBucketLoad = 0;

    FFree = nullptr;
    FActive = nullptr;
    FCount = 0;

    Assign(Source);     // Assign will clear caches
}

template <typename _Tp>
THashList<_Tp>& THashList<_Tp>::operator=(const THashList& Source)
{
    if (this != &Source) {
        Clear();
        FLimitCount = Source.FLimitCount;
        Resize(Source.FHashSize);
        Assign(Source);
    }
    return *this;
}

template <typename _Tp>
THashList<_Tp>::~THashList()
{
    ReleaseList();

    // Free FreeList
    PBucket Bucket = FFree;
    while (Bucket != nullptr) {
        PBucket P = Bucket;
        Bucket = Bucket->Link;
        delete P;
    }

    delete[] FList;
}

// Return true if add success, false if Key alreay exists!
template <typename _Tp>
bool THashList<_Tp>::Add(const string& Key, const _Tp& Value)
{
    PBucket Curr, Last;
    size_t nth;
    bool Result = !Find0(Key,nth,Last,Curr);
    if (Result) {
        if (FLimitCount > 0 && FCount >= FLimitCount) {
            // Remove useless which hit-counter is smallest
            RemoveUseless();
        }

        PBucket Bucket = NewBucket();
        if (Last == nullptr) {
            // First bucket for FList[nth]
            FList[nth] = Bucket;
            FBucketLoad++;
            Bucket->Link = nullptr;
        } else {
            // Create Single-Linked list
            Bucket->Link = Curr;    // Curr == NULL
            Last->Link = Bucket;
        }

        Bucket->Key = Key;
        Bucket->SetValue(Value);
        Bucket->HitCount = 0;

        // Check resize hints
        if (FMaxBucketLoad > 0 && (FOverMaxDeeps ||
            FBucketLoad >= FMaxBucketLoad || FCount > FBucketLoad*FAvgDeeps)) {
            Resize(FHashSize+31);   // a number >= 1 to force expanding hash size
        }
    }

    return Result;
}

template <typename _Tp>
void THashList<_Tp>::Assign(THashList& Source)
{
    MRUFirst = Source.MRUFirst;
    FMaxLoadFactor = Source.FMaxLoadFactor;
    FMaxBucketLoad = (int)(FHashSize * FMaxLoadFactor);
    FAvgDeeps = Source.FAvgDeeps;
    FMaxDeeps = Source.FMaxDeeps;

    Clear();
    PBucket Bucket = Source.FActive;
    for (int Index=Source.Count(); --Index >= 0;) {
        Add(Bucket->Key,Bucket->Value);
        Bucket = Bucket->Next;
    }
}

template <typename _Tp>
bool THashList<_Tp>::Delete(const string& Key)
{
    PBucket Curr, Last;
    size_t nth;
    bool Result = Find0(Key,nth,Last,Curr);
    if (Result) {
        PBucket Next = Curr->Link;
        if (Last == nullptr) {
            // Root for FList[nth]
            FList[nth] = Next;
            if (Next == nullptr) FBucketLoad--;
        } else {
            // Remove sigle link: (Last)->(Curr)->(Next) ==> (Last)->(Next)
            Last->Link = Next;
        }

        ReleaseBucket(Curr);
        FLastIndex = -1;
        FLastBucket = nullptr;
    }

    return Result;
}

template <typename _Tp>
bool THashList<_Tp>::Find0(const string& Key, size_t& nth, PBucket& Last, PBucket& Curr)
{
    Last = nullptr;
    nth = HashKey(Key) % FHashSize;

    int Deeps = 0;
    PBucket Bucket = FList[nth];
    if (Bucket != nullptr) {
        // HashList: FList[] Single-Linked list
        do {
            if (Bucket->Key == Key) {
                // Found -> true
                Curr = Bucket;
                ++(Bucket->HitCount);

                if (MRUFirst && Last != nullptr && Bucket->HitCount > Last->HitCount) {
                    // Move to front of FList[nth]: -> [First] -> ... -> [Last] -> [Bucket] -> ...
                    //                                ^                               |
                    //                                |-------------------------------+
                    Last->Link = Bucket->Link;
                    Bucket->Link = FList[nth];
                    FList[nth] = Bucket;
                    Last = nullptr;
                }
                
                return true;
            }
            Last = Bucket;
            Bucket = Bucket->Link;
            Deeps++;
        } while (Bucket != nullptr);
    }

    // Not found -> false
    Curr = nullptr;
    FOverMaxDeeps = (Deeps > FMaxDeeps);
    return false;
}

template <typename _Tp>
bool THashList<_Tp>::Find(const string& Key)
{
    PBucket Curr, Last;
    size_t nth;
    return Find0(Key,nth,Last,Curr);
}

template <typename _Tp>
bool THashList<_Tp>::Find(const string& Key, _Tp& Value)
{
    PBucket Curr, Last;
    size_t nth;
    bool Result = Find0(Key,nth,Last,Curr);
    if (Result) {
        Value = Curr->Value;
    }
    return Result;
}

template <typename _Tp>
_Tp& THashList<_Tp>::operator[](const string& Key)
{
    PBucket Curr, Last;
    size_t nth;
    if (Find0(Key,nth,Last,Curr)) return Curr->Value;

    _Tp EMPTY = Empty_<_Tp>();
    Add(Key,EMPTY);
    return Find0(Key,nth,Last,Curr) ? Curr->Value : EMPTY;
}

template <typename _Tp>
void THashList<_Tp>::max_load_factor(double factor, double avgDeeps, int maxDeeps)
{
    FMaxLoadFactor = factor;
    FMaxBucketLoad = (int)(FHashSize * FMaxLoadFactor);
    if (avgDeeps > 0) FAvgDeeps = avgDeeps;
    if (maxDeeps > 0) FMaxDeeps = maxDeeps;
}

template <typename _Tp>
double THashList<_Tp>::load_factor() const
{
    // c++11: The average number of elements per bucket.
    //return (double)FCount / FHashSize;

    // Density of FHastList[]
    return (double)FBucketLoad / FHashSize;
}

template <typename _Tp>
void THashList<_Tp>::GetStatistics(double& density, double& AvgDeeps, int& MaxDeeps) const
{
    int nActiveBuckets = 0;
    int nMaxDeeps = 0;
    for (int i = 0; i < FHashSize; i++) {
        PBucket Bucket = FList[i];
        if (Bucket != nullptr) {
            int nDeeps = 0;
            do {
                nDeeps++;
                Bucket = Bucket->Link;
            } while (Bucket != nullptr);
            if (nDeeps > nMaxDeeps) nMaxDeeps = nDeeps;
            nActiveBuckets += nDeeps;
        }
    }
    assert(nActiveBuckets == FCount);

    if (nActiveBuckets > 0) {
        density = (double)FBucketLoad / FHashSize;
        AvgDeeps = (double)FCount / FBucketLoad;
        MaxDeeps = nMaxDeeps;
    } else {
        density = 0;
        AvgDeeps = 0;
        MaxDeeps = 0;
    }
}

template <typename _Tp>
size_t THashList<_Tp>::HashKey(const string& Key) const
{
    int size = Key.size();
    const char* buf = reinterpret_cast<const char*>(Key.data());
    size_t Result = 2166136261U;
    for (int i = 0; i < size; i++) {
        //Result = 31 * (Result + buf[i]);
        //Result = 33 * (Result + buf[i]);
        //Result = (16777619 * Result) + buf[i];
        //Result = 16777619 * (Result ^ static_cast<size_t>(buf[i]));
        Result = 16777619 * (Result + static_cast<size_t>(buf[i]));
    }
    return Result;
}

template <typename _Tp>
int THashList<_Tp>::IndexOf(const string& Key)
{
    PBucket Curr, Last;
    size_t nth;
    if (Find0(Key,nth,Last,Curr)) {
        FLastBucket = Curr;
        int Result = 0;
        while (Curr != FActive) {
            Curr = Curr->Prev;
            Result++;
            if (Result >= FCount) {
                throw new runtime_error(Format("THashList.IndexOf(%s)> Internal error -- Invalid bucket for this key.",Key.c_str()));
            }
        }
        FLastIndex = Result;
        return Result;
    } else {
        return -1;
    }
}

template <typename _Tp>
void THashList<_Tp>::ReleaseBucket(PBucket Bucket)
{
    if (Bucket == nullptr) return;

    PBucket Next = Bucket->Next;
    if (Bucket == Next) {
        // Only one
        FActive = nullptr;
        FCount = 0;
    } else {
        PBucket Prev = Bucket->Prev;
        Next->Prev = Prev;
        Prev->Next = Next;
        if (FActive == Bucket) FActive = Next;
        FCount--;
    }

    // Move Bucket to FreeList or free now
    PBucket LastFree = FFree;
    int nCount = (LastFree == nullptr ? 0 : LastFree->HitCount);
    if (nCount >= RESERVED_BUCKET_SIZE) {
        // Keep only RESERVED_BUCKET_SIZE buckets in FreeList, otherwise free it now
        delete Bucket;
    } else {
        // Insert Bucket at front of FreeList
        Bucket->Link = LastFree;
        Bucket->Prev = nullptr;
        Bucket->Next = nullptr;
        Bucket->HitCount = nCount+1;    // Counter of FreeList
        Bucket->Key.clear();
        Bucket->ClearValue();
    }
    FFree = Bucket;
}

template <typename _Tp>
void THashList<_Tp>::ReleaseList(bool FreeNow)
{
    FLastIndex = -1;
    FLastBucket = nullptr;
    if (FActive == nullptr) return;

    if (FreeNow) {
        // ActiveList: Double-Linked list
        PBucket Curr = FActive;
        do {
            PBucket Next = Curr->Next;
            delete Curr;
            Curr = Next;
        } while (Curr != FActive);

        FActive = nullptr;
        FCount = 0;
    } else {
        while (FActive != nullptr) {
            ReleaseBucket(FActive);
        }
    }

    // Clear HashList[]
    memset(FList,0,FHashSize*sizeof(PBucket));
    FBucketLoad = 0;
}

template <typename _Tp>
void THashList<_Tp>::RemoveUseless()
{
    if (FActive != nullptr) {
        PBucket Target = FActive;
        if (Target != Target->Next) {
            // Find the useless -- which HitCount is smallest
            size_t Cnt = Target->HitCount;
            PBucket Bucket = Target->Next;
            while (Bucket != FActive) {
                size_t N = Bucket->HitCount;
                Bucket->HitCount = N >> 1;  // Reduce counter by half
                if (N < Cnt) {
                    // So far, the smallest counter
                    Target = Bucket;
                    Cnt = N;
                }
                Bucket = Bucket->Next;
            }
        }
        Delete(Target->Key);
    }
}

template <typename _Tp>
bool THashList<_Tp>::Resize(size_t HashSize)
{
    HashSize = ToPrime(HashSize);
    if (HashSize == FHashSize) return false;

    ZBucket XList = FList;

    FHashSize = HashSize;
    FList = new PBucket[FHashSize];
    memset(FList,0,FHashSize*sizeof(PBucket));

    FBucketLoad = 0;
    FMaxBucketLoad = (int)(FHashSize * FMaxLoadFactor);

    // Move ActiveList from XList[] to FList[]
    if (FActive != nullptr) {
        PBucket Bucket = FActive;
        do {
            Bucket = Bucket->Prev;
            // Create Single-Linked list, insert into first position
            size_t nth = HashKey(Bucket->Key) % FHashSize;
            Bucket->Link = FList[nth];
            FList[nth] = Bucket;
            if (Bucket->Link == nullptr) FBucketLoad++;
        } while (Bucket != FActive);
    }

    if (XList != nullptr) delete[] XList;
    return true;
}

}   // namespace tony
#endif
