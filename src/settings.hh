#pragma once

#include "json-read.hh"
#include "json-write.hh"
#include "draw.hh"
#include "date.hh"

// ----------------------------------------------------------------------

class Clades;
class Node;

// ----------------------------------------------------------------------

class SettingsAATransition
{
 public:
    static constexpr const char* json_name = "aa_transition";

 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsAATransition& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                using namespace jsonr;
                return (skey(json_name) > object(
                      object_value("size", mSettings.data.size)
                    | object_string_value("color", mSettings.data.color)
                    | object_value("style", mSettings.data.style)
                    | object_value("interline", mSettings.data.interline)
                    | object_value("show_empty_left", mSettings.show_empty_left)
                    | object_value("show_node_for_left_line", mSettings.show_node_for_left_line)
                    | object_string_value("node_for_left_line_color", mSettings.node_for_left_line_color)
                    | object_value("node_for_left_line_width", mSettings.node_for_left_line_width)
                    | object_value("number_strains_threshold", mSettings.number_strains_threshold)
                    | object_string_ignore_value("?per_branch")
                    | object_value("per_branch", mSettings.per_branch)
                    | object_string_ignore_value("?")
                      ))(i1, i2);
            }

          private:
            SettingsAATransition& mSettings;
        };

 public:
    class TransitionData
    {
     public:
        inline TransitionData(bool empty = true) : size(empty ? -1 : 8), color(empty ? COLOR_NOT_SET : BLACK), style(FontStyle::Monospace), interline(empty ? -1 : 1.2) {}
        inline TransitionData(std::string aBranchId, std::vector<std::string>&& aLabels) : TransitionData(true) { branch_id = aBranchId; labels = std::move(aLabels); }

        double size;
        Color color;
        TextStyle style;
        double interline;
        std::string branch_id;
        std::vector<std::string> labels;

        inline void update(const TransitionData& source)
            {
                if (source.size >= 0)
                    size = source.size;
                if (source.color != COLOR_NOT_SET)
                    color = source.color;
                style = source.style;
                if (source.interline > 0)
                    interline = source.interline;
                if (!source.labels.empty())
                    labels = source.labels;
            }

        inline jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
            {
                comma = jsonw::json_begin(target, comma, '{', indent, prefix);
                comma = jsonw::json(target, comma, "branch_id", branch_id, 0, prefix);
                comma = jsonw::json(target, comma, "labels", labels, 0, prefix);
                return  jsonw::json_end(target, '}', 0, prefix);
            }

        class json_parser_t AXE_RULE
            {
              public:
                inline json_parser_t(TransitionData& aData) : mData(aData) {}

                template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
                {
                    using namespace jsonr;
                    return (object(
                        object_value("size", mData.size)
                      | object_string_value("color", mData.color)
                      | object_value("style", mData.style)
                      | object_value("interline", mData.interline)
                      | object_value("branch_id", mData.branch_id)
                      | object_value("labels", mData.labels)
                      | object_string_ignore_value("?")
                        ))(i1, i2);
                }

              private:
                TransitionData& mData;
            };

        inline auto json_parser() { return json_parser_t(*this); }
    };

    inline SettingsAATransition() : data(false), show_empty_left(false), show_node_for_left_line(false), node_for_left_line_color(0x00FF00), node_for_left_line_width(1), number_strains_threshold(20) {}

    inline jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
        {
            comma = jsonw::json_begin(target, comma, '{', indent, prefix);
            comma = jsonw::json(target, comma, "size", data.size, indent, prefix);
            comma = jsonw::json(target, comma, "color", data.color, indent, prefix);
            comma = jsonw::json(target, comma, "style", data.style, indent, prefix);
            comma = jsonw::json(target, comma, "interline", data.interline, indent, prefix);
            if (!per_branch.empty()) {
                comma = jsonw::json(target, comma, "?per_branch", "add size, color, style, interline, if different from the default listed above.", indent, prefix);
                comma = jsonw::json(target, comma, "per_branch", per_branch, indent, prefix);
            }
            comma = jsonw::json(target, comma, "show_empty_left", show_empty_left, indent, prefix);
            comma = jsonw::json(target, comma, "show_node_for_left_line", show_node_for_left_line, indent, prefix);
            comma = jsonw::json(target, comma, "node_for_left_line_color", node_for_left_line_color, indent, prefix);
            comma = jsonw::json(target, comma, "node_for_left_line_width", node_for_left_line_width, indent, prefix);
            comma = jsonw::json(target, comma, "number_strains_threshold", number_strains_threshold, indent, prefix);
            return  jsonw::json_end(target, '}', indent, prefix);
        }

    inline auto json_parser() { return json_parser_t(*this); }

    inline void add(std::string branch_id, const std::vector<std::pair<std::string, const Node*>>& aLabels)
        {
            if (!std::any_of(per_branch.begin(), per_branch.end(), [&branch_id](const auto& e) { return branch_id == e.branch_id; })) {
                std::vector<std::string> labels;
                std::transform(aLabels.begin(), aLabels.end(), std::back_inserter(labels), [](const auto& e) -> std::string { return e.first; });
                per_branch.emplace_back(branch_id, std::move(labels));
            }
        }

    inline TransitionData for_branch(std::string aBranchId) const
        {
            TransitionData r = data;
            const auto for_branch = find_if(per_branch.begin(), per_branch.end(), [&aBranchId](const auto& e) { return e.branch_id == aBranchId; });
            if (for_branch != per_branch.end())
                r.update(*for_branch);
            return r;
        }

    TransitionData data;
    std::vector<TransitionData> per_branch;
    bool show_empty_left;
    bool show_node_for_left_line;
    Color node_for_left_line_color;
    double node_for_left_line_width;
    size_t number_strains_threshold; // Do not show aa transition label if number_strains (leaf nodes) for the branch is less than this value.

}; // class SettingsAATransition

