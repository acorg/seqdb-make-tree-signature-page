#pragma once

#include "json-read.hh"
#include "json-write.hh"
#include "draw.hh"
#include "date.hh"

// ----------------------------------------------------------------------

class Clades;

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
                auto r_size = jsonr::object_value("size", mSettings.size);
                auto r_color = jsonr::object_string_value("color", mSettings.color);
                auto r_style = jsonr::object_value("style", mSettings.style);
                auto r_interline = jsonr::object_value("interline", mSettings.interline);
                auto r_show_empty_left = jsonr::object_value("show_empty_left", mSettings.show_empty_left);
                auto r_show_node_for_left_line = jsonr::object_value("show_node_for_left_line", mSettings.show_node_for_left_line);
                auto r_node_for_left_line_color = jsonr::object_string_value("node_for_left_line_color", mSettings.node_for_left_line_color);
                auto r_node_for_left_line_width = jsonr::object_value("node_for_left_line_width", mSettings.node_for_left_line_width);
                auto r_comment = jsonr::object_string_ignore_value("?");
                return (jsonr::skey(json_name) > jsonr::object(r_size | r_color | r_style | r_interline | r_show_empty_left | r_show_node_for_left_line | r_node_for_left_line_color | r_node_for_left_line_width | r_comment))(i1, i2);
            }

          private:
            SettingsAATransition& mSettings;
        };

 public:
    inline SettingsAATransition()
        : size(8), color(0), style(FontStyle::Monospace), interline(1.2), show_empty_left(false),
          show_node_for_left_line(false), node_for_left_line_color(0x00FF00), node_for_left_line_width(1) {}

    inline jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
        {
            comma = jsonw::json_begin(target, comma, '{', indent, prefix);
            comma = jsonw::json(target, comma, "size", size, indent, prefix);
            comma = jsonw::json(target, comma, "color", color, indent, prefix);
            comma = jsonw::json(target, comma, "style", style, indent, prefix);
            comma = jsonw::json(target, comma, "interline", interline, indent, prefix);
            comma = jsonw::json(target, comma, "show_empty_left", show_empty_left, indent, prefix);
            comma = jsonw::json(target, comma, "show_node_for_left_line", show_node_for_left_line, indent, prefix);
            comma = jsonw::json(target, comma, "node_for_left_line_color", node_for_left_line_color, indent, prefix);
            comma = jsonw::json(target, comma, "node_for_left_line_width", node_for_left_line_width, indent, prefix);
            return  jsonw::json_end(target, '}', indent, prefix);
        }

    inline auto json_parser() { return json_parser_t(*this); }

    double size;
    Color color;
    TextStyle style;
    double interline;
    bool show_empty_left;
    bool show_node_for_left_line;
    Color node_for_left_line_color;
    double node_for_left_line_width;

}; // class SettingsAATransition

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
                auto r_root_edge = jsonr::object_double_non_negative_value("root_edge", mSettings.root_edge);
                auto r_line_color = jsonr::object_string_value("line_color", mSettings.line_color);
                auto r_line_width = jsonr::object_double_non_negative_value("line_width", mSettings.line_width);
                auto r_name_offset = jsonr::object_double_value("name_offset", mSettings.name_offset);
                auto r_label_style = jsonr::object_value("label_style", mSettings.label_style);
                auto r_aa_transition = mSettings.aa_transition.json_parser();
                auto r_grid_step = jsonr::object_value("grid_step", mSettings.grid_step);
                auto r_grid_color = jsonr::object_string_value("grid_color", mSettings.grid_color);
                auto r_grid_width = jsonr::object_double_non_negative_value("grid_width", mSettings.grid_width);
                auto r_comment = jsonr::object_string_ignore_value("?");
                return (jsonr::skey("tree") > jsonr::object(r_root_edge | r_line_color | r_line_width | r_name_offset | r_label_style | r_aa_transition | r_grid_step | r_grid_color | r_grid_width | r_comment))(i1, i2);
            }

          private:
            SettingsDrawTree& mSettings;
        };

 public:
    inline SettingsDrawTree()
        : root_edge(0), line_color(0), line_width(1), label_style(), name_offset(0.2), grid_step(0), grid_color(0x80000000), grid_width(0.3) {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    double root_edge;
    Color line_color;
    double line_width;
    TextStyle label_style;
    double name_offset;
    SettingsAATransition aa_transition;
    size_t grid_step;           // 0 - no grid, N - use N*vertical_step as grid cell height
    Color grid_color;
    double grid_width;

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

}; // class SettingsDrawTree

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
                auto r_comment = jsonr::object_string_ignore_value("?");
                return (jsonr::skey("signature_page") > jsonr::object(r_outer_padding | r_tree_time_series_space | r_time_series_clades_space | r_comment))(i1, i2);
            }

          private:
            SettingsSignaturePage& mSettings;
        };

 public:
    inline SettingsSignaturePage()
        : outer_padding(0.01), tree_time_series_space(0), time_series_clades_space(0) {}

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;

    inline auto json_parser() { return json_parser_t(*this); }

    double outer_padding;             // relative to the canvas width
    double tree_time_series_space;    // relative to the canvas width
    double time_series_clades_space;    // relative to the canvas width

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
        : slot_width(5), arrow_color(0), arrow_extra(0.5), arrow_width(3), line_width(1),
          label_color(0), label_size(10), separator_color(0x808080), separator_width(0.2) {}

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

    SettingsClade* current_clade; // bad solution but no idea how to make it better in the current design

}; // class SettingsClades

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
                return (jsonr::skey("settings") > jsonr::object(
                    mSettings.draw_tree.json_parser()
                  | mSettings.legend.json_parser()
                  | mSettings.signature_page.json_parser()
                  | mSettings.time_series.json_parser()
                  | mSettings.clades.json_parser())
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

}; // class Settings

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
