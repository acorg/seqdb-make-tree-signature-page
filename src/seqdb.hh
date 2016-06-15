#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <algorithm>
#include <regex>
#include <iterator>
#include <deque>

#include "messages.hh"
#include "json-struct.hh"
#include "sequence-shift.hh"
#include "amino-acids.hh"

// ----------------------------------------------------------------------

// {
//     "N": <name>,
//     "d": [<date>]
//     "C": <continent>
//     "c": <country>
//     "l": "VICTORIA",  # VICTORIA, YAMAGATA, 2009PDM, SEASONAL
//     "s": [
//         {
//             "a": <sequence-amino-acids>,
//             "c": <list of clades (str)>,
//             "g": <gene: HA|NA>,
//             "h": <list of hi-names (str)>,
//             "l": {<lab> :[<lab_id>]},
//             "n": <sequence-nucleotides>,
//             "p": <list of passages (str)>,
//             "r": <reassortant>,
//             "s": <shift (int) for aa sequence>,
//             "t": <shift (int) for nuc sequence>,
//         },
//     ],
//     "v": <virus_type>,
// }

// ----------------------------------------------------------------------

class Seqdb;
class SeqdbIterator;

// ----------------------------------------------------------------------

class SequenceNotAligned : public std::runtime_error
{
 public:
    inline SequenceNotAligned(std::string prefix) : std::runtime_error(prefix + ": sequence not aligned") {}
};

// ----------------------------------------------------------------------

class SeqdbSeq
{
 public:
    inline SeqdbSeq() : mGene("HA") {}

    inline SeqdbSeq(std::string aNucleotides, std::string aAminoAcids, std::string aGene)
        : SeqdbSeq()
        {
            mNucleotides = aNucleotides;
            mAminoAcids = aAminoAcids;
            if (!aGene.empty())
                mGene = aGene;
        }

    inline SeqdbSeq(std::string aNucleotides, std::string aGene)
        : SeqdbSeq(aNucleotides, std::string(), aGene)
        {
        }

    AlignAminoAcidsData align(bool aForce, Messages& aMessages);

      // returns if aNucleotides matches mNucleotides
    bool match_update_nucleotides(std::string aNucleotides);
    bool match_update_amino_acids(std::string aAminoAcids);
    void add_passage(std::string aPassage);
    void update_gene(std::string aGene, Messages& aMessages, bool replace_ha = false);
    void add_reassortant(std::string aReassortant);
    void add_lab_id(std::string aLab, std::string aLabId);
    void update_clades(std::string aVirusType, std::string aLineage);
    inline const std::vector<std::string>& clades() const { return mClades; }

    inline bool is_short() const { return mAminoAcids.empty() ? mNucleotides.size() < (MINIMUM_SEQUENCE_AA_LENGTH * 3) : mAminoAcids.size() < MINIMUM_SEQUENCE_AA_LENGTH; }
    inline bool translated() const { return !mAminoAcids.empty(); }
    inline bool aligned() const { return mAminoAcidsShift.aligned(); }
    inline bool matched() const { return !mHiNames.empty(); }

    inline bool has_lab(std::string aLab) const { return mLabIds.find(aLab) != mLabIds.end(); }
    inline std::string lab() const { return mLabIds.empty() ? std::string() : mLabIds.begin()->first; }
    inline std::string lab_id() const { return mLabIds.empty() ? std::string() : (mLabIds.begin()->second.empty() ? std::string() : mLabIds.begin()->second[0]); }
    inline const std::vector<std::string> cdcids() const { auto i = mLabIds.find("CDC"); return i == mLabIds.end() ? std::vector<std::string>() : i->second; }
    inline const std::vector<std::string>& passages() const { return mPassages; }
    inline std::string passage() const { return mPassages.empty() ? std::string() : mPassages[0]; }
    inline bool passage_present(std::string aPassage) const { return mPassages.empty() ? aPassage.empty() : std::find(mPassages.begin(), mPassages.end(), aPassage) != mPassages.end(); }
    inline const std::vector<std::string>& reassortant() const { return mReassortant; }
    inline std::string gene() const { return mGene; }

    inline const std::vector<std::string>& hi_names() const { return mHiNames; }
    inline std::vector<std::string>& hi_names() { return mHiNames; }
    inline void add_hi_name(std::string aHiName) { mHiNames.push_back(aHiName); }
    inline bool hi_name_present(std::string aHiName) const { return std::find(mHiNames.begin(), mHiNames.end(), aHiName) != mHiNames.end(); }

