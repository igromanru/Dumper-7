#pragma once
#include "Enums.h"

#include <cassert>


static constexpr uint8_t FiveBitPermutation[32] = {
    0x20, 0x1D, 0x08, 0x18, 0x06, 0x0F, 0x15, 0x19,
    0x13, 0x1F, 0x17, 0x10, 0x14, 0x0C, 0x0E, 0x02,
    0x16, 0x11, 0x12, 0x0A, 0x0B, 0x1E, 0x04, 0x07,
    0x1B, 0x03, 0x0D, 0x1A, 0x1C, 0x01, 0x09, 0x05,
};

inline uint8 SmallPearsonHash(const char* StringToHash)
{
    uint8 Hash = 0;

    while (*StringToHash != '\0')
    {
        const uint8 MaskedDownChar = (*StringToHash - 'A') & 0b11111;
        Hash = FiveBitPermutation[Hash ^ MaskedDownChar];
        StringToHash++;
    }

    return (Hash & 0b11111);
}


#pragma pack(0x1)
class StringEntry
{
public:
    static constexpr int32 MaxStringLength = 512;

    static constexpr int32 StringEntrySizeWithoutStr = 0x3;

private:
    friend class HashStringTable;
    friend class StringTableTest;

    template<typename CharType>
    friend int32 Strcmp(const CharType* String, const StringEntry& Entry);

private:
    // Length of object name
    uint16 Length : 9;

    // If this string uses char or wchar_t
    uint16 bIsWide : 1;

    // If this name is unique amongst others --- always true if RawNameLength > 0
    mutable uint16 bIsUnique : 1;

    // PearsonHash reduced to 5 bits --- only computed if string was added when StringTable::CurrentMode == EDuplicationCheckingMode::Check
    uint16 Hash : 5;

    // Length of RawName to be appended to 'Legth' if this is a FullName, zero if this name is edited (eg. "UEditTool3Plus3")
    uint8 RawNameLength;

    union
    {
        // null-terminated
        const char Char[MaxStringLength];
        const wchar_t WChar[MaxStringLength];
    };

private:
    inline int32 GetLengthBytes() const { return StringEntrySizeWithoutStr + Length + RawNameLength; }

public:
    inline bool IsUnique() const { return bIsUnique; }
    inline bool IsPrefixedValidName() const { return RawNameLength == 0; }

    inline uint8 GetHash() const { return Hash; }

    inline std::string_view GetRawName() const { return Char + Length; }
    inline std::wstring_view GetWideRawName() const { return WChar + Length; }

    inline std::string_view GetUniqueName() const { return Char; }
    inline std::wstring_view GetWideUniqueName() const { return WChar; }

    inline std::string_view GetFullName() const { return Char; }
    inline std::wstring_view GetWideFullName() const { return WChar; }
};

template<typename CharType>
inline int32 Strcmp(const CharType* String, const StringEntry& Entry)
{
    if constexpr (std::is_same_v<CharType, char>)
    {
        return strcmp(Entry.Char, String);
    }
    else
    {
        return wcscmp(Entry.WChar, String);
    }
}

struct HashStringTableIndex
{
    int32 bIsChecked : 1;
    int32 HashIndex : 5;
    int32 InBlockOffset : 26;

    static inline HashStringTableIndex FromInt(int32 Idx)
    {
        return *reinterpret_cast<HashStringTableIndex*>(Idx);
    }

    explicit inline operator int32()
    {
        return *reinterpret_cast<int32*>(this);
    }
};

class HashStringTable
{
private:
    static constexpr int64 NumHashBits = 5;
    static constexpr int64 NumBuckets = 1 << NumHashBits;

    /* Checked, Unchecked */
    static constexpr int64 NumSectionsPerBucket = 2;

private:
    struct StringBucket
    {
        // One allocated block, split in two sections, checked and unchecked
        uint8* Data;
        int32 UncheckedSize;
        int32 CheckedSize;
        int32 UncheckedSizeMax;
        int32 CheckedSizeMax;
    };

private:
    StringBucket Buckets[NumBuckets];

public:
    HashStringTable() = default;