// ----------------------------------------------------------------------

class SettingsVaccineOnTree
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsVaccineOnTree& aVaccine) : mVaccine(aVaccine) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                using namespace jsonr;
                return (object(
                    object_value("id", mVaccine.id)
                  | object_value("label", mVaccine.label)
                  | object_value("label_style", mVaccine.label_style)
                  | object_value("label_size", mVaccine.label_size)
                  | object_value("label_offset_x", mVaccine.label_offset_x)
                  | object_value("label_offset_y", mVaccine.label_offset_y)
                  | object_string_value("label_color", mVaccine.label_color)
                  | object_string_value("line_color", mVaccine.line_color)
                  | object_value("line_width", mVaccine.line_width)
                  | object_string_ignore_value("?")
                    ))(i1, i2);
            }

          private:
            SettingsVaccineOnTree& mVaccine;
        };

 public:
    inline SettingsVaccineOnTree() : label_size(10), label_offset_x(-50), label_offset_y(50), label_color(BLACK), line_color(BLACK), line_width(1) {}
    inline SettingsVaccineOnTree(std::string aId, std::string aLabel) : id(aId), label(aLabel), label_size(10), label_offset_x(-50), label_offset_y(50), label_color(BLACK), line_color(BLACK), line_width(1) {}

    inline jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
        {
            comma = jsonw::json_begin(target, comma, '{', indent, prefix);
            comma = jsonw::json(target, comma, "id", id, 0, prefix);
            comma = jsonw::json(target, comma, "label", label, 0, prefix);
            comma = jsonw::json(target, comma, "label_style", label_style, 0, prefix);
            comma = jsonw::json(target, comma, "label_size", label_size, 0, prefix);
            comma = jsonw::json(target, comma, "label_offset_x", label_offset_x, 0, prefix);
            comma = jsonw::json(target, comma, "label_offset_y", label_offset_y, 0, prefix);
            comma = jsonw::json(target, comma, "label_color", label_color, 0, prefix);
            comma = jsonw::json(target, comma, "line_color", line_color, 0, prefix);
            comma = jsonw::json(target, comma, "line_width", line_width, 0, prefix);
            return  jsonw::json_end(target, '}', 0, prefix);
        }

    inline auto json_parser() { return json_parser_t(*this); }

    std::string id;
    std::string label;
    TextStyle label_style;
    double label_size;
    double label_offset_x;
    double label_offset_y;
    Color label_color;
    Color line_color;
    double line_width;

}; // class SettingsVaccineOnTree

// ----------------------------------------------------------------------