      // if aAligned && aLeftPartSize > 0 - include signal peptide and other stuff to the left from the beginning of the aligned sequence
    std::string amino_acids(bool aAligned, size_t aLeftPartSize = 0) const;
    std::string nucleotides(bool aAligned, size_t aLeftPartSize = 0) const;
    inline int amino_acids_shift() const { return mAminoAcidsShift; } // throws if sequence was not aligned
    inline int nucleotides_shift() const { return mNucleotidesShift; }  // throws if sequence was not aligned

    //   // Empty passages must not be removed! this is just for testing purposes
    // inline void remove_empty_passages()
    //     {
    //         mPassages.erase(std::remove(mPassages.begin(), mPassages.end(), std::string()), mPassages.end());
    //     }

 private:
    std::vector<std::string> mPassages;
    std::string mNucleotides;
    std::string mAminoAcids;
    Shift mNucleotidesShift;
    Shift mAminoAcidsShift;
    std::map<std::string, std::vector<std::string>> mLabIds;
    std::string mGene;
    std::vector<std::string> mHiNames;
    std::vector<std::string> mReassortant;
    std::vector<std::string> mClades;

    static inline std::string shift(std::string aSource, int aShift, char aFill)
        {
            std::string r = aSource;
            if (aShift < 0)
                r.erase(0, static_cast<size_t>(-aShift));
            else if (aShift > 0)
                r.insert(0, static_cast<size_t>(aShift), aFill);
            return r;
        }

    friend class Seqdb;
    friend class SeqdbIterator;
    friend class SeqdbIteratorBase;

    friend inline auto json_fields(SeqdbSeq& a)
        {
            return std::make_tuple(
                "p", json::field(&a.mPassages, json::output_if_not_empty),
                "n", json::field(&a.mNucleotides, json::output_if_not_empty),
                "a", json::field(&a.mAminoAcids, json::output_if_not_empty),
                "t", json::field(&a.mNucleotidesShift, &Shift::to_json, &Shift::from_json), // if mNucleotidesShift.aligned()
                "s", json::field(&a.mAminoAcidsShift, &Shift::to_json, &Shift::from_json), // if mAminoAcidsShift.aligned()
                "l", json::field(&a.mLabIds, json::output_if_not_empty),
                "g", json::field(&a.mGene, json::output_if_not_empty),
                "h", json::field(&a.mHiNames, json::output_if_not_empty),
                "r", json::field(&a.mReassortant, json::output_if_not_empty),
                "c", json::field(&a.mClades, json::output_if_not_empty)
                                   );
        }

}; // class SeqdbSeq

// ----------------------------------------------------------------------

inline std::ostream& operator<<(std::ostream& out, const SeqdbSeq& seq)
{
    return out << json::dump(seq, 0);
}

// ----------------------------------------------------------------------

class SeqdbEntry
{
 public:
    inline SeqdbEntry() {}
    inline SeqdbEntry(std::string aName) : mName(aName) {}

    inline std::string name() const { return mName; }
    inline std::string country() const { return mCountry; }
    inline void country(std::string aCountry) { mCountry = aCountry; }
    inline std::string continent() const { return mContinent; }
    inline void continent(std::string aContinent) { mContinent = aContinent; }
    inline bool empty() const { return mSeq.empty(); }

    inline std::string virus_type() const { return mVirusType; }
    inline void virus_type(std::string aVirusType) { mVirusType = aVirusType; }
    void add_date(std::string aDate);
    inline std::string date() const { return mDates.empty() ? std::string() : mDates.back(); }
    inline std::string lineage() const { return mLineage; }
    void update_lineage(std::string aLineage, Messages& aMessages);
    void update_subtype(std::string aSubtype, Messages& aMessages);
      // returns warning message or an empty string
    std::string add_or_update_sequence(std::string aSequence, std::string aPassage, std::string aReassortant, std::string aLab, std::string aLabId, std::string aGene);

    inline bool date_within_range(std::string aBegin, std::string aEnd) const
        {
            const std::string date = mDates.size() > 0 ? mDates.back() : "0000-00-00";
            return (aBegin.empty() || date >= aBegin) && (aEnd.empty() || date < aEnd);
        }

    inline void remove_short_sequences()
        {
            mSeq.erase(std::remove_if(mSeq.begin(), mSeq.end(), std::mem_fn(&SeqdbSeq::is_short)), mSeq.end());
        }

