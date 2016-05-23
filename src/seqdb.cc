#include "seqdb.hh"
#include "clades.hh"
#include "string.hh"

// ----------------------------------------------------------------------

bool SeqdbSeq::match_update_nucleotides(std::string aNucleotides)
{
    bool matches = false;
    if (mNucleotides == aNucleotides) {
        matches = true;
    }
    else if (mNucleotides.find(aNucleotides) != std::string::npos) { // sub
        matches = true;
    }
    else if (aNucleotides.find(mNucleotides) != std::string::npos) { // super
        matches = true;
        mNucleotides = aNucleotides;
        mNucleotidesShift.reset();
        mAminoAcidsShift.reset();
        mAminoAcids.clear();
    }
    return matches;

} // SeqdbSeq::match_update_nucleotides

// ----------------------------------------------------------------------

bool SeqdbSeq::match_update_amino_acids(std::string aAminoAcids)
{
    bool matches = false;
    if (mAminoAcids == aAminoAcids) {
        matches = true;
    }
    else if (mAminoAcids.find(aAminoAcids) != std::string::npos) { // sub
        matches = true;
    }
    else if (aAminoAcids.find(mAminoAcids) != std::string::npos) { // super
        matches = true;
        mNucleotides.clear();
        mNucleotidesShift.reset();
        mAminoAcidsShift.reset();
        mAminoAcids = aAminoAcids;
    }
    return matches;

} // SeqdbSeq::match_update_amino_acids

// ----------------------------------------------------------------------

void SeqdbSeq::add_passage(std::string aPassage)
{
    if (std::find(mPassages.begin(), mPassages.end(), aPassage) == mPassages.end())
        mPassages.push_back(aPassage);

} // SeqdbSeq::add_passage

// ----------------------------------------------------------------------

void SeqdbSeq::update_gene(std::string aGene, Messages& aMessages, bool replace_ha)
{
    if (!aGene.empty()) {
        if (mGene.empty())
            mGene = aGene;
        else if (aGene != mGene) {
            if (replace_ha && mGene == "HA")
                mGene = aGene;
            else
                aMessages.warning() << "[SAMESEQ] different genes " << mGene << " vs. " << aGene << std::endl;
        }
    }

} // SeqdbSeq::update_gene

// ----------------------------------------------------------------------

void SeqdbSeq::add_reassortant(std::string aReassortant)
{
    if (std::find(mReassortant.begin(), mReassortant.end(), aReassortant) == mReassortant.end())
        mReassortant.push_back(aReassortant);

} // SeqdbSeq::add_reassortant

// ----------------------------------------------------------------------

void SeqdbSeq::add_lab_id(std::string aLab, std::string aLabId)
{
    if (!aLab.empty()) {
        auto& lab_ids = mLabIds[aLab];
        if (!aLabId.empty() && std::find(lab_ids.begin(), lab_ids.end(), aLabId) == lab_ids.end()) {
            lab_ids.push_back(aLabId);
        }
    }

} // SeqdbSeq::add_lab_id

// ----------------------------------------------------------------------

AlignAminoAcidsData SeqdbSeq::align(bool aForce, Messages& aMessages)
{
    AlignAminoAcidsData align_data;

    enum WhatAlign {no_align, align_nucleotides, aling_amino_acids};
    WhatAlign what_align = no_align;
    if (!mNucleotides.empty() && (mAminoAcids.empty() || aForce))
        what_align = align_nucleotides;
    else if (!mAminoAcids.empty() && mNucleotides.empty() && (!mAminoAcidsShift.aligned() || aForce))
        what_align = aling_amino_acids;

    switch (what_align) {
      case no_align:
          break;
      case align_nucleotides:
          mAminoAcidsShift.reset();
          align_data = translate_and_align(mNucleotides, aMessages);
          if (!align_data.shift.alignment_failed()) {
              mAminoAcids = align_data.amino_acids;
              update_gene(align_data.gene, aMessages, true);
              if (align_data.shift.aligned()) {
                  mAminoAcidsShift = align_data.shift;
                  mNucleotidesShift = - align_data.offset + align_data.shift * 3;
              }
          }
          else {
              aMessages.warning() << "Nucs not translated/aligned" /* << mNucleotides */ << std::endl;
          }
          break;
      case aling_amino_acids:
          mAminoAcidsShift.reset();
          align_data = align_amino_acids(mAminoAcids, aMessages);
          if (align_data.shift.aligned()) {
              mAminoAcidsShift = align_data.shift;
              update_gene(align_data.gene, aMessages, true);
          }
          else {
              aMessages.warning() << "AA not aligned" /* << mAminoAcids */ << std::endl;
          }
          break;
    }

    return align_data;

} // SeqdbSeq::align