class SettingsVaccinesOnTree : public std::vector<SettingsVaccineOnTree>
{
 public:
    static constexpr const char* json_name = "vaccines";

 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsVaccinesOnTree& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                using namespace jsonr;
                return (skey(json_name) > object(
                      object_value(SettingsVaccinesOnTree::json_name, static_cast<std::vector<SettingsVaccineOnTree>&>(mSettings))
                    | object_string_ignore_value("?")
                      ))(i1, i2);
            }

          private:
            SettingsVaccinesOnTree& mSettings;
        };

 public:
    inline SettingsVaccinesOnTree() {}

    inline jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
        {
            comma = jsonw::json_begin(target, comma, '{', indent, prefix);
            comma = jsonw::json(target, comma, SettingsVaccinesOnTree::json_name, static_cast<const std::vector<SettingsVaccineOnTree>&>(*this), indent, prefix);
            return  jsonw::json_end(target, '}', indent, prefix);
        }

    inline auto json_parser() { return json_parser_t(*this); }

}; // class SettingsVaccinesOnTree

// ----------------------------------------------------------------------

class HzLineSection
{
 private:
    constexpr static size_t LINE_NOT_SET = static_cast<size_t>(-1);

    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(HzLineSection& aSection) : mSection(aSection) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                using namespace jsonr;
                return (object(
                    object_value("first_name", mSection.first_name)
                  | object_value("first_line", mSection.first_line)
                  | object_string_value("color", mSection.color)
                  | object_value("line_width", mSection.line_width)
                  | object_string_ignore_value("?")
                    ))(i1, i2);
            }

          private:
            HzLineSection& mSection;
        };

 public:
    inline HzLineSection() : first_line(LINE_NOT_SET), color(COLOR_NOT_SET), line_width(2) {}
    inline HzLineSection(std::string aFirstName, size_t aFirstLine, Color aColor = COLOR_NOT_SET)
        : first_name(aFirstName), first_line(aFirstLine), color(aColor), line_width(2) {}
    HzLineSection(const Node& aNode, Color aColor = COLOR_NOT_SET);

    inline jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
        {
            comma = jsonw::json_begin(target, comma, '{', indent, prefix);
            comma = jsonw::json_if(target, comma, "first_name", first_name, 0, prefix);
            // if (first_line != LINE_NOT_SET)
            //     comma = jsonw::json(target, comma, "first_line", first_line, 0, prefix);
            comma = jsonw::json(target, comma, "color", color, 0, prefix);
            comma = jsonw::json(target, comma, "line_width", line_width, 0, prefix);
            return  jsonw::json_end(target, '}', 0, prefix);
        }

    inline auto json_parser() { return json_parser_t(*this); }

    std::string first_name;
    size_t first_line;
    Color color;
    double line_width;

}; // class HzLineSection

class HzLineSections : public std::vector<HzLineSection>
{
 private:
    constexpr static size_t LINE_NOT_SET = static_cast<size_t>(-1);

    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(HzLineSections& aSections) : mSections(aSections) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                using namespace jsonr;
                return (skey(HzLineSections::json_name) > object(
                    object_value("hz_line_width", mSections.hz_line_width)
                  | object_string_value("hz_line_color", mSections.hz_line_color)
                  | object_value("sequenced_antigen_line_show", mSections.sequenced_antigen_line_show)
                  | object_value("sequenced_antigen_line_width", mSections.sequenced_antigen_line_width)
                  | object_value("sequenced_antigen_line_length", mSections.sequenced_antigen_line_length)
                  | object_string_value("sequenced_antigen_line_color", mSections.sequenced_antigen_line_color)
                  | object_value(HzLineSections::json_name, static_cast<std::vector<HzLineSection>&>(mSections))
                  | object_string_ignore_value("?")
                    ))(i1, i2);
            }

          private:
            HzLineSections& mSections;
        };

 public:
    static constexpr const char* json_name = "hz_line_sections";

    inline HzLineSections() : hz_line_width(0.5), hz_line_color(GREY), sequenced_antigen_line_show(true), sequenced_antigen_line_width(0.5), sequenced_antigen_line_length(5), sequenced_antigen_line_color(GREY) {}

    inline void sort()
        {
            std::sort(begin(), end(), [](const auto& a, const auto& b) { return a.first_line < b.first_line; });
        }

    inline jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
        {
            comma = jsonw::json_begin(target, comma, '{', indent, prefix);
            comma = jsonw::json(target, comma, "hz_line_width", hz_line_width, indent, prefix);
            comma = jsonw::json(target, comma, "hz_line_color", hz_line_color, indent, prefix);
            comma = jsonw::json(target, comma, "sequenced_antigen_line_show", sequenced_antigen_line_show, indent, prefix);
            comma = jsonw::json(target, comma, "sequenced_antigen_line_width", sequenced_antigen_line_width, indent, prefix);
            comma = jsonw::json(target, comma, "sequenced_antigen_line_length", sequenced_antigen_line_length, indent, prefix);
            comma = jsonw::json(target, comma, "sequenced_antigen_line_color", sequenced_antigen_line_color, indent, prefix);
            comma = jsonw::json(target, comma, HzLineSections::json_name, static_cast<const std::vector<HzLineSection>&>(*this), indent, prefix);
            return  jsonw::json_end(target, '}', indent, prefix);
        }

    inline auto json_parser() { return json_parser_t(*this); }

    double hz_line_width;
    Color hz_line_color;
    bool sequenced_antigen_line_show;
    double sequenced_antigen_line_width, sequenced_antigen_line_length;
    Color sequenced_antigen_line_color;

}; // class HzLineSections