    inline void remove_not_translated_sequences()
        {
            mSeq.erase(std::remove_if(mSeq.begin(), mSeq.end(), [](auto& seq) { return !seq.translated(); }), mSeq.end());
        }

    inline const std::vector<std::string> cdcids() const
        {
            std::vector<std::string> r;
            std::for_each(mSeq.begin(), mSeq.end(), [&r](auto const & seq) {auto seq_cdcids = seq.cdcids(); r.insert(r.end(), std::make_move_iterator(seq_cdcids.begin()), std::make_move_iterator(seq_cdcids.end())); });
            return r;
        }

    inline auto begin_seq() { return mSeq.begin(); }
    inline auto end_seq() { return mSeq.end(); }
    inline auto begin_seq() const { return mSeq.begin(); }
    inline auto end_seq() const { return mSeq.end(); }

    //   // Empty passages must not be removed! this is just for testing purposes
    // inline void remove_empty_passages()
    //     {
    //         std::for_each(mSeq.begin(), mSeq.end(), std::mem_fn(&SeqdbSeq::remove_empty_passages));
    //     }

 private:
    std::string mName;
    std::string mCountry;
    std::string mContinent;
    std::vector<std::string> mDates;
    std::string mLineage;
    std::string mVirusType;
    std::vector<SeqdbSeq> mSeq;

    friend class Seqdb;
    friend class SeqdbIteratorBase;
    friend class SeqdbIterator;
    friend class ConstSeqdbIterator;

    friend inline auto json_fields(SeqdbEntry& a)
        {
            return std::make_tuple(
                "N", json::field(&a.mName, json::output_if_not_empty),
                "c", json::field(&a.mCountry, json::output_if_not_empty),
                "C", json::field(&a.mContinent, json::output_if_not_empty),
                "d", json::field(&a.mDates, json::output_if_not_empty),
                "l", json::field(&a.mLineage, json::output_if_not_empty),
                "v", json::field(&a.mVirusType, json::output_if_not_empty),
                "s", &a.mSeq
                                   );
        }

}; // class SeqdbEntry

// ----------------------------------------------------------------------

class SeqdbEntrySeq
{
 public:
    inline SeqdbEntrySeq() : mEntry(nullptr), mSeq(nullptr) {}
    inline SeqdbEntrySeq(SeqdbEntry& aEntry, SeqdbSeq& aSeq) : mEntry(&aEntry), mSeq(&aSeq) {}
    inline SeqdbEntrySeq(const SeqdbEntry& aEntry, const SeqdbSeq& aSeq) : mEntry(const_cast<SeqdbEntry*>(&aEntry)), mSeq(const_cast<SeqdbSeq*>(&aSeq)) {}

    inline void assign(SeqdbEntry& aEntry, SeqdbSeq& aSeq) { mEntry = &aEntry, mSeq = &aSeq; }
    inline void assign(const SeqdbEntry& aEntry, const SeqdbSeq& aSeq) { mEntry = const_cast<SeqdbEntry*>(&aEntry), mSeq = const_cast<SeqdbSeq*>(&aSeq); }

    inline operator bool() const { return mEntry != nullptr && mSeq != nullptr; }

    inline SeqdbEntry& entry() { return *mEntry; }
    inline SeqdbSeq& seq() { return *mSeq; }
    inline const SeqdbEntry& entry() const { return *mEntry; }
    inline const SeqdbSeq& seq() const { return *mSeq; }

    inline std::string make_name(std::string aPassageSeparator = " ") const
        {
            return mSeq->hi_names().empty() ? string::strip(mEntry->name() + aPassageSeparator + mSeq->passage()) : mSeq->hi_names()[0];
        }

      // seq_id is either hi-name unmodified or concatenation of sequence name and passage separeted by __
    inline std::string seq_id() const
        {
            return make_name("__");
        }

 private:
    SeqdbEntry* mEntry;
    SeqdbSeq* mSeq;

}; // class SeqdbEntrySeq

namespace std
{
    template<> inline void swap(SeqdbEntrySeq& a, SeqdbEntrySeq& b) noexcept(is_nothrow_move_constructible<SeqdbEntrySeq>::value && is_nothrow_move_assignable<SeqdbEntrySeq>::value)
    {
        auto z = std::move(a);
        a = std::move(b);
        b = std::move(z);
    }
}

// ----------------------------------------------------------------------

class SeqdbIteratorBase : public std::iterator<std::input_iterator_tag, SeqdbEntrySeq>
{
 public:
    inline SeqdbIteratorBase(const SeqdbIteratorBase& a) = default;
    inline SeqdbIteratorBase(SeqdbIteratorBase&& a) = default;
    inline virtual ~SeqdbIteratorBase() {}