    HashStringTable(int32 PerBucketPerSectionSize)
    {
        if (PerBucketPerSectionSize < 0x5000)
            PerBucketPerSectionSize = 0x5000;

        for (int i = 0; i < NumBuckets; i++)
        {
            StringBucket& CurrentBucket = Buckets[i];
            CurrentBucket.Data = static_cast<uint8*>(malloc(PerBucketPerSectionSize * NumSectionsPerBucket));

            CurrentBucket.UncheckedSize = 0x0;
            CurrentBucket.CheckedSize = 0x0;

            CurrentBucket.UncheckedSizeMax = PerBucketPerSectionSize;
            CurrentBucket.CheckedSizeMax = PerBucketPerSectionSize;
        }
    }

    ~HashStringTable()
    {
        for (int i = 0; i < NumBuckets; i++)
        {
            StringBucket& CurrentBucket = Buckets[i];

            if (!CurrentBucket.Data)
                continue;

            free(CurrentBucket.Data);
            CurrentBucket.Data = nullptr;
        }
    }

public:
    class HashBucketIterator
    {
    private:
        const StringBucket* IteratedBucket;
        int32 InBucketIndex;

    public:
        HashBucketIterator(const StringBucket& Bucket, int32 InBucketStartPos = 0)
            : IteratedBucket(&Bucket)
            , InBucketIndex(InBucketStartPos)
        {
            if (InBucketIndex >= Bucket.UncheckedSize && InBucketIndex < Bucket.UncheckedSizeMax)
                InBucketIndex = Bucket.UncheckedSizeMax;
        }

    public:
        static inline HashBucketIterator begin(const StringBucket& Bucket) { return HashBucketIterator(Bucket, 0); }
        static inline HashBucketIterator beginChecked(const StringBucket& Bucket) { return HashBucketIterator(Bucket, Bucket.UncheckedSizeMax); }
        static inline HashBucketIterator end(const StringBucket& Bucket) { return HashBucketIterator(Bucket, Bucket.CheckedSize != 0 ? Bucket.UncheckedSizeMax + Bucket.CheckedSize : Bucket.UncheckedSize); }

    public:
        inline int32 GetInBucketIndex() const { return InBucketIndex; }
        inline const StringEntry& GetStringEntry() const { return *reinterpret_cast<StringEntry*>(IteratedBucket->Data + InBucketIndex); }

    public:
        inline bool operator==(const HashBucketIterator& Other) const { return InBucketIndex == Other.InBucketIndex; }
        inline bool operator!=(const HashBucketIterator& Other) const { return InBucketIndex != Other.InBucketIndex; }

        inline const StringEntry& operator*() const { return GetStringEntry(); }

        inline HashBucketIterator& operator++()
        {
            InBucketIndex += GetStringEntry().GetLengthBytes();

            const bool bIsBetweenUncheckedAndCheckedRegion = InBucketIndex >= IteratedBucket->UncheckedSize && InBucketIndex < IteratedBucket->UncheckedSizeMax;

            if (bIsBetweenUncheckedAndCheckedRegion)
                InBucketIndex = IteratedBucket->UncheckedSizeMax; // continue to next valid memory

            return *this;
        }
    };

    class HashStringTableIterator
    {
    private:
        const HashStringTable& IteratedTable;
        HashBucketIterator CurrentBucketIterator;
        int32 BucketIdx;

    public:
        HashStringTableIterator(const HashStringTable& Table, int32 BucketStartPos = 0, int32 InBucketStartPos = 0)
            : IteratedTable(Table)
            , CurrentBucketIterator(Table.Buckets[BucketStartPos], InBucketStartPos)
            , BucketIdx(BucketStartPos)
        {
            while (CurrentBucketIterator == HashBucketIterator::end(IteratedTable.Buckets[BucketIdx]) && (++BucketIdx < NumBuckets))
                CurrentBucketIterator = HashBucketIterator::begin(IteratedTable.Buckets[BucketIdx]);
        }

        /* Only used for 'end' */
        HashStringTableIterator(const HashStringTable& Table, HashBucketIterator LastBucketEndIterator)
            : IteratedTable(Table)
            , CurrentBucketIterator(LastBucketEndIterator)
            , BucketIdx(NumBuckets)
        {
        }

