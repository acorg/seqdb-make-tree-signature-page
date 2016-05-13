#include <cstdlib>

#include "seqdb.hh"
#include "read-file.hh"

// ----------------------------------------------------------------------

std::string Seqdb::json(size_t indent) const
{
    std::string target;
    size_t prefix = 0;
    jsonw::json_begin(target, jsonw::NoCommaNoIndent, '{', indent, prefix);
    auto comma = jsonw::json(target, jsonw::NoComma, "  version", "sequence-database-v2", indent, prefix);
    jsonw::json(target, comma, "data", mEntries, indent, prefix);
    jsonw::json_end(target, '}', indent, prefix);
    return target;

} // Seqdb::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma SeqdbSeq::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json_if(target, comma, 'a', mAminoAcids, indent, prefix);
    comma = jsonw::json_if(target, comma, 'c', mClades, indent, prefix);
    comma = jsonw::json_if(target, comma, 'g', mGene, indent, prefix);
    comma = jsonw::json_if(target, comma, 'h', mHiNames, indent, prefix);
    comma = jsonw::json_if(target, comma, 'l', mLabIds, indent, prefix);
    comma = jsonw::json_if(target, comma, 'n', mNucleotides, indent, prefix);
    comma = jsonw::json_if(target, comma, 'p', mPassages, indent, prefix);
    comma = jsonw::json_if(target, comma, 'r', mReassortant, indent, prefix);
    if (mAminoAcidsShift.aligned())
        comma = jsonw::json_if(target, comma, 's', static_cast<int>(mAminoAcidsShift), indent, prefix);
    if (mNucleotidesShift.aligned())
        comma = jsonw::json_if(target, comma, 't', static_cast<int>(mNucleotidesShift), indent, prefix);
    return jsonw::json_end(target, '}', indent, prefix);

} // SeqdbSeq::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma SeqdbEntry::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json_if(target, jsonw::NoComma, 'N', mName, indent, prefix);
    comma = jsonw::json_if(target, comma, 'C', mContinent, indent, prefix);
    comma = jsonw::json_if(target, comma, 'c', mCountry, indent, prefix);
    comma = jsonw::json_if(target, comma, 'd', mDates, indent, prefix);
    comma = jsonw::json_if(target, comma, 'l', mLineage, indent, prefix);
    comma = jsonw::json(target, comma, 's', mSeq, indent, prefix);
    comma = jsonw::json_if(target, comma, 'v', mVirusType, indent, prefix);
    return jsonw::json_end(target, '}', indent, prefix);

} // SeqdbEntry::json

// ----------------------------------------------------------------------

void Seqdb::from_json(std::string data)
{
    const std::string expected_version = "sequence-database-v2";
    auto parse_db = jsonr::object(jsonr::version(expected_version) | jsonr::object_array_value("data", mEntries));
    try {
        parse_db(std::begin(data), std::end(data));
    }
    catch (axe::failure<char>& err) {
        std::cerr << "Cannot parse seqdb: " << err.message() << std::endl;
        throw jsonr::JsonParsingError(err.message());
    }

} // Seqdb::from_json

// ----------------------------------------------------------------------

void Seqdb::load(std::string filename)
{
    // if (filename.empty()) {
    //     filename = std::string(getenv("HOME")) + "/WHO/seqdb.json.xz";
    // }
    from_json(read_file(filename));

} // Seqdb::from_json_file

// ----------------------------------------------------------------------

void Seqdb::save(std::string filename, size_t indent) const
{
    // if (filename.empty()) {
    //     filename = std::string(getenv("HOME")) + "/WHO/seqdb.json.xz";
    // }
    write_file(filename, json(indent));

} // Seqdb::save

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