    inline virtual bool operator==(const SeqdbIteratorBase& aNother) const { return mEntryNo == aNother.mEntryNo && mSeqNo == aNother.mSeqNo; }
    inline virtual bool operator!=(const SeqdbIteratorBase& aNother) const { return ! operator==(aNother); }

    inline SeqdbIteratorBase& filter_lab(std::string aLab) { mLab = aLab; filter_added(); return *this; }
    inline SeqdbIteratorBase& filter_subtype(std::string aSubtype) { mSubtype = aSubtype; filter_added(); return *this; }
    inline SeqdbIteratorBase& filter_lineage(std::string aLineage) { mLineage = aLineage; filter_added(); return *this; }
    inline SeqdbIteratorBase& filter_aligned(bool aAligned) { mAligned = aAligned; filter_added(); return *this; }
    inline SeqdbIteratorBase& filter_gene(std::string aGene) { mGene = aGene; filter_added(); return *this; }
    inline SeqdbIteratorBase& filter_date_range(std::string aBegin, std::string aEnd) { mBegin = aBegin; mEnd = aEnd; filter_added(); return *this; }
    inline SeqdbIteratorBase& filter_hi_name(bool aHasHiName) { mHasHiName = aHasHiName; filter_added(); return *this; }
    inline SeqdbIteratorBase& filter_name_regex(std::string aNameRegex) { mNameMatcher.assign(aNameRegex, std::regex::icase); mNameMatcherSet = true; filter_added(); return *this; }

    virtual const Seqdb& seqdb() const = 0;
    virtual std::string make_name(std::string aPassageSeparator = " ") const = 0;

    inline SeqdbIteratorBase& operator ++ ();

    inline void validate() const;

 protected:
    inline SeqdbIteratorBase() : mNameMatcherSet(false) { end(); }
    inline SeqdbIteratorBase(size_t aEntryNo, size_t aSeqNo) : mEntryNo(aEntryNo), mSeqNo(aSeqNo), mAligned(false), mHasHiName(false), mNameMatcherSet(false) /*, mNameMatcher(".")*/ {}

    inline bool suitable_entry() const;
    inline bool suitable_seq() const;
    inline bool next_seq();
    inline void next_entry();

    inline size_t entry_no() const { return mEntryNo; }
    inline size_t seq_no() const { return mSeqNo; }

 private:
    size_t mEntryNo;
    size_t mSeqNo;

      // filter
    std::string mLab;
    std::string mSubtype;
    std::string mLineage;
    bool mAligned;
    std::string mGene;
    std::string mBegin;
    std::string mEnd;
    bool mHasHiName;
    bool mNameMatcherSet;
    std::regex mNameMatcher;

    inline void end() { mEntryNo = mSeqNo = std::numeric_limits<size_t>::max(); }
    inline void filter_added() { if (!suitable_entry() || !suitable_seq()) operator ++(); }

}; // class SeqdbIteratorBase

// ----------------------------------------------------------------------

class SeqdbIterator : public SeqdbIteratorBase
{
 public:
    inline SeqdbIterator(const SeqdbIterator& a) = default;
    inline SeqdbIterator(SeqdbIterator&& a) = default;
    inline virtual ~SeqdbIterator() {}

    inline bool operator==(const SeqdbIterator& aNother) const { return &mSeqdb == &aNother.mSeqdb && SeqdbIteratorBase::operator==(aNother); }

    inline SeqdbEntrySeq operator*();

    inline virtual const Seqdb& seqdb() const { return mSeqdb; }
    inline virtual std::string make_name(std::string aPassageSeparator = " ") const { return const_cast<SeqdbIterator*>(this)->operator*().make_name(aPassageSeparator); }

 private:
    inline SeqdbIterator(Seqdb& aSeqdb) : SeqdbIteratorBase(), mSeqdb(aSeqdb) {}
    inline SeqdbIterator(Seqdb& aSeqdb, size_t aEntryNo, size_t aSeqNo) : SeqdbIteratorBase(aEntryNo, aSeqNo), mSeqdb(aSeqdb) {}

    Seqdb& mSeqdb;

    friend class Seqdb;

}; // class SeqdbIterator

class ConstSeqdbIterator : public SeqdbIteratorBase
{
 public:
    inline ConstSeqdbIterator(const ConstSeqdbIterator& a) = default;
    inline ConstSeqdbIterator(ConstSeqdbIterator&& a) = default;
    inline virtual ~ConstSeqdbIterator() {}