    public:
        inline int32 GetBucketIndex() const { return BucketIdx; }
        inline int32 GetInBucketIndex() const { return CurrentBucketIterator.GetInBucketIndex(); }

    public:
        inline bool operator==(const HashStringTableIterator& Other) const { return BucketIdx == Other.BucketIdx && CurrentBucketIterator == Other.CurrentBucketIterator; }
        inline bool operator!=(const HashStringTableIterator& Other) const { return BucketIdx != Other.BucketIdx || CurrentBucketIterator != Other.CurrentBucketIterator; }

        inline const StringEntry& operator*() const { return *CurrentBucketIterator; }

        inline HashStringTableIterator& operator++()
        {
            ++CurrentBucketIterator;

            while (CurrentBucketIterator == HashBucketIterator::end(IteratedTable.Buckets[BucketIdx]) && (++BucketIdx < NumBuckets))
                CurrentBucketIterator = HashBucketIterator::begin(IteratedTable.Buckets[BucketIdx]);

            return *this;
        }
    };

public:
    inline HashStringTableIterator begin() const { return HashStringTableIterator(*this, 0); }
    inline HashStringTableIterator end()   const { return HashStringTableIterator(*this, HashBucketIterator::end(Buckets[NumBuckets - 1])); }

private:
    inline bool CanFit(const StringBucket& Bucket, int32 StrLengthBytes, bool bIsChecked) const
    {
        const int32 EntryLength = StringEntry::StringEntrySizeWithoutStr + StrLengthBytes;

        if (bIsChecked)
            return (EntryLength + Bucket.CheckedSize) <= Bucket.CheckedSizeMax;

        return (EntryLength + Bucket.UncheckedSize) <= Bucket.UncheckedSizeMax;
    }

    inline StringEntry& GetRefToEmpty(const StringBucket& Bucket, bool bIsChecked)
    {
        if (bIsChecked)
            return *reinterpret_cast<StringEntry*>(Bucket.Data + Bucket.UncheckedSizeMax + Bucket.CheckedSize);

        return *reinterpret_cast<StringEntry*>(Bucket.Data + Bucket.UncheckedSize);
    }

    inline const StringEntry& GetStringEntry(const StringBucket& Bucket, int32 InBucketIndex) const
    {
        return *reinterpret_cast<StringEntry*>(Bucket.Data + InBucketIndex);
    }

    inline const StringEntry& GetStringEntry(int32 BucketIndex, int32 InBucketIndex) const
    {
        assert(BucketIndex < 32 && "Bucket index out of range!");

        const StringBucket& Bucket = Buckets[BucketIndex];

        assert(InBucketIndex > 0 && InBucketIndex < (Bucket.UncheckedSize + Bucket.CheckedSize) && "InBucketIndex was out of range!");

        return *reinterpret_cast<StringEntry*>(Bucket.Data + InBucketIndex);
    }

    inline void ResizeBucket(StringBucket& Bucket, bool bIsChecked)
    {
        if (bIsChecked)
        {
            /* Only extend the checked part of the bucket */
            int32 NewBucketSize = Bucket.UncheckedSizeMax + (Bucket.CheckedSizeMax * 2);

            uint8_t* NewData = static_cast<uint8_t*>(realloc(Bucket.Data, NewBucketSize));

            if (NewData)
                Bucket.Data = NewData;
        }
        else
        {
            /* Only extend the unchecked part of the bucket */
            int32 OldBucketUncheckedSizeMax = Bucket.UncheckedSizeMax;
            int64 NewBucketUncheckedSizeMax = Bucket.UncheckedSizeMax * 2;

            uint8_t* NewData = static_cast<uint8_t*>(realloc(Bucket.Data, Bucket.CheckedSizeMax + NewBucketUncheckedSizeMax));

            if (NewData)
            {
                /* Move checked part of the bucket to the back so we don't write over it when adding new unchecked elements */
                Bucket.Data = NewData;
                memmove(Bucket.Data + OldBucketUncheckedSizeMax, Bucket.Data + NewBucketUncheckedSizeMax, Bucket.CheckedSize);
            }
        }
    }

