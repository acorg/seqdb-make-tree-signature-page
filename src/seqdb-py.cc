#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wreserved-id-macro" // in Python.h
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#pragma GCC diagnostic pop

namespace py = pybind11;

#include "seqdb.hh"
#include "tree-import.hh"
#include "draw.hh"
#include "draw-tree.hh"
#include "signature-page.hh"
#include "settings.hh"
#include "chart.hh"

// ----------------------------------------------------------------------

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
// #pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

// ----------------------------------------------------------------------

struct PySeqdbEntrySeqIterator
{
    inline PySeqdbEntrySeqIterator(Seqdb& aSeqdb, py::object aRef)
        : mRef(aRef), mCurrent(aSeqdb.begin()), mEnd(aSeqdb.end())
        {
        }

    inline SeqdbEntrySeq next()
        {
            if (mCurrent == mEnd)
                throw py::stop_iteration();
            auto r = *mCurrent;
            ++mCurrent;
            return r;
        }

    inline PySeqdbEntrySeqIterator& filter_lab(std::string aLab) { mCurrent.filter_lab(aLab); return *this; }
    inline PySeqdbEntrySeqIterator& filter_labid(std::string aLab, std::string aId) { mCurrent.filter_labid(aLab, aId); return *this; }
    inline PySeqdbEntrySeqIterator& filter_subtype(std::string aSubtype) { mCurrent.filter_subtype(aSubtype); return *this; }
    inline PySeqdbEntrySeqIterator& filter_lineage(std::string aLineage) { mCurrent.filter_lineage(aLineage); return *this; }
    inline PySeqdbEntrySeqIterator& filter_aligned(bool aAligned) { mCurrent.filter_aligned(aAligned); return *this; }
    inline PySeqdbEntrySeqIterator& filter_gene(std::string aGene) { mCurrent.filter_gene(aGene); return *this; }
    inline PySeqdbEntrySeqIterator& filter_date_range(std::string aBegin, std::string aEnd) { mCurrent.filter_date_range(aBegin, aEnd); return *this; }
    inline PySeqdbEntrySeqIterator& filter_hi_name(bool aHasHiName) { mCurrent.filter_hi_name(aHasHiName); return *this; }
    inline PySeqdbEntrySeqIterator& filter_name_regex(std::string aNameRegex) { mCurrent.filter_name_regex(aNameRegex); return *this; }

    py::object mRef; // keep a reference
    SeqdbIterator mCurrent;
    SeqdbIterator mEnd;

}; // struct PySeqdbSeqIterator

// ----------------------------------------------------------------------

struct PySeqdbEntryIterator
{
    inline PySeqdbEntryIterator(Seqdb& aSeqdb, py::object aRef)
        : mRef(aRef), mCurrent(aSeqdb.begin_entry()), mEnd(aSeqdb.end_entry())
        {
        }

    inline auto& next()
        {
            if (mCurrent == mEnd)
                throw py::stop_iteration();
            auto& r = *mCurrent;
            ++mCurrent;
            return r;
        }

    py::object mRef; // keep a reference
    std::vector<SeqdbEntry>::iterator mCurrent;
    std::vector<SeqdbEntry>::iterator mEnd;

}; // struct PySeqdbEntryIterator

// ----------------------------------------------------------------------

struct PySeqdbSeqIterator
{
    inline PySeqdbSeqIterator(SeqdbEntry& aSeqdbEntry)
        : mCurrent(aSeqdbEntry.begin_seq()), mEnd(aSeqdbEntry.end_seq())
        {
        }

    inline auto& next()
        {
            if (mCurrent == mEnd)
                throw py::stop_iteration();
            auto& r = *mCurrent;
            ++mCurrent;
            return r;
        }

    std::vector<SeqdbSeq>::iterator mCurrent;
    std::vector<SeqdbSeq>::iterator mEnd;

}; // struct PySeqdbSeqIterator

// ----------------------------------------------------------------------