    inline bool operator==(const ConstSeqdbIterator& aNother) const { return &mSeqdb == &aNother.mSeqdb && SeqdbIteratorBase::operator==(aNother); }

    inline const SeqdbEntrySeq operator*() const;

    inline virtual const Seqdb& seqdb() const { return mSeqdb; }
    inline virtual std::string make_name(std::string aPassageSeparator = " ") const { return operator*().make_name(aPassageSeparator); }

 private:
    inline ConstSeqdbIterator(const Seqdb& aSeqdb) : SeqdbIteratorBase(), mSeqdb(aSeqdb) {}
    inline ConstSeqdbIterator(const Seqdb& aSeqdb, size_t aEntryNo, size_t aSeqNo) : SeqdbIteratorBase(aEntryNo, aSeqNo), mSeqdb(aSeqdb) {}

    const Seqdb& mSeqdb;

    friend class Seqdb;

}; // class ConstSeqdbIterator

// ----------------------------------------------------------------------

class Seqdb
{
 public:
    inline Seqdb() {}

    void from_json(std::string data);
    void load(std::string filename);
    inline std::string to_json(size_t indent = 0) const { return json::dump(*this, static_cast<int>(indent)); }
    void save(std::string filename, size_t indent = 0) const;

    inline size_t number_of_entries() const { return mEntries.size(); }

    inline SeqdbEntry* find_by_name(std::string aName)
        {
            auto const first = find_insertion_place(aName);
            return (first != mEntries.end() && aName == first->name()) ? &(*first) : nullptr;
        }

    inline const SeqdbEntry* find_by_name(std::string aName) const
        {
            auto const first = find_insertion_place(aName);
            return (first != mEntries.end() && aName == first->name()) ? &(*first) : nullptr;
        }

    SeqdbEntrySeq find_by_seq_id(std::string aSeqId) const;

    SeqdbEntry* new_entry(std::string aName);

      // removes short sequences, removes entries having no sequences. returns messages
    std::string cleanup(bool remove_short_sequences);

      // returns db stat
    std::string report() const;
    std::string report_identical() const;
    std::string report_not_aligned(size_t prefix_size) const;
    std::vector<std::string> all_hi_names() const;
    void remove_hi_names();

      // iterating over sequences with filtering
    inline SeqdbIterator begin() { return SeqdbIterator(*this, 0, 0); }
    inline SeqdbIterator end() { return SeqdbIterator(*this); }
    inline ConstSeqdbIterator begin() const { return ConstSeqdbIterator(*this, 0, 0); }
    inline ConstSeqdbIterator end() const { return ConstSeqdbIterator(*this); }
    inline auto begin_entry() { return mEntries.begin(); }
    inline auto end_entry() { return mEntries.end(); }

    template <typename Value> std::deque<std::vector<SeqdbEntrySeq>> find_identical_sequences(Value value) const;

 private:
    std::vector<SeqdbEntry> mEntries;
    const std::regex sReYearSpace = std::regex("/[12][0-9][0-9][0-9] ");

    inline std::vector<SeqdbEntry>::iterator find_insertion_place(std::string aName)
        {
            return std::lower_bound(mEntries.begin(), mEntries.end(), aName, [](const SeqdbEntry& entry, std::string name) -> bool { return entry.name() < name; });
        }

    inline std::vector<SeqdbEntry>::const_iterator find_insertion_place(std::string aName) const
        {
            return std::lower_bound(mEntries.begin(), mEntries.end(), aName, [](const SeqdbEntry& entry, std::string name) -> bool { return entry.name() < name; });
        }

    friend class SeqdbIteratorBase;
    friend class SeqdbIterator;
    friend class ConstSeqdbIterator;

    static constexpr const char* SEQDB_JSON_DUMP_VERSION = "sequence-database-v2";
    std::string mJsonDumpVersion = SEQDB_JSON_DUMP_VERSION;

    friend inline auto json_fields(Seqdb& a)
        {
            return std::make_tuple(
                "  version", &a.mJsonDumpVersion,
                "data", &a.mEntries
                                   );
        }
};

// ----------------------------------------------------------------------

inline void SeqdbIteratorBase::validate() const
{
    if (mEntryNo >= seqdb().mEntries.size() || mSeqNo >= seqdb().mEntries[mEntryNo].mSeq.size())
        throw std::out_of_range("SeqdbIterator is out of range");

} // SeqdbIteratorBase::valid