// ----------------------------------------------------------------------

class SettingsDrawTree
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsDrawTree& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                using namespace jsonr;
                return (skey("tree") > object(
                    object_double_non_negative_value("root_edge", mSettings.root_edge)
                  | object_string_value("line_color", mSettings.line_color)
                  | object_double_non_negative_value("line_width", mSettings.line_width)
                  | object_double_value("name_offset", mSettings.name_offset)
                  | object_value("label_style", mSettings.label_style)
                  | mSettings.aa_transition.json_parser()
                  | mSettings.vaccines.json_parser()
                  | object_value("grid_step", mSettings.grid_step)
                  | object_string_value("grid_color", mSettings.grid_color)
                  | object_double_non_negative_value("grid_width", mSettings.grid_width)
                  | mSettings.hz_line_sections.json_parser()
                  | object_string_ignore_value("?")
                    ))(i1, i2);
            }

          private:
            SettingsDrawTree& mSettings;
        };

 public:
    inline SettingsDrawTree()
        : root_edge(0), line_color(0), line_width(1), label_style(), name_offset(0.2), grid_step(0), grid_color(0x80000000), grid_width(0.3)
           {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    double root_edge;
    Color line_color;
    double line_width;
    TextStyle label_style;
    double name_offset;
    SettingsAATransition aa_transition;
    SettingsVaccinesOnTree vaccines;
    size_t grid_step;           // 0 - no grid, N - use N*vertical_step as grid cell height
    Color grid_color;
    double grid_width;
    HzLineSections hz_line_sections;

}; // class SettingsDrawTree

// ----------------------------------------------------------------------

class SettingsLegend
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsLegend& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                auto r_font_size = jsonr::object_double_non_negative_value("font_size", mSettings.font_size);
                auto r_interline = jsonr::object_double_non_negative_value("interline", mSettings.interline);
                auto r_style = jsonr::object_value("style", mSettings.style);
                auto r_comment = jsonr::object_string_ignore_value("?");
                return (jsonr::skey("legend") > jsonr::object(r_font_size | r_interline | r_style | r_comment))(i1, i2);
            }

          private:
            SettingsLegend& mSettings;
        };

 public:
    inline SettingsLegend()
        : font_size(14), interline(1.2), style(FontStyle::Monospace) {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    double font_size;
    double interline;
    TextStyle style;

}; // class SettingsLegend

// ----------------------------------------------------------------------

class SettingsSignaturePage
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsSignaturePage& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                auto r_outer_padding = jsonr::object_double_non_negative_value("outer_padding", mSettings.outer_padding);
                auto r_tree_time_series_space = jsonr::object_double_non_negative_value("tree_time_series_space", mSettings.tree_time_series_space);
                auto r_time_series_clades_space = jsonr::object_double_non_negative_value("time_series_clades_space", mSettings.time_series_clades_space);
                auto r_clades_antigenic_maps_space = jsonr::object_double_non_negative_value("clades_antigenic_maps_space", mSettings.clades_antigenic_maps_space);
                auto r_comment = jsonr::object_string_ignore_value("?");
                return (jsonr::skey("signature_page") > jsonr::object(r_outer_padding | r_tree_time_series_space | r_time_series_clades_space | r_clades_antigenic_maps_space | r_comment))(i1, i2);
            }

          private:
            SettingsSignaturePage& mSettings;
        };

 public:
    inline SettingsSignaturePage()
        : outer_padding(0.01), tree_time_series_space(0), time_series_clades_space(0.01), clades_antigenic_maps_space(0.02) {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    double outer_padding;             // relative to the canvas width
    double tree_time_series_space;    // relative to the canvas width
    double time_series_clades_space;  // relative to the canvas width
    double clades_antigenic_maps_space; // relative to the canvas width

}; // class SettingsSignaturePage