// ----------------------------------------------------------------------

void SeqdbSeq::update_clades(std::string aVirusType, std::string aLineage)
{
    if (aligned()) {
        if (aVirusType == "B" && aLineage == "YAMAGATA") {
            mClades = clades_b_yamagata(mAminoAcids, mAminoAcidsShift);
        }
        else if (aVirusType == "A(H1N1)") {
            mClades = clades_h1pdm(mAminoAcids, mAminoAcidsShift);
        }
        else if (aVirusType == "A(H3N2)") {
            mClades = clades_h3n2(mAminoAcids, mAminoAcidsShift);
        }
    }

} // SeqdbSeq::update_clades

// ----------------------------------------------------------------------

std::string SeqdbSeq::amino_acids(bool aAligned, size_t aLeftPartSize) const
{
    std::string r = mAminoAcids;
    if (aAligned) {
        if (!aligned())
            throw std::runtime_error("amino_acids(): sequence not aligned");
        r = shift(r, mAminoAcidsShift + static_cast<int>(aLeftPartSize), 'X');

          // find the longest part not having *, replace parts before longest with X, truncate traling parts
        size_t longest_part_start = 0;
        size_t longest_part_len = 0;
        for (size_t start = 0; start != std::string::npos; ) {
            const size_t found = r.find_first_of('*', start);
            const size_t len = (found == std::string::npos ? r.size() : found) - start;
            if (longest_part_len < len) {
                longest_part_len = len;
                longest_part_start = start;
            }
            start = found == std::string::npos ? found : found + 1;
        }
        r.replace(0, longest_part_start, longest_part_start, 'X');
        r.resize(longest_part_start + longest_part_len, 'X');
    }
    return r;

} // SeqdbSeq::amino_acids

// ----------------------------------------------------------------------

std::string SeqdbSeq::nucleotides(bool aAligned, size_t aLeftPartSize) const
{
    std::string r = mNucleotides;
    if (aAligned) {
        if (!aligned())
            throw std::runtime_error("nucleotides(): sequence not aligned");
        r = shift(r, mNucleotidesShift + static_cast<int>(aLeftPartSize), '-');
    }
    return r;

} // SeqdbSeq::nucleotides

// ----------------------------------------------------------------------

void SeqdbEntry::add_date(std::string aDate)
{
    auto insertion_pos = std::lower_bound(mDates.begin(), mDates.end(), aDate);
    if (insertion_pos == mDates.end() || aDate != *insertion_pos) {
        mDates.insert(insertion_pos, aDate);
    }

} // SeqdbEntry::add_date

// ----------------------------------------------------------------------

void SeqdbEntry::update_lineage(std::string aLineage, Messages& aMessages)
{
    if (!aLineage.empty()) {
        if (mLineage.empty())
            mLineage = aLineage;
        else if (aLineage != mLineage)
            aMessages.warning() << "Different lineages " << mLineage << " (stored) vs. " << aLineage << " (ignored)" << std::endl;
    }

} // SeqdbEntry::update_lineage

// ----------------------------------------------------------------------

void SeqdbEntry::update_subtype(std::string aSubtype, Messages& aMessages)
{
    if (!aSubtype.empty()) {
        if (mVirusType.empty())
            mVirusType = aSubtype;
        else if (aSubtype != mVirusType)
            aMessages.warning() << "Different subtypes " << mVirusType << " (stored) vs. " << aSubtype << " (ignored)" << std::endl;
    }

} // SeqdbEntry::update_subtype

// ----------------------------------------------------------------------

std::string SeqdbEntry::add_or_update_sequence(std::string aSequence, std::string aPassage, std::string aReassortant, std::string aLab, std::string aLabId, std::string aGene)
{
    Messages messages;
    const bool nucs = is_nucleotides(aSequence);
    decltype(mSeq.begin()) found;
    if (nucs)
        found = std::find_if(mSeq.begin(), mSeq.end(), [&aSequence](SeqdbSeq& seq) { return seq.match_update_nucleotides(aSequence); });
    else
        found = std::find_if(mSeq.begin(), mSeq.end(), [&aSequence](SeqdbSeq& seq) { return seq.match_update_amino_acids(aSequence); });
    if (found != mSeq.end()) {  // update
        found->update_gene(aGene, messages);
    }
    else { // add
        if (nucs)
            mSeq.push_back(SeqdbSeq(aSequence, aGene));
        else
            mSeq.push_back(SeqdbSeq(std::string(), aSequence, aGene));
        found = mSeq.end() - 1;
    }
    if (found != mSeq.end()) {
        found->add_passage(aPassage);
        found->add_reassortant(aReassortant);
        found->add_lab_id(aLab, aLabId);
        auto align_data = found->align(false, messages);
        update_subtype(align_data.subtype, messages);
        update_lineage(align_data.lineage, messages);
    }
    return messages;

} // SeqdbEntry::add_or_update_sequence