// ----------------------------------------------------------------------

inline SeqdbEntrySeq SeqdbIterator::operator*()
{
    validate();
    return SeqdbEntrySeq(mSeqdb.mEntries[entry_no()], mSeqdb.mEntries[entry_no()].mSeq[seq_no()]);

} // SeqdbIterator::operator*

// ----------------------------------------------------------------------

inline const SeqdbEntrySeq ConstSeqdbIterator::operator*() const
{
    validate();
    return SeqdbEntrySeq(mSeqdb.mEntries[entry_no()], mSeqdb.mEntries[entry_no()].mSeq[seq_no()]);

} // SeqdbIterator::operator*

// ----------------------------------------------------------------------

// inline SeqdbIterator::ConstDereferenceType SeqdbIterator::operator*() const
// {
//     if (mEntryNo >= mSeqdb.mEntries.size() || mSeqNo >= mSeqdb.mEntries[mEntryNo].mSeq.size())
//         throw std::out_of_range("SeqdbIterator is out of range");
//     return std::make_pair(&mSeqdb.mEntries[mEntryNo], &mSeqdb.mEntries[mEntryNo].mSeq[mSeqNo]);

// } // SeqdbIterator::operator*

// ----------------------------------------------------------------------

inline bool SeqdbIteratorBase::suitable_entry() const
{
    auto const & entry = seqdb().mEntries[mEntryNo];
    return (mSubtype.empty() || entry.mVirusType == mSubtype)
            && (mLineage.empty() || entry.mLineage == mLineage)
            && entry.date_within_range(mBegin, mEnd)
            ;

} // SeqdbIterator::suitable_entry

// ----------------------------------------------------------------------

inline bool SeqdbIteratorBase::suitable_seq() const
{
    auto const & seq = seqdb().mEntries[mEntryNo].mSeq[mSeqNo];
    return (!mAligned || seq.aligned())
            && (mGene.empty() || seq.mGene == mGene)
            && (!mHasHiName || !seq.mHiNames.empty())
            && (mLab.empty() || seq.has_lab(mLab))
            && (!mNameMatcherSet || std::regex_search(make_name(), mNameMatcher))
            ;

} // SeqdbIterator::suitable_seq

// ----------------------------------------------------------------------

inline bool SeqdbIteratorBase::next_seq()
{
    auto const & entry = seqdb().mEntries[mEntryNo];
    ++mSeqNo;
    while (mSeqNo < entry.mSeq.size() && !suitable_seq())
        ++mSeqNo;
    return mSeqNo < entry.mSeq.size();

} // SeqdbIterator::next_seq

// ----------------------------------------------------------------------

inline void SeqdbIteratorBase::next_entry()
{
    while (true) {
        ++mEntryNo;
        while (mEntryNo < seqdb().mEntries.size() && !suitable_entry())
            ++mEntryNo;
        if (mEntryNo >= seqdb().mEntries.size()) {
            end();
            break;
        }
        else {
            mSeqNo = static_cast<size_t>(-1);
            if (next_seq())
                break;
        }
    }

} // SeqdbIterator::next_entry

// ----------------------------------------------------------------------

inline SeqdbIteratorBase& SeqdbIteratorBase::operator ++ ()
{
    if (!next_seq()) {
        next_entry();
    }
    return *this;

} // SeqdbIterator::operator ++

// ----------------------------------------------------------------------

template <typename Value> std::deque<std::vector<SeqdbEntrySeq>> Seqdb::find_identical_sequences(Value value) const
{
    std::vector<SeqdbEntrySeq> refs(begin(), end());
    sort(refs.begin(), refs.end(), [&value](const auto& a, const auto b) { return value(a) < value(b); });
    std::deque<std::vector<SeqdbEntrySeq>> identical = {{}};
    for (auto previous = refs.begin(), current = previous + 1; current != refs.end(); ++current) {
        if (value(*previous) == value(*current)) {
            if (!value(*previous).empty()) { // empty means not aligned, ignore them
                if (identical.back().empty())
                    identical.back().push_back(*previous);
                identical.back().push_back(*current);
            }
        }
        else {
            previous = current;
            if (!identical.back().empty())
                identical.push_back(std::vector<SeqdbEntrySeq>());
        }
    }
    if (identical.back().empty())
        identical.pop_back();
    return identical;

} // Seqdb::find_identical_sequences

// ----------------------------------------------------------------------

// class SeqdbParsingError : public std::runtime_error
// {
//  public:
//     using std::runtime_error::runtime_error;
// };

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