// ----------------------------------------------------------------------

class SettingsTimeSeries
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsTimeSeries& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                auto r_begin = jsonr::object_string_value("begin", mSettings.begin);
                auto r_end = jsonr::object_string_value("end", mSettings.end);
                auto r_dash_width = jsonr::object_double_non_negative_value("dash_width", mSettings.dash_width);
                auto r_dash_line_width = jsonr::object_double_non_negative_value("dash_line_width", mSettings.dash_line_width);
                auto r_max_number_of_months = jsonr::object_int_value("max_number_of_months", mSettings.max_number_of_months, 10, &jsonr::validator_size_t_positive);
                auto r_month_label_scale = jsonr::object_double_non_negative_value("month_label_scale", mSettings.month_label_scale);
                auto r_month_separator_color = jsonr::object_string_value("month_separator_color", mSettings.month_separator_color);
                auto r_month_separator_width = jsonr::object_double_non_negative_value("month_separator_width", mSettings.month_separator_width);
                auto r_month_width = jsonr::object_double_non_negative_value("month_width", mSettings.month_width);
                auto r_month_label_style = jsonr::object_value("month_label_style", mSettings.month_label_style);
                auto r_comment = jsonr::object_string_ignore_value("?");
                return (jsonr::skey("time_series") > jsonr::object(r_begin | r_end | r_dash_width | r_dash_line_width | r_max_number_of_months | r_month_label_scale | r_month_separator_color | r_month_separator_width | r_month_width | r_month_label_style | r_comment))(i1, i2);
            }

          private:
            SettingsTimeSeries& mSettings;
        };

 public:
    inline SettingsTimeSeries()
        : dash_width(0.5), dash_line_width(1), max_number_of_months(20), month_label_scale(0.9), month_separator_color(0),
          month_separator_width(0.1), month_width(10) {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    Date begin;
    Date end;
    double dash_width;          // relative to month_width
    double dash_line_width;          // relative to month_width
    size_t max_number_of_months;
    double month_label_scale;
    TextStyle month_label_style;
    Color month_separator_color;
    double month_separator_width;
    double month_width;

}; // class SettingsTimeSeries

// ----------------------------------------------------------------------

enum class CladeLabelPosition { Middle, Top, Bottom };

inline std::string to_string(CladeLabelPosition a)
{
    switch (a) {
      case CladeLabelPosition::Middle: return "middle";
      case CladeLabelPosition::Top: return "top";
      case CladeLabelPosition::Bottom: return "bottom";
    }
    return "middle";            // to shut compiler up
}

inline CladeLabelPosition CladeLabelPosition_from_string(std::string source)
{
    if (source == "middle") return CladeLabelPosition::Middle;
    if (source == "top") return CladeLabelPosition::Top;
    if (source == "bottom") return CladeLabelPosition::Bottom;
    throw std::invalid_argument("cannot parse CladeLabelPosition from " + source);
}

// ----------------------------------------------------------------------