PYBIND11_PLUGIN(seqdb_backend)
{
    py::module m("seqdb_backend", "SeqDB access plugin");

    py::class_<SeqdbSeq>(m, "SeqdbSeq")
            .def("has_lab", &SeqdbSeq::has_lab, py::arg("lab"))
            .def("cdcids", &SeqdbSeq::cdcids)
            .def("update_clades", &SeqdbSeq::update_clades, py::arg("virus_type"), py::arg("lineage"))
            .def_property_readonly("passages", &SeqdbSeq::passages)
            .def_property_readonly("reassortant", &SeqdbSeq::reassortant)
            .def_property_readonly("hi_names", static_cast<const std::vector<std::string>& (SeqdbSeq::*)() const>(&SeqdbSeq::hi_names))
            .def("add_hi_name", &SeqdbSeq::add_hi_name, py::arg("hi_name"))
            .def("amino_acids", &SeqdbSeq::amino_acids, py::arg("aligned"), py::arg("left_part_size") = int(0), py::doc("if aligned and left_part_size > 0 - include signal peptide and other stuff to the left from the beginning of the aligned sequence."))
            .def("nucleotides", &SeqdbSeq::nucleotides, py::arg("aligned"), py::arg("left_part_size") = int(0), py::doc("if aligned and left_part_size > 0 - include signal peptide and other stuff to the left from the beginning of the aligned sequence."))
            .def("amino_acids_shift", &SeqdbSeq::amino_acids_shift)
            .def("nucleotides_shift", &SeqdbSeq::nucleotides_shift)
            .def("lab", &SeqdbSeq::lab)
            .def("lab_id", &SeqdbSeq::lab_id)
            .def("lab_ids", &SeqdbSeq::lab_ids_for_lab, py::arg("lab"))
            .def("lab_ids", &SeqdbSeq::lab_ids)
            .def("passage", &SeqdbSeq::passage)
            .def("gene", &SeqdbSeq::gene)
            .def("clades", &SeqdbSeq::clades)
            ;

    py::class_<SeqdbEntry>(m, "SeqdbEntry")
            .def_property_readonly("name", &SeqdbEntry::name)
            .def_property("continent", static_cast<std::string (SeqdbEntry::*)() const>(&SeqdbEntry::continent), static_cast<void (SeqdbEntry::*)(std::string)>(&SeqdbEntry::continent))
            .def_property("country", static_cast<std::string (SeqdbEntry::*)() const>(&SeqdbEntry::country), static_cast<void (SeqdbEntry::*)(std::string)>(&SeqdbEntry::country))
            .def_property("virus_type", static_cast<std::string (SeqdbEntry::*)() const>(&SeqdbEntry::virus_type), static_cast<void (SeqdbEntry::*)(std::string)>(&SeqdbEntry::virus_type))
              // .def("set_virus_type", static_cast<void (SeqdbEntry::*)(std::string)>(&SeqdbEntry::virus_type), py::arg("virus_type"))
            .def("add_date", &SeqdbEntry::add_date, py::arg("date"))
            .def("add_or_update_sequence", &SeqdbEntry::add_or_update_sequence, py::arg("sequence"), py::arg("passage"), py::arg("reassortant"), py::arg("lab"), py::arg("lab_id"), py::arg("gene"))
            .def("cdcids", &SeqdbEntry::cdcids)
            .def("date", &SeqdbEntry::date)
            .def_property_readonly("lineage", &SeqdbEntry::lineage)
            .def("update_lineage", [](SeqdbEntry& entry, std::string lineage) -> std::string { Messages msg; entry.update_lineage(lineage, msg); return msg; }, py::arg("lineage"))
            .def("__iter__", [](py::object entry) { return PySeqdbSeqIterator(entry.cast<SeqdbEntry&>()); })
            ;

    py::class_<SeqdbEntrySeq>(m, "SeqdbEntrySeq")
            .def_property_readonly("entry", static_cast<const SeqdbEntry& (SeqdbEntrySeq::*)() const>(&SeqdbEntrySeq::entry), py::return_value_policy::reference)
            .def_property_readonly("seq", static_cast<const SeqdbSeq& (SeqdbEntrySeq::*)() const>(&SeqdbEntrySeq::seq), py::return_value_policy::reference)
            .def("make_name", &SeqdbEntrySeq::make_name, py::arg("passage_separator") = std::string(" "))
            .def("seq_id", &SeqdbEntrySeq::seq_id)
            ;

    py::class_<PySeqdbEntrySeqIterator>(m, "PySeqdbEntrySeqIterator")
            .def("__iter__", [](PySeqdbEntrySeqIterator& it) { return it; })
            .def("__next__", &PySeqdbEntrySeqIterator::next)
            .def("filter_lab", &PySeqdbEntrySeqIterator::filter_lab)
            .def("filter_labid", &PySeqdbEntrySeqIterator::filter_labid, py::arg("lab"), py::arg("id"))
            .def("filter_subtype", &PySeqdbEntrySeqIterator::filter_subtype)
            .def("filter_lineage", &PySeqdbEntrySeqIterator::filter_lineage)
            .def("filter_aligned", &PySeqdbEntrySeqIterator::filter_aligned)
            .def("filter_gene", &PySeqdbEntrySeqIterator::filter_gene)
            .def("filter_date_range", &PySeqdbEntrySeqIterator::filter_date_range)
            .def("filter_hi_name", &PySeqdbEntrySeqIterator::filter_hi_name)
            .def("filter_name_regex", &PySeqdbEntrySeqIterator::filter_name_regex)
            ;

    py::class_<PySeqdbEntryIterator>(m, "PySeqdbEntryIterator")
            .def("__iter__", [](PySeqdbEntryIterator& it) { return it; })
            .def("__next__", &PySeqdbEntryIterator::next, py::return_value_policy::reference);
            ;

    py::class_<PySeqdbSeqIterator>(m, "PySeqdbSeqIterator")
            .def("__iter__", [](PySeqdbSeqIterator& it) { return it; })
            .def("__next__", &PySeqdbSeqIterator::next, py::return_value_policy::reference);
            ;

    py::class_<Seqdb>(m, "Seqdb")
            .def(py::init<>())
            .def("from_json", &Seqdb::from_json, py::doc("reads seqdb from json"))
            .def("load", &Seqdb::load, py::arg("filename") = std::string(), py::doc("reads seqdb from file containing json"))
            .def("json", &Seqdb::to_json, py::arg("indent") = size_t(0))
            .def("save", &Seqdb::save, py::arg("filename") = std::string(), py::arg("indent") = size_t(0), py::doc("writes seqdb into file in json format"))
            .def("number_of_entries", &Seqdb::number_of_entries)
            .def("find_by_name", static_cast<SeqdbEntry* (Seqdb::*)(std::string)>(&Seqdb::find_by_name), py::arg("name"), py::return_value_policy::reference, py::doc("returns entry found by name or None"))
            .def("new_entry", &Seqdb::new_entry, py::arg("name"), py::return_value_policy::reference, py::doc("creates and inserts into the database new entry with the passed name, returns that name, throws if database already has entry with that name."))
            .def("cleanup", &Seqdb::cleanup, py::arg("remove_short_sequences") = true)
            .def("report", &Seqdb::report)
            .def("report_identical", &Seqdb::report_identical)
            .def("report_not_aligned", &Seqdb::report_not_aligned, py::arg("prefix_size"), py::doc("returns report with AA prefixes of not aligned sequences."))
            .def("iter_seq", [](py::object seqdb) { return PySeqdbEntrySeqIterator(seqdb.cast<Seqdb&>(), seqdb); })
            .def("iter_entry", [](py::object seqdb) { return PySeqdbEntryIterator(seqdb.cast<Seqdb&>(), seqdb); })
            .def("all_hi_names", &Seqdb::all_hi_names, py::doc("returns list of all hi_names (\"h\") found in seqdb."))
            .def("remove_hi_names", &Seqdb::remove_hi_names, py::doc("removes all hi_names (\"h\") found in seqdb (e.g. before matching again)."))
            ;

      // ----------------------------------------------------------------------

    py::class_<Node>(m, "Node")
            .def_readonly("name", &Node::name)
            .def_readonly("edge_length", &Node::edge_length)
            .def_readonly("cumulative_edge_length", &Node::cumulative_edge_length)
            ;

    py::enum_<Node::LadderizeMethod>(m, "LadderizeMethod")
            .value("MaxEdgeLength", Node::LadderizeMethod::MaxEdgeLength)
            .value("NumberOfLeaves", Node::LadderizeMethod::NumberOfLeaves)
            .export_values()
            ;

    py::class_<Tree>(m, "Tree", py::base<Node>())
              //.def("json", static_cast<std::string (Tree::*)(int) const>(&Tree::json), py::arg("indent") = 0)
            .def("json", &Tree::json, py::arg("indent") = size_t(0))
            .def("ladderize", &Tree::ladderize, py::arg("method") = Node::LadderizeMethod::MaxEdgeLength)
            .def("make_hz_line_sections", &Tree::make_hz_line_sections, py::arg("tolerance"))
            .def("match_seqdb", &Tree::match_seqdb, py::arg("seqdb"))
            .def("clade_setup", &Tree::clade_setup)
            .def("make_aa_transitions", static_cast<void (Tree::*)()>(&Tree::make_aa_transitions))
            .def("make_aa_transitions", static_cast<void (Tree::*)(const std::vector<size_t>&)>(&Tree::make_aa_transitions), py::arg("positions"), py::doc("positions must be in the ascending order, positions are 0-based."))
            .def("aa_per_pos", &Tree::aa_per_pos)
            .def("find_name", &Tree::find_name, py::arg("name"), py::return_value_policy::reference, py::doc("Leaks memory, use for debugging only!"))
            .def("re_root", static_cast<void (Tree::*)(std::string)>(&Tree::re_root), py::arg("name"))
            .def("virus_type", &Tree::virus_type)
            .def("lineage", &Tree::lineage)
            .def("names", &Tree::names)
            .def("leaf_nodes_sorted_by_cumulative_edge_length", &Tree::leaf_nodes_sorted_by_cumulative_edge_length, py::return_value_policy::reference, py::doc("Leaks memory, use for debugging only!"))
            .def("add_vaccine", &Tree::add_vaccine, py::arg("id"), py::arg("label") = std::string())
            .def("settings", static_cast<Settings& (Tree::*)()>(&Tree::settings), py::return_value_policy::reference)
            ;

    m.def("import_tree", &import_tree, py::arg("data"), py::doc("Imports tree from newick or json string/file."));

      // ----------------------------------------------------------------------
      // Chart
      // ----------------------------------------------------------------------

    py::class_<Chart>(m, "Chart")
            ;

    m.def("import_chart", &import_chart, py::arg("data"), py::doc("Imports chart from a buffer or file in the sdb format."));

      // ----------------------------------------------------------------------
      // Drawing
      // ----------------------------------------------------------------------

    py::class_<Location>(m, "Location")
            .def(py::init<double, double>(), py::arg("x"), py::arg("y"))
            .def("__repr__", &Size::to_string)
            ;

    py::class_<Size>(m, "Size")
            .def(py::init<double, double>(), py::arg("width"), py::arg("height"))
            .def("__repr__", &Size::to_string)
            ;

    py::class_<Viewport>(m, "Viewport")
            .def(py::init<const Location&, const Size&>(), py::arg("origin"), py::arg("size"))
            .def("__repr__", &Size::to_string)
            ;

    py::class_<Color>(m, "Color")
            .def(py::init<size_t>())
            .def("__repr__", &Size::to_string)
            ;
      // py::implicitly_convertible<int, Color>();
    py::implicitly_convertible<size_t, Color>();

    py::class_<TextStyle>(m, "TextStyle")
            .def(py::init<>())
            ;

    py::enum_<cairo_line_cap_t>(m, "LineCap")
            .value("LINE_CAP_BUTT", CAIRO_LINE_CAP_BUTT)
            .value("LINE_CAP_ROUND", CAIRO_LINE_CAP_ROUND)
            .value("LINE_CAP_SQUARE", CAIRO_LINE_CAP_SQUARE)
            .export_values()
            ;

    py::class_<Text>(m, "Text")
            .def(py::init<const Location&, std::string, Color, double, const TextStyle&, double>(), py::arg("origin"), py::arg("text"), py::arg("color"), py::arg("size"), py::arg("style") = TextStyle(), py::arg("rotation") = 0.0)
            .def("draw", &Text::draw, py::arg("surface"), py::arg("viewport"))
            ;

    py::class_<Surface>(m, "Surface")
            .def(py::init<std::string, double, double>(), py::arg("filename"), py::arg("width"), py::arg("height"))
            .def("line", &Surface::line, py::arg("a"), py::arg("b"), py::arg("color"), py::arg("width"), py::arg("line_cap") = CAIRO_LINE_CAP_BUTT)
            .def("double_arrow", &Surface::double_arrow, py::arg("a"), py::arg("b"), py::arg("color"), py::arg("line_width"), py::arg("arrow_width"))
            .def("text", static_cast<void (Surface::*)(const Location&, std::string, Color, double, const TextStyle&, double)>(&Surface::text), py::arg("a"), py::arg("text"), py::arg("color"), py::arg("size"), py::arg("style") = TextStyle(), py::arg("rotation") = 0.0)
            .def("text", static_cast<void (Surface::*)(const Text&, const Viewport&)>(&Surface::text), py::arg("text"), py::arg("viewport"))
            .def("text_size", static_cast<Size (Surface::*)(std::string, double, const TextStyle&)>(&Surface::text_size), py::arg("text"), py::arg("size"), py::arg("style") = TextStyle()) //, py::arg("x_bearing") = static_cast<double*>(0))
            ;

      // ----------------------------------------------------------------------
      // Signature page
      // ----------------------------------------------------------------------

    py::class_<Settings>(m, "Settings")
            .def_readwrite("draw_tree", &Settings::draw_tree)
            .def_readwrite("signature_page", &Settings::signature_page)
            ;

    py::class_<SettingsDrawTree>(m, "SettingsDrawTree")
            .def_readwrite("grid_step", &SettingsDrawTree::grid_step)
            .def_readwrite("aa_transition", &SettingsDrawTree::aa_transition)
            ;

    py::class_<SettingsAATransition>(m, "SettingsAATransition")
            .def_readwrite("show_node_for_left_line", &SettingsAATransition::show_node_for_left_line)
            .def_readwrite("show_empty_left", &SettingsAATransition::show_empty_left)
            .def_readwrite("number_strains_threshold", &SettingsAATransition::number_strains_threshold)
            ;

    py::class_<SettingsSignaturePage>(m, "SettingsSignaturePage")
            .def_readwrite("pdf_height", &SettingsSignaturePage::pdf_height)
            .def_readwrite("pdf_aspect_ratio", &SettingsSignaturePage::pdf_aspect_ratio)
            ;

      // ----------------------------------------------------------------------

    py::class_<DrawTree>(m, "DrawTree")
            .def(py::init<>())
              // .def("prepare", &DrawTree::prepare, py::arg("tree"), py::arg("HzLineSections"), py::return_value_policy::take_ownership)
            .def("color_by_continent", &DrawTree::color_by_continent, py::arg("color_by_continent"), py::return_value_policy::take_ownership)
            .def("color_by_pos", &DrawTree::color_by_pos, py::arg("pos"), py::return_value_policy::take_ownership)
            .def("draw", &DrawTree::draw, py::arg("tree"), py::arg("surface"), py::arg("viewport"), py::arg("settings"))
            ;


    py::enum_<int>(m, "Show")
            .value("Title", SignaturePage::ShowTitle)
            .value("Tree", SignaturePage::ShowTree)
            .value("Legend", SignaturePage::ShowLegend)
            .value("TimeSeries", SignaturePage::ShowTimeSeries)
            .value("Clades", SignaturePage::ShowClades)
            .value("AntigenicMaps", SignaturePage::ShowAntigenicMaps)
            .export_values()
            ;

    py::class_<SignaturePage>(m, "SignaturePage")
            .def(py::init<>())
            .def("select_parts", &SignaturePage::select_parts, py::arg("parts"), py::return_value_policy::take_ownership)
            .def("title", &SignaturePage::title, py::arg("title"), py::return_value_policy::take_ownership)
            .def("color_by_continent", &SignaturePage::color_by_continent, py::arg("color_by_continent"), py::return_value_policy::take_ownership)
            .def("color_by_pos", &SignaturePage::color_by_pos, py::arg("pos"), py::return_value_policy::take_ownership)
            .def("prepare", &SignaturePage::prepare, py::arg("tree"), py::arg("surface"), py::arg("chart") = static_cast<Chart*>(nullptr), py::return_value_policy::take_ownership)
            .def("draw", &SignaturePage::draw, py::arg("tree"), py::arg("surface"), py::arg("chart") = static_cast<Chart*>(nullptr))
            ;

      // ----------------------------------------------------------------------

    return m.ptr();
}

// ----------------------------------------------------------------------