    template<typename CharType>
    inline std::pair<HashStringTableIndex, bool> AddUnchecked(const CharType* Str, int32 Length, int32 RawNameLength, uint8 Hash, bool bIsChecked)
    {
        static_assert(std::is_same_v<CharType, char> || std::is_same_v<CharType, wchar_t>, "Invalid CharType! Type must be 'char' or 'wchar_t'.");

        const int32 LengthBytes = (Length + RawNameLength) * sizeof(CharType);

        StringBucket& Bucket = Buckets[Hash];

        if (!CanFit(Bucket, LengthBytes, bIsChecked))
            ResizeBucket(Bucket, bIsChecked);

        StringEntry& NewEmpty = GetRefToEmpty(Bucket, bIsChecked);

        NewEmpty.Length = Length;
        NewEmpty.RawNameLength = RawNameLength;
        NewEmpty.bIsWide = std::is_same_v<CharType, wchar_t>;
        NewEmpty.Hash = Hash;

        // Initially always true, later marked as false if duplicate is found
        NewEmpty.bIsUnique = true;

        // Always copy to the WChar, memcyp only copies bytes anways
        memcpy((void*)(NewEmpty.WChar), Str, LengthBytes);

        const int32 LengthOfNewEntry = StringEntry::StringEntrySizeWithoutStr + LengthBytes;

        HashStringTableIndex ReturnIndex;
        ReturnIndex.bIsChecked = bIsChecked;
        ReturnIndex.HashIndex = Hash;
        ReturnIndex.InBlockOffset = bIsChecked ? Bucket.CheckedSize : Bucket.UncheckedSize;

        (bIsChecked ? Bucket.CheckedSize : Bucket.UncheckedSize) += LengthOfNewEntry;

        return { ReturnIndex, true };
    }

public:
    template<typename CharType>
    inline std::pair<HashStringTableIndex, bool> FindOrAdd(const CharType* Str, int32 Length, bool bIsChecked, int32 RawNameLength = 0)
    {
        const int CombinedLength = Length + RawNameLength;
        constexpr bool bIsWChar = std::is_same_v<CharType, wchar_t>;

        static_assert(!bIsWChar, "'wchar_t' is not supported by the hashing function yet!");

        if (!Str || CombinedLength <= 0 || CombinedLength > StringEntry::MaxStringLength)
        {
            std::cout << std::format("Error on line {{{:d}}}: {}\n", __LINE__, !Str ? "!Str" : Length <= 0 ? "LengthBytes <= 0" : "LengthBytes > MaxStringLength") << std::endl;
            return { HashStringTableIndex(-1), false };
        }

        uint8 Hash = SmallPearsonHash(Str);

        if (!bIsChecked)
            return AddUnchecked(Str, Length, RawNameLength, Hash, bIsChecked);

        StringBucket& Bucket = Buckets[Hash];

        for (auto It = HashBucketIterator::beginChecked(Bucket); It != HashBucketIterator::end(Bucket); ++It)
        {
            const StringEntry& Entry = *It;

            if (Entry.Length == Length && Entry.bIsWide == bIsWChar && Strcmp(Str, Entry) == 0)
            {
                Entry.bIsUnique = false;

                HashStringTableIndex Idx;
                Idx.bIsChecked = true;
                Idx.HashIndex = Hash;
                Idx.InBlockOffset = It.GetInBucketIndex();

                return { Idx, false };
            }
        }

        // Only reached if Str wasn't found in StringTable, else entry is marked as not unique
        return AddUnchecked(Str, Length, RawNameLength, Hash, bIsChecked);
    }

    inline std::pair<HashStringTableIndex, bool> FindOrAdd(const std::string& String, bool bIsChecked = false)
    {
        size_t LastDotIdx = String.find_last_of('.');

        if (LastDotIdx == -1) {
            return FindOrAdd(String.c_str(), String.size() + 1, bIsChecked);
        }
        else {
            return FindOrAdd(String.c_str(), LastDotIdx + 1, bIsChecked, String.size() - LastDotIdx);
        }
    }
};