// ----------------------------------------------------------------------

SeqdbEntry* Seqdb::new_entry(std::string aName)
{
    auto const first = find_insertion_place(aName);
    if (first != mEntries.end() && aName == first->name())
        throw std::runtime_error(std::string("Entry for \"") + aName + "\" already exists");
    auto inserted = mEntries.insert(first, SeqdbEntry(aName));
    return &*inserted;

} // Seqdb::new_entry

// ----------------------------------------------------------------------

std::string Seqdb::cleanup(bool remove_short_sequences)
{
    Messages messages;
    if (remove_short_sequences) {
        std::for_each(mEntries.begin(), mEntries.end(), std::mem_fn(&SeqdbEntry::remove_short_sequences));
    }
    std::for_each(mEntries.begin(), mEntries.end(), std::mem_fn(&SeqdbEntry::remove_not_translated_sequences));
    //   // Note empty passages must not be removed, this is just for testing purposes
    // std::for_each(mEntries.begin(), mEntries.end(), std::mem_fn(&SeqdbEntry::remove_empty_passages));
      // remove empty entries
    auto const num_entries_before = mEntries.size();
      //std::remove_if(mEntries.begin(), mEntries.end(), [](auto entry) { return entry.empty(); });
    mEntries.erase(std::remove_if(mEntries.begin(), mEntries.end(), std::mem_fn(&SeqdbEntry::empty)), mEntries.end());
    if (mEntries.size() != num_entries_before)
        messages.warning() << (num_entries_before - mEntries.size()) << " entries removed during cleanup" << std::endl;
    return messages;

} // Seqdb::cleanup

// ----------------------------------------------------------------------

std::string Seqdb::report() const
{
    std::ostringstream os;
    os << "Entries: " << mEntries.size() << std::endl;

    std::string buf;

    std::map<std::string, size_t> virus_types;
    for_each(mEntries.begin(), mEntries.end(), [&virus_types](auto& e) { ++virus_types[e.virus_type()]; });
    jsonw::json(buf, jsonw::NoCommaNoIndent, virus_types, 0, 0);
    os << "Virus types: " << buf << std::endl;
    buf.clear();

    std::map<std::string, size_t> lineages;
    for_each(mEntries.begin(), mEntries.end(), [&lineages](auto& e) { ++lineages[e.lineage()]; });
    jsonw::json(buf, jsonw::NoCommaNoIndent, lineages, 0, 0);
    os << "Lineages: " << buf << std::endl;
    buf.clear();

    std::map<std::string, size_t> aligned;
    for_each(mEntries.begin(), mEntries.end(), [&aligned](auto& e) { if (any_of(e.mSeq.begin(), e.mSeq.end(), std::mem_fn(&SeqdbSeq::aligned))) ++aligned[e.virus_type()]; });
    jsonw::json(buf, jsonw::NoCommaNoIndent, virus_types, 0, 0);
    os << "Aligned: " << buf << std::endl;
    buf.clear();

    std::map<std::string, size_t> matched;
    for_each(mEntries.begin(), mEntries.end(), [&matched](auto& e) { if (any_of(e.mSeq.begin(), e.mSeq.end(), std::mem_fn(&SeqdbSeq::matched))) ++matched[e.virus_type()]; });
    jsonw::json(buf, jsonw::NoCommaNoIndent, matched, 0, 0);
    os << "Matched: " << buf << std::endl;
    buf.clear();

    std::map<std::string, size_t> have_dates;
    for_each(mEntries.begin(), mEntries.end(), [&have_dates](auto& e) { if (!e.mDates.empty()) ++have_dates[e.virus_type()]; });
    jsonw::json(buf, jsonw::NoCommaNoIndent, have_dates, 0, 0);
    os << "Have dates: " << buf << std::endl;
    buf.clear();

    std::map<std::string, size_t> have_clades;
    for_each(mEntries.begin(), mEntries.end(), [&have_clades](auto& e) { if (any_of(e.mSeq.begin(), e.mSeq.end(), [](auto& seq) { return !seq.mClades.empty(); })) ++have_clades[e.virus_type()]; });
    jsonw::json(buf, jsonw::NoCommaNoIndent, have_clades, 0, 0);
    os << "Have clades: " << buf << std::endl;
    buf.clear();

    return os.str();

} // Seqdb::report