class SettingsClade
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsClade& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                auto r_show = jsonr::object_value("show", mSettings.show);
                auto r_begin = jsonr::object_int_value("begin", mSettings.begin);
                auto r_end = jsonr::object_int_value("end", mSettings.end);
                auto r_id = jsonr::object_value("id", mSettings.id);
                auto r_label = jsonr::object_value("label", mSettings.label);
                auto r_slot = jsonr::object_int_value("slot", mSettings.slot);
                auto r_label_position = jsonr::object_enum_value("label_position", mSettings.label_position, &CladeLabelPosition_from_string);
                auto r_label_position_offset = jsonr::object_double_value("label_position_offset", mSettings.label_position_offset);
                auto r_label_rotation = jsonr::object_double_value("label_rotation", mSettings.label_rotation);
                auto r_label_offset = jsonr::object_double_value("label_offset", mSettings.label_offset);
                auto r_comment = jsonr::object_string_ignore_value("?");
                return jsonr::object(r_show | r_begin | r_end | r_id | r_slot | r_label_position_offset | r_label_position | r_label_rotation | r_label_offset | r_label | r_comment)(i1, i2);
            }

          private:
            SettingsClade& mSettings;
        };

 public:
    inline SettingsClade()
        : show(false), begin(-1), end(-1), slot(-1), label_position(CladeLabelPosition::Middle),
          label_position_offset(0.0), label_rotation(0.0), label_offset(3.0) {}
    inline SettingsClade(int aBegin, int aEnd, std::string aLabel, std::string aId)
        : show(true), begin(aBegin), end(aEnd), label(aLabel), id(aId), slot(-1), label_position(CladeLabelPosition::Middle),
          label_position_offset(0.0), label_rotation(0.0), label_offset(3.0) {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    void update(const SettingsClade& source);

    bool show;
    int begin;
    int end;
    std::string label;
    std::string id;
    int slot;
    CladeLabelPosition label_position;
    double label_position_offset;
    double label_rotation;
    double label_offset;

}; // class SettingsClade

// ----------------------------------------------------------------------

class SettingsClades
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsClades& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                auto r_slot_width = jsonr::object_double_non_negative_value("slot_width", mSettings.slot_width);
                auto r_arrow_color = jsonr::object_string_value("arrow_color", mSettings.arrow_color);
                auto r_arrow_extra = jsonr::object_double_non_negative_value("arrow_extra", mSettings.arrow_extra);
                auto r_arrow_width = jsonr::object_double_non_negative_value("arrow_width", mSettings.arrow_width);
                auto r_line_width = jsonr::object_double_non_negative_value("line_width", mSettings.line_width);
                auto r_label_color = jsonr::object_string_value("label_color", mSettings.label_color);
                auto r_label_size = jsonr::object_double_non_negative_value("label_size", mSettings.label_size);
                auto r_separator_color = jsonr::object_string_value("separator_color", mSettings.separator_color);
                auto r_separator_width = jsonr::object_double_non_negative_value("separator_width", mSettings.separator_width);
                auto r_label_style = jsonr::object_value("label_style", mSettings.label_style);
                auto r_per_clade = jsonr::object_array_value("per_clade", mSettings.per_clade);
                auto r_comment = jsonr::object_string_ignore_value("?");
                return (jsonr::skey("clades") > jsonr::object(r_slot_width | r_arrow_color | r_arrow_extra | r_arrow_width | r_line_width | r_label_color | r_label_size | r_separator_color | r_separator_width | r_label_style | r_per_clade | r_comment))(i1, i2);
            }

          private:
            SettingsClades& mSettings;
        };

 public:
    inline SettingsClades()
        : slot_width(5), arrow_color(BLACK), arrow_extra(0.5), arrow_width(3), line_width(1),
          label_color(0), label_size(10), separator_color(GREY), separator_width(1) {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    void extract(const Clades& aClades);

    double slot_width;
    Color arrow_color;
    double arrow_extra;
    double arrow_width;
    double line_width;
    Color label_color;
    double label_size;
    TextStyle label_style;
    Color separator_color;
    double separator_width;
    std::vector<SettingsClade> per_clade;

}; // class SettingsClades

// ----------------------------------------------------------------------

class SettingsAntigenicMaps
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(SettingsAntigenicMaps& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                using namespace jsonr;
                return (jsonr::skey("antigenic_maps") > jsonr::object(
                      object_double_non_negative_value("border_width", mSettings.border_width)
                    | object_string_value("border_color", mSettings.border_color)
                    | object_double_non_negative_value("grid_line_width", mSettings.grid_line_width)
                    | object_string_value("grid_color", mSettings.grid_color)
                    | object_value("gap_between_maps", mSettings.gap_between_maps)
                    | object_value("map_zoom", mSettings.map_zoom)
                    | object_value("egg_antigen_aspect", mSettings.egg_antigen_aspect)
                    | object_value("reassortant_rotation", mSettings.reassortant_rotation)
                    | object_value("serum_scale", mSettings.serum_scale)
                    | object_value("reference_antigen_scale", mSettings.reference_antigen_scale)
                    | object_value("test_antigen_scale", mSettings.test_antigen_scale)
                    | object_value("vaccine_antigen_scale", mSettings.vaccine_antigen_scale)
                    | object_value("tracked_antigen_scale", mSettings.tracked_antigen_scale)
                    | object_value("serum_outline_width", mSettings.serum_outline_width)
                    | object_value("reference_antigen_outline_width", mSettings.reference_antigen_outline_width)
                    | object_value("test_antigen_outline_width", mSettings.test_antigen_outline_width)
                    | object_value("vaccine_antigen_outline_width", mSettings.vaccine_antigen_outline_width)
                    | object_value("tracked_antigen_outline_width", mSettings.tracked_antigen_outline_width)
                    | object_string_value("serum_outline_color", mSettings.serum_outline_color)
                    | object_string_value("reference_antigen_outline_color", mSettings.reference_antigen_outline_color)
                    | object_string_value("test_antigen_outline_color", mSettings.test_antigen_outline_color)
                    | object_string_value("test_antigen_fill_color", mSettings.test_antigen_fill_color)
                    | object_string_value("vaccine_antigen_outline_color", mSettings.vaccine_antigen_outline_color)
                    | object_string_value("tracked_antigen_outline_color", mSettings.tracked_antigen_outline_color)
                    | object_value("sequenced_antigen_outline_width", mSettings.sequenced_antigen_outline_width)
                    | object_string_value("sequenced_antigen_outline_color", mSettings.sequenced_antigen_outline_color)
                    | object_string_value("sequenced_antigen_fill_color", mSettings.sequenced_antigen_fill_color)
                    | object_string_ignore_value("?")
                      ))(i1, i2);
            }

          private:
            SettingsAntigenicMaps& mSettings;
        };

 public:
    inline SettingsAntigenicMaps()
        : border_width(1), grid_line_width(0.5), border_color(BLACK), grid_color(GREY), gap_between_maps(0.005), map_zoom(1.1),
          serum_scale(5), reference_antigen_scale(8), test_antigen_scale(5), vaccine_antigen_scale(15), tracked_antigen_scale(8),
          serum_outline_width(0.5), reference_antigen_outline_width(0.5), test_antigen_outline_width(0.5), vaccine_antigen_outline_width(0.5),
          sequenced_antigen_outline_width(0.5), tracked_antigen_outline_width(0.5),
          serum_outline_color(LIGHT_GREY), reference_antigen_outline_color(LIGHT_GREY), test_antigen_outline_color(LIGHT_GREY),
          test_antigen_fill_color(LIGHT_GREY), vaccine_antigen_outline_color(BLACK), sequenced_antigen_outline_color(BLACK), sequenced_antigen_fill_color(LIGHT_GREY),
          tracked_antigen_outline_color(BLACK),
          egg_antigen_aspect(0.75), reassortant_rotation(M_PI / 6.0)
        {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    double border_width, grid_line_width;
    Color border_color, grid_color;
    double gap_between_maps;    // relative to canvas width
    double map_zoom;
    double serum_scale, reference_antigen_scale, test_antigen_scale, vaccine_antigen_scale, tracked_antigen_scale;
    double serum_outline_width, reference_antigen_outline_width, test_antigen_outline_width, vaccine_antigen_outline_width, sequenced_antigen_outline_width, tracked_antigen_outline_width;
    Color serum_outline_color, reference_antigen_outline_color, test_antigen_outline_color, test_antigen_fill_color, vaccine_antigen_outline_color, sequenced_antigen_outline_color, sequenced_antigen_fill_color, tracked_antigen_outline_color;
    double egg_antigen_aspect, reassortant_rotation;

}; // class SettingsAntigenicMaps

// ----------------------------------------------------------------------

class Settings
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(Settings& aSettings) : mSettings(aSettings) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                using namespace jsonr;
                return (skey("settings") > object(
                            mSettings.draw_tree.json_parser()
                            | mSettings.legend.json_parser()
                            | mSettings.signature_page.json_parser()
                            | mSettings.time_series.json_parser()
                            | mSettings.clades.json_parser()
                            | mSettings.antigenic_maps.json_parser())
                        )(i1, i2);
            }

          private:
            Settings& mSettings;
        };

 public:
    inline Settings() {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    SettingsSignaturePage signature_page;
    SettingsDrawTree draw_tree;
    SettingsLegend legend;
    SettingsTimeSeries time_series;
    SettingsClades clades;
    SettingsAntigenicMaps antigenic_maps;

}; // class Settings

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