// ----------------------------------------------------------------------

std::string Seqdb::report_identical() const
{
    std::ostringstream os;

    auto report = [&os](std::string prefix, const auto& groups) {
        if (!groups.empty()) {
            os << prefix << std::endl;
            for (auto const& group: groups) {
                for (auto const& entry: group) {
                    os << entry.make_name() << ' ';
                }
                os << std::endl;
            }
        }
    };

    try {
        report("Identical nucleotides:", find_identical_sequences([](const SeqdbEntrySeq& e) -> std::string { return e.seq().nucleotides(true); }));
        os << std::endl;
    }
    catch (InvalidShift&) {
        os << "Cannot report identical nucleotides: not aligned or absent" << std::endl;
    }

    try {
        report("Identical amino-acids:", find_identical_sequences([](const SeqdbEntrySeq& e) -> std::string { return e.seq().amino_acids(true); }));
        os << std::endl;
    }
    catch (InvalidShift&) {
        os << "Cannot report identical amino-acids: not aligned or absent" << std::endl;
    }

    return os.str();

} // Seqdb::report_identical

// ----------------------------------------------------------------------

std::vector<std::string> Seqdb::all_hi_names() const
{
    std::vector<std::string> r;
    for (auto const& entry: mEntries) {
        for (auto const& seq: entry.mSeq) {
            std::copy(seq.hi_names().begin(), seq.hi_names().end(), std::back_inserter(r));
        }
    }
    std::sort(r.begin(), r.end());
    auto last = std::unique(r.begin(), r.end());
    r.erase(last, r.end());
    return r;

} // Seqdb::all_hi_names

// ----------------------------------------------------------------------

void Seqdb::remove_hi_names()
{
    for (auto& entry: mEntries) {
        for (auto& seq: entry.mSeq) {
            seq.hi_names().clear();
        }
    }

} // Seqdb::remove_hi_names

// ----------------------------------------------------------------------

SeqdbEntrySeq Seqdb::find_by_seq_id(std::string aSeqId) const
{
    SeqdbEntrySeq result;
    const auto passage_separator = aSeqId.find("__");
    if (passage_separator != std::string::npos) { // seq_id
        const auto entry = find_by_name(std::string(aSeqId, 0, passage_separator));
        if (entry != nullptr) {
            const auto passage_distinct = string::split(std::string(aSeqId, passage_separator + 2), "__", string::Split::KeepEmpty);
            auto index = passage_distinct.size() == 1 ? 0 : std::stoi(passage_distinct[1]);
            for (auto seq = entry->begin_seq(); seq != entry->end_seq(); ++seq) {
                if (seq->passage() == passage_distinct[0]) {
                    if (index == 0) {
                        result.assign(*entry, *seq);
                        break;
                    }
                    else
                        --index;
                }
            }
        }
    }
    else {
        std::smatch year_space;
        const auto year_space_present = std::regex_search(aSeqId, year_space, sReYearSpace);
        const std::string look_for = year_space_present ? std::string(aSeqId, 0, static_cast<std::string::size_type>(year_space.position(0) + year_space.length(0)) - 1) : aSeqId;
        const auto entry = find_by_name(look_for);
        if (entry != nullptr) {
            auto found = std::find_if(entry->begin_seq(), entry->end_seq(), [&aSeqId](const auto& seq) -> bool { return seq.hi_name_present(aSeqId); });
            if (found == entry->end_seq()) { // not found by hi_name, look by passage (or empty passage)
                const std::string passage = year_space_present ? std::string(aSeqId, static_cast<std::string::size_type>(year_space.position(0) + year_space.length(0))) : std::string();
                found = std::find_if(entry->begin_seq(), entry->end_seq(), [&passage](const auto& seq) -> bool { return seq.passage_present(passage); });
            }
            if (found != entry->end_seq()) {
                result.assign(*entry, *found);
            }
        }
    }

    if (!result) {
        std::cerr << "Error: \"" << aSeqId << "\" not in seqdb" << std::endl;
    }
    return result;

} // Seqdb::find_by_seq_id

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
