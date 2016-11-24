#pragma once

#include <limits>

#include "json-struct.hh"
#include "draw.hh"
#include "date.hh"

// ----------------------------------------------------------------------

class Clades;
class Node;

// ----------------------------------------------------------------------

class SettingsAATransition
{
 public:
    class TransitionData
    {
     public:
        inline TransitionData(bool empty = true)
            : size(empty ? -1 : 8), color(empty ? COLOR_NOT_SET : BLACK), style("Courier New"), interline(empty ? -1 : 1.2),
              label_offset_x(empty ? 0 : -40), label_offset_y(empty ? 0 : 20), label_connection_line_width(0.1), label_connection_line_color(BLACK) {}
        inline TransitionData(std::string aBranchId, std::vector<std::string>&& aLabels) : TransitionData(true) { branch_id = aBranchId; labels = std::move(aLabels); }

        double size;
        Color color;
        TextStyle style;
        double interline;
        std::string branch_id;
        std::vector<std::string> labels;
        double label_offset_x, label_offset_y;
        double label_connection_line_width;
        Color label_connection_line_color;

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

        friend inline auto json_fields(TransitionData& a)
        {
            return std::make_tuple(
                "label_connection_line_color", json::field(&a.label_connection_line_color, &Color::to_string, &Color::from_string, json::output_if_true),
                "label_connection_line_width", &a.label_connection_line_width,
                "branch_id", &a.branch_id,
                "labels", &a.labels,
                "size", &a.size,
                "color", json::field(&a.color, &Color::to_string, &Color::from_string, json::output_if_true),
                "style", json::field(&a.style, json::output_if_true),
                "interline", &a.interline
                                   );
        }
    };

    inline SettingsAATransition()
        : show(true), data(false), show_empty_left(false), show_node_for_left_line(false),
          node_for_left_line_color(0x00FF00), node_for_left_line_width(1), number_strains_threshold(20)
        {}

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

    bool show;                  // show aa-transition labels in the tree
    TransitionData data;
    std::vector<TransitionData> per_branch;
    bool show_empty_left;
    bool show_node_for_left_line;
    Color node_for_left_line_color;
    double node_for_left_line_width;
    size_t number_strains_threshold; // Do not show aa transition label if number_strains (leaf nodes) for the branch is less than this value.

    friend inline auto json_fields(SettingsAATransition& a)
        {
            return std::make_tuple(
                "show_empty_left", &a.show_empty_left,
                "show_node_for_left_line", &a.show_node_for_left_line,
                "node_for_left_line_color", json::field(&a.node_for_left_line_color, &Color::to_string, &Color::from_string),
                "node_for_left_line_width", &a.node_for_left_line_width,
                "number_strains_threshold", &a.number_strains_threshold,
                "?per_branch", json::comment("add size, color, style, interline, if different from the default listed above."),
                "per_branch", json::field(&a.per_branch, json::output_if_not_empty),
                "label_offset_x", &a.data.label_offset_x,
                "label_offset_y", &a.data.label_offset_y,
                "interline", &a.data.interline,
                "size", &a.data.size,
                "color", json::field(&a.data.color, &Color::to_string, &Color::from_string),
                "style", &a.data.style,
                "show", &a.show
                                   );
        }

}; // class SettingsAATransition

// ----------------------------------------------------------------------

class SettingsMarkNodeOnTree
{
 public:
    inline SettingsMarkNodeOnTree() : label_size(10), label_offset_x(-50), label_offset_y(50), label_color(BLACK), line_color(BLACK), line_width(1) {}
    inline SettingsMarkNodeOnTree(std::string aId, std::string aLabel) : id(aId), label(aLabel), label_size(10), label_offset_x(-50), label_offset_y(50), label_color(BLACK), line_color(BLACK), line_width(1) {}

    std::string id;
    std::string label;
    TextStyle label_style;
    double label_size;
    double label_offset_x;
    double label_offset_y;
    Color label_color;
    Color line_color;
    double line_width;

    friend inline auto json_fields(SettingsMarkNodeOnTree& a)
        {
            return std::make_tuple(
                "id", &a.id,
                "label_style", &a.label_style,
                "label_size", &a.label_size,
                "label_offset_x", &a.label_offset_x,
                "label_offset_y", &a.label_offset_y,
                "label_color", json::field(&a.label_color, &Color::to_string, &Color::from_string),
                "label", &a.label,
                "line_color", json::field(&a.line_color, &Color::to_string, &Color::from_string),
                "line_width", &a.line_width
                                   );
        }

}; // class SettingsMarkNodeOnTree

// ----------------------------------------------------------------------

class SettingsMarkNodesOnTree : public std::vector<SettingsMarkNodeOnTree>
{
 public:
    inline SettingsMarkNodesOnTree() {}
    void add(std::string aId, std::string aLabel);

}; // class SettingsMarkNodesOnTree

// ----------------------------------------------------------------------

class HzLineSection
{
 private:
    constexpr static size_t LINE_NOT_SET = static_cast<size_t>(-1);

 public:
    inline HzLineSection() : first_line(LINE_NOT_SET), color(COLOR_NOT_SET), line_width(2), show_map(true) {}
    // inline HzLineSection(std::string aFirstName, size_t aFirstLine, Color aColor = COLOR_NOT_SET)
    //     : first_name(aFirstName), first_line(aFirstLine), color(aColor), line_width(2) {}
    inline HzLineSection(std::string aFirstName, Color aColor = COLOR_NOT_SET)
        : first_name(aFirstName), first_line(LINE_NOT_SET), color(aColor), line_width(2), show_map(true) {}
    HzLineSection(const Node& aNode, Color aColor = COLOR_NOT_SET);

    std::string first_name;
    size_t first_line;
    Color color;
    double line_width;
    std::string label;          // for named_grid mode, empty to set automatically
    bool show_map;              // show antigenic map for this section, used to disable showing some maps

    friend inline auto json_fields(HzLineSection& a)
        {
            return std::make_tuple(
                "first_name", &a.first_name,
                  // "first_line", &a.first_line,
                "line_width", &a.line_width,
                "show_map", &a.show_map,
                "label", &a.label,
                "color", json::field(&a.color, &Color::to_string, &Color::from_string)
                                   );
        }

}; // class HzLineSection

// ----------------------------------------------------------------------

class HzLineSections : public std::vector<HzLineSection>
{
 public:
    inline HzLineSections() : hz_line_width(0.5), hz_line_color(GREY),
                              sequenced_antigen_line_show(true), sequenced_antigen_line_width(0.5), sequenced_antigen_line_length(5), sequenced_antigen_line_color(GREY),
                              this_section_antigen_color(0x75DB51),
                              connecting_pipe_border_color(BLACK), connecting_pipe_background_color(0xFFFFF8), connecting_pipe_border_width(1),
                              vertical_gap(25), section_label_color(BLACK), section_label_offset_x(-2), section_label_offset_y(2), section_label_size(10) {}

    inline void sort()
        {
            std::sort(begin(), end(), [](const auto& a, const auto& b) { return a.first_line < b.first_line; });
        }

    double hz_line_width;
    Color hz_line_color;
    bool sequenced_antigen_line_show;
    double sequenced_antigen_line_width, sequenced_antigen_line_length;
    Color sequenced_antigen_line_color;
    Color this_section_antigen_color; // BWVpos, NamedGrid mode only
    Color connecting_pipe_border_color, connecting_pipe_background_color; // BWVpos mode only
    double connecting_pipe_border_width; // BWVpos mode only
    size_t vertical_gap;
    Color section_label_color;
    double section_label_offset_x, section_label_offset_y, section_label_size;

 private:
    friend inline auto json_fields(HzLineSections& a)
        {
            return std::make_tuple(
                "this_section_antigen_color", json::field(&a.this_section_antigen_color, &Color::to_string, &Color::from_string),
                "connecting_pipe_border_color", json::field(&a.connecting_pipe_border_color, &Color::to_string, &Color::from_string),
                "connecting_pipe_background_color", json::field(&a.connecting_pipe_background_color, &Color::to_string, &Color::from_string),
                "connecting_pipe_border_width", &a.connecting_pipe_border_width,
                "sequenced_antigen_line_show", &a.sequenced_antigen_line_show,
                "sequenced_antigen_line_width", &a.sequenced_antigen_line_width,
                "sequenced_antigen_line_length", &a.sequenced_antigen_line_length,
                "sequenced_antigen_line_color", json::field(&a.sequenced_antigen_line_color, &Color::to_string, &Color::from_string),
                "section_label_color", json::field(&a.section_label_color, &Color::to_string, &Color::from_string),
                "section_label_offset_x", &a.section_label_offset_x,
                "section_label_offset_y", &a.section_label_offset_y,
                "section_label_size", &a.section_label_size,
                "hz_line_sections", static_cast<std::vector<HzLineSection>*>(&a),
                "hz_line_width", &a.hz_line_width,
                "hz_line_color", json::field(&a.hz_line_color, &Color::to_string, &Color::from_string),
                "vertical_gap", &a.vertical_gap
                                   );
        }

}; // class HzLineSections

// ----------------------------------------------------------------------

class SettingsDrawTree
{
 public:
    inline SettingsDrawTree()
        : root_edge(0), line_color(0), line_width(1), force_line_width(false), label_style("Helvetica Neue"), name_offset(0.2),
          grid_step(0), grid_color(0x80000000), grid_width(0.3),
          hide_if_cumulative_edge_length_bigger_than(std::numeric_limits<double>::max())
           {}

    double root_edge;
    Color line_color;
    double line_width;
    bool force_line_width;      // use line_width settings, ignore vertical_step adjustment
    TextStyle label_style;
    double name_offset;
    SettingsAATransition aa_transition;
    SettingsMarkNodesOnTree mark_nodes;
    size_t grid_step;           // 0 - no grid, N - use N*vertical_step as grid cell height
    Color grid_color;
    double grid_width;
    HzLineSections hz_line_sections;
    Date hide_isolated_before;
    double hide_if_cumulative_edge_length_bigger_than;

    friend inline auto json_fields(SettingsDrawTree& a)
        {
            return std::make_tuple(
                "hide_if_cumulative_edge_length_bigger_than", &a.hide_if_cumulative_edge_length_bigger_than,
                "hide_isolated_before", json::field(&a.hide_isolated_before, &Date::display, &Date::parse, json::output_if_true),
                "root_edge", &a.root_edge, // object_double_non_negative_value
                "line_color", json::field(&a.line_color, &Color::to_string, &Color::from_string),
                "force_line_width", &a.force_line_width,
                "line_width", &a.line_width, // object_double_non_negative_value
                "name_offset", &a.name_offset,
                "label_style", &a.label_style,
                "aa_transition", &a.aa_transition,
                "?grid_step", json::comment("grid_step: 0 - off, N - use N*vertical_step as grid cell height"),
                "grid_step", &a.grid_step,
                "grid_color", json::field(&a.grid_color, &Color::to_string, &Color::from_string),
                "grid_width", &a.grid_width, // object_double_non_negative_value
                "hz_line_sections", json::field(&a.hz_line_sections),
                  // "vaccines", &a.mark_nodes,
                "mark_nodes", &a.mark_nodes
                                   );
        }

}; // class SettingsDrawTree

// ----------------------------------------------------------------------

class SettingsLegend
{
 public:
    inline SettingsLegend()
        : font_size(14), interline(1.2), style("monospace"), geographic_map(true),
          geographic_map_fraction(0.1), geographic_map_outline_color(GREY), geographic_map_outline_width(1.0),
          offset_x(0), offset_y(0) {}

    double font_size;
    double interline;
    TextStyle style;
    bool geographic_map;        // show geographic_map as a coloring by continents legend
    double geographic_map_fraction; // height of geographic map relative to the canvas height
    Color geographic_map_outline_color;
    double geographic_map_outline_width;
    double offset_x, offset_y;

    friend inline auto json_fields(SettingsLegend& a)
        {
            return std::make_tuple(
                "geographic_map_outline_color", json::field(&a.geographic_map_outline_color, &Color::to_string, &Color::from_string),
                "geographic_map_outline_width", &a.geographic_map_outline_width,
                "geographic_map_fraction", &a.geographic_map_fraction,
                "geographic_map", &a.geographic_map,
                "font_size", &a.font_size, // object_double_non_negative_value
                "interline", &a.interline, // object_double_non_negative_value
                "style", &a.style,
                "offset_x", &a.offset_x,
                "offset_y", &a.offset_y
                                   );
        }

}; // class SettingsLegend

// ----------------------------------------------------------------------

class SettingsSignaturePage
{
 public:
    enum Layout { TreeTimeseriesCladesMaps, TreeCladesTimeseriesMaps };

    inline SettingsSignaturePage()
        : pdf_height(800), pdf_aspect_ratio(1.6), layout(TreeCladesTimeseriesMaps),
          padding_left(0.01), padding_right(0.01), padding_top(0.03), padding_bottom(0.03),
          tree_time_series_space(0), time_series_clades_space(0.01), clades_antigenic_maps_space(0.02) {}

    double pdf_height, pdf_aspect_ratio;
    Layout layout;
      // Obsolete double outer_padding;             // relative to the canvas width
    double padding_left, padding_right, padding_top, padding_bottom;             // relative to the canvas width
    double tree_time_series_space;    // relative to the canvas width
    double time_series_clades_space;  // relative to the canvas width
    double clades_antigenic_maps_space; // relative to the canvas width

 private:
    inline static std::string layout_to_string(const Layout* a)
        {
            switch (*a) {
              case TreeTimeseriesCladesMaps: return "tree-timeseries-clades-maps";
              case TreeCladesTimeseriesMaps: return "tree-clades-timeseries-maps";
            }
            return "colored_grid";            // to shut compiler up
        }

    inline static void layout_from_string(Layout* target, std::string source)
        {
            if (source == "tree-timeseries-clades-maps" || source == "tree_timeseries_clades_maps") *target = TreeTimeseriesCladesMaps;
            else if (source == "tree-clades-timeseries-maps" || source == "tree_clades_timeseries_maps") *target = TreeCladesTimeseriesMaps;
            else throw std::invalid_argument("cannot parse signature page layout from \"" + source + "\", supported values: \"tree-timeseries-clades-maps\", \"tree-clades-timeseries-maps\"");
        }

    inline static double outer_padding_writer(SettingsSignaturePage*)
        {
            throw json::no_value();
        }

    inline static void outer_padding_reader(SettingsSignaturePage* target, double source)
        {
            target->padding_left = target->padding_right = target->padding_top = target->padding_bottom = source;
        }

    friend inline auto json_fields(SettingsSignaturePage& a)
        {
            return std::make_tuple(
                "clades_antigenic_maps_space", &a.clades_antigenic_maps_space, // object_double_non_negative_value
                "time_series_clades_space", &a.time_series_clades_space, // object_double_non_negative_value
                "tree_time_series_space", &a.tree_time_series_space, // object_double_non_negative_value
                "outer_padding", json::field(&a, &outer_padding_writer, &outer_padding_reader), // obsolete, input only
                "padding_left", &a.padding_left,
                "padding_right", &a.padding_right,
                "padding_top", &a.padding_top,
                "padding_bottom", &a.padding_bottom,
                "pdf_aspect_ratio", &a.pdf_aspect_ratio,
                "pdf_height", &a.pdf_height,
                "layout", json::field(&a.layout, &layout_to_string, &layout_from_string),
                "?", json::comment("outer_padding: size of space around the image, fraction of canvas width, default: 0.01; tree_time_series_space: fraction of canvas width")
                                   );
        }

}; // class SettingsSignaturePage

// ----------------------------------------------------------------------

class SettingsTimeSeries
{
 public:
    inline SettingsTimeSeries()
        : dash_width(0.5), dash_line_width(1), max_number_of_months(20), month_label_scale(0.9), month_separator_color(0),
          month_separator_width(0.1), month_width(10), month_year_to_timeseries_gap(0.0035) {}

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
    double month_year_to_timeseries_gap;

    inline void set_begin(std::string aBegin) { begin.parse(aBegin); }
    inline void set_end(std::string aEnd) { end.parse(aEnd); }

    friend inline auto json_fields(SettingsTimeSeries& a)
        {
            return std::make_tuple(
                "month_year_to_timeseries_gap", &a.month_year_to_timeseries_gap,
                "begin", json::field(&a.begin, &Date::display, &Date::parse, json::output_if_true),
                "end", json::field(&a.end, &Date::display, &Date::parse, json::output_if_true),
                "dash_width", &a.dash_width, // object_double_non_negative_value
                "dash_line_width", &a.dash_line_width, // object_double_non_negative_value
                "max_number_of_months", &a.max_number_of_months, // validator_size_t_positive);
                "month_label_scale", &a.month_label_scale, // object_double_non_negative_value
                "month_separator_color", json::field(&a.month_separator_color, &Color::to_string, &Color::from_string, json::output_if_true),
                "month_separator_width", &a.month_separator_width, // object_double_non_negative_value
                "month_width", &a.month_width, // object_double_non_negative_value
                "month_label_style", &a.month_label_style
                                   );
        }

}; // class SettingsTimeSeries

// ----------------------------------------------------------------------

class SettingsClade
{
 public:
    enum class LabelPosition { Middle, Top, Bottom };

    inline SettingsClade()
        : show(false), slot(-1), label_position(LabelPosition::Middle),
          label_position_offset(0.0), label_rotation(0.0), label_offset(3.0) {}
    inline SettingsClade(std::string aBegin, std::string aEnd, size_t aBeginLine, size_t aEndLine, std::string aLabel, std::string aId)
        : show(true), begin(aBegin), end(aEnd), begin_line(aBeginLine), end_line(aEndLine),
          label(aLabel), id(aId), slot(-1), label_position(LabelPosition::Middle),
          label_position_offset(0.0), label_rotation(0.0), label_offset(3.0) {}

    void update(const SettingsClade& source);

    bool show;
    std::string begin;          // name of the node
    std::string end;            // name of the node
    size_t begin_line, end_line; // for drawing only, not saved/loaded
    std::string label;
    std::string id;
    int slot;
    LabelPosition label_position;
    double label_position_offset;
    double label_rotation;
    double label_offset;

 private:
    inline static std::string LabelPosition_to_string(const LabelPosition* a)
        {
            switch (*a) {
              case LabelPosition::Middle: return "middle";
              case LabelPosition::Top: return "top";
              case LabelPosition::Bottom: return "bottom";
            }
            return "middle";            // to shut compiler up
        }

    inline static void LabelPosition_from_string(LabelPosition* target, std::string source)
        {
            if (source == "middle") *target = LabelPosition::Middle;
            else if (source == "top") *target = LabelPosition::Top;
            else if (source == "bottom") *target = LabelPosition::Bottom;
            else throw std::invalid_argument("cannot parse SettingsClade::LabelPosition from " + source);
        }

    friend inline auto json_fields(SettingsClade& a)
        {
            return std::make_tuple(
                "show", &a.show,
                "begin", &a.begin,
                "end", &a.end,
                "id", &a.id,
                "slot", &a.slot,
                "label_position_offset", &a.label_position_offset,
                "label_position", json::field(&a.label_position, &SettingsClade::LabelPosition_to_string, &SettingsClade::LabelPosition_from_string),
                "label_rotation", &a.label_rotation,
                "label_offset", &a.label_offset,
                "label", &a.label
                                   );
        }

}; // class SettingsClade

// ----------------------------------------------------------------------

class SettingsClades
{
 public:
    inline SettingsClades()
        : slot_width(5), arrow_color(BLACK), arrow_extra(0.5), arrow_width(3), line_width(1),
          label_color(0), label_size(10), separator_color(GREY), separator_width(1) {}

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

    friend inline auto json_fields(SettingsClades& a)
        {
            return std::make_tuple(
                "slot_width", &a.slot_width, // object_double_non_negative_value
                "arrow_color", json::field(&a.arrow_color, &Color::to_string, &Color::from_string),
                "arrow_extra", &a.arrow_extra, // object_double_non_negative_value
                "arrow_width", &a.arrow_width, // object_double_non_negative_value
                "line_width", &a.line_width, // object_double_non_negative_value
                "label_color", json::field(&a.label_color, &Color::to_string, &Color::from_string),
                "label_size", &a.label_size, // object_double_non_negative_value
                "separator_color", json::field(&a.separator_color, &Color::to_string, &Color::from_string),
                "separator_width", &a.separator_width, // object_double_non_negative_value
                "label_style", &a.label_style,
                "per_clade", &a.per_clade,
                "?", json::comment("arrow_extra: fraction of vertical_step to extend arrow up and down")
                                   );
        }

}; // class SettingsClades

// ----------------------------------------------------------------------

class SettingsMarkAntigen
{
 public:
    inline SettingsMarkAntigen() : scale(10), outline_color(TRANSPARENT), fill_color(ORANGE), outline_width(0.5), aspect(1), rotation(0), show_on_map(-1),
                                   label_scale(1), label_offset_x(0), label_offset_y(0), label_line_width(0.1), label_color(BLACK), label_line_color(BLACK), label_font("default") {}

    std::string id, tree_id;    // if tree_id is empty, id is used to match node on the tree (if marked_antigens_on_all_maps is false)
    double scale;
    Color outline_color, fill_color;
    double outline_width, aspect, rotation;
    int show_on_map;            // force showing antigen on the map corresponding to this section number, -1 - ignore this
    std::string label;
    double label_scale, label_offset_x, label_offset_y, label_line_width;
    Color label_color, label_line_color;
    std::string label_font;

    friend inline auto json_fields(SettingsMarkAntigen& a)
        {
            return std::make_tuple(
                "id", &a.id,
                "tree_id", &a.tree_id,
                "scale", &a.scale,
                "outline_color", json::field(&a.outline_color, &Color::to_string, &Color::from_string),
                "fill_color", json::field(&a.fill_color, &Color::to_string, &Color::from_string),
                "outline_width", &a.outline_width,
                "aspect", &a.aspect,
                "rotation", &a.rotation,
                "show_on_map", &a.show_on_map,
                "label_scale", &a.label_scale,
                "label_offset_x", &a.label_offset_x,
                "label_offset_y", &a.label_offset_y,
                "label_line_width", &a.label_line_width,
                "label_color", json::field(&a.label_color, &Color::to_string, &Color::from_string),
                "label_line_color", json::field(&a.label_line_color, &Color::to_string, &Color::from_string),
                "label_font", &a.label_font,
                "label", &a.label
                                   );
        }

}; // class SettingsMarkAntigen

// ----------------------------------------------------------------------

class SettingsMarkAntigens : public std::vector<SettingsMarkAntigen>
{
 public:
    inline SettingsMarkAntigens() {}

}; // class SettingsMarkNodesOnTree

// ----------------------------------------------------------------------

class SettingsAntigenicMaps
{
 public:
    enum Mode { ColoredGrid, BWVpos, NamedGrid };

    inline SettingsAntigenicMaps()
        : mode(NamedGrid),
          border_width(1), grid_line_width(0.5), border_color(BLACK), grid_color(GREY), background_color(0xFFFFF8),
          map_height_fraction_of_page(0.15), gap_between_maps(0.005),
          map_zoom(1.1), map_x_offset(0), map_y_offset(0),
          serum_scale(5), reference_antigen_scale(5), test_antigen_scale(3), vaccine_antigen_scale(8), tracked_antigen_scale(5),
          serum_outline_width(0.5), reference_antigen_outline_width(0.5), test_antigen_outline_width(0.5), vaccine_antigen_outline_width(0.5),
          sequenced_antigen_outline_width(0.5), tracked_antigen_outline_width(0.5),
          serum_outline_color(LIGHT_GREY), reference_antigen_outline_color(LIGHT_GREY), test_antigen_outline_color(LIGHT_GREY),
          test_antigen_fill_color(LIGHT_GREY), vaccine_antigen_outline_color(WHITE), sequenced_antigen_outline_color(WHITE), sequenced_antigen_fill_color(0xA0A0A0),
          tracked_antigen_outline_color(WHITE), tracked_antigen_colored_by_clade(false),
          egg_antigen_aspect(0.75), reassortant_rotation(0.5 /* M_PI / 6.0 */), maps_for_sections_without_antigens(false), marked_antigens_on_all_maps(false),
          max_number_columns(100), grid_width(0), bracket_border_color(BLACK), bracket_background_color(TRANSPARENT), bracket_border_width(1),
          map_label_color(BLACK), map_label_offset_x(2), map_label_offset_y(10), map_label_size(10),
          show_tracked_homologous_sera(false), serum_circle_color(BLACK), serum_circle_thickness(1)
        {}

    Mode mode;
    double border_width, grid_line_width;
    Color border_color, grid_color, background_color;
    double map_height_fraction_of_page;
    double gap_between_maps;    // relative to canvas width
    double map_zoom, map_x_offset, map_y_offset; // zoom>1 means zoom out, offsets are in the antigenic units
    double serum_scale, reference_antigen_scale, test_antigen_scale, vaccine_antigen_scale, tracked_antigen_scale;
    double serum_outline_width, reference_antigen_outline_width, test_antigen_outline_width, vaccine_antigen_outline_width, sequenced_antigen_outline_width, tracked_antigen_outline_width;
    Color serum_outline_color, reference_antigen_outline_color, test_antigen_outline_color, test_antigen_fill_color, vaccine_antigen_outline_color, sequenced_antigen_outline_color, sequenced_antigen_fill_color, tracked_antigen_outline_color;
    bool tracked_antigen_colored_by_clade;
    double egg_antigen_aspect, reassortant_rotation;
    bool maps_for_sections_without_antigens; // draw maps for sections having no tracked antigens
    Transformation map_transformation;
    SettingsMarkAntigens mark_antigens;
    bool marked_antigens_on_all_maps; // if false, antigen is marked if it is tracked on this map
      // BWVpos
    size_t max_number_columns;  // do not make more columns that this value in the bw_vpos layout
      // NamedGrid
    size_t grid_width;          // 0 - choose autmatically
    Color bracket_border_color, bracket_background_color;
    double bracket_border_width;
    Color map_label_color;
    double map_label_offset_x, map_label_offset_y, map_label_size;
    bool show_tracked_homologous_sera;
    Color serum_circle_color;
    double serum_circle_thickness;
    std::string tracked_antigen_isolated_after;

 private:
    inline static std::string mode_to_string(const Mode* a)
        {
            switch (*a) {
              case ColoredGrid: return "colored_grid";
              case BWVpos: return "bw_vpos";
              case NamedGrid: return "named_grid";
            }
            return "named_grid";            // to shut compiler up
        }

    inline static void mode_from_string(Mode* target, std::string source)
        {
            if (source == "colored_grid" || source == "colored-grid") *target = ColoredGrid;
            else if (source == "bw_vpos" || source == "bw-vpos" || source == "bwvpos") *target = BWVpos;
            else if (source == "named_grid" || source == "named-grid") *target = NamedGrid;
            else throw std::invalid_argument("cannot parse hz line section mode from \"" + source + "\", supported values: \"colored_grid\", \"bw_vpos\", \"named_grid\"");
        }

    friend inline auto json_fields(SettingsAntigenicMaps& a)
        {
            return std::make_tuple(
                "map_height_fraction_of_page", &a.map_height_fraction_of_page,
                "gap_between_maps", &a.gap_between_maps,
                "max_number_columns", &a.max_number_columns,
                "maps_for_sections_without_antigens", &a.maps_for_sections_without_antigens,
                "show_tracked_homologous_sera", &a.show_tracked_homologous_sera,
                "serum_circle_color", json::field(&a.serum_circle_color, &Color::to_string, &Color::from_string),
                "serum_circle_thickness", &a.serum_circle_thickness,
                "border_width", &a.border_width, // object_double_non_negative_value
                "border_color", json::field(&a.border_color, &Color::to_string, &Color::from_string),
                "grid_line_width", &a.grid_line_width, // object_double_non_negative_value
                "grid_color", json::field(&a.grid_color, &Color::to_string, &Color::from_string),
                "grid_width", &a.grid_width,
                "background_color", json::field(&a.background_color, &Color::to_string, &Color::from_string),
                "map_x_offset", &a.map_x_offset,
                "map_y_offset", &a.map_y_offset,
                "map_zoom", &a.map_zoom,
                "map_transformation", &a.map_transformation,
                "egg_antigen_aspect", &a.egg_antigen_aspect,
                "reassortant_rotation", &a.reassortant_rotation,
                "serum_scale", &a.serum_scale,
                "reference_antigen_scale", &a.reference_antigen_scale,
                "test_antigen_scale", &a.test_antigen_scale,
                "vaccine_antigen_scale", &a.vaccine_antigen_scale,
                "tracked_antigen_scale", &a.tracked_antigen_scale,
                "serum_outline_width", &a.serum_outline_width,
                "reference_antigen_outline_width", &a.reference_antigen_outline_width,
                "test_antigen_outline_width", &a.test_antigen_outline_width,
                "vaccine_antigen_outline_width", &a.vaccine_antigen_outline_width,
                "tracked_antigen_outline_width", &a.tracked_antigen_outline_width,
                "tracked_antigen_isolated_after", &a.tracked_antigen_isolated_after,
                "serum_outline_color", json::field(&a.serum_outline_color, &Color::to_string, &Color::from_string),
                "reference_antigen_outline_color", json::field(&a.reference_antigen_outline_color, &Color::to_string, &Color::from_string),
                "test_antigen_outline_color", json::field(&a.test_antigen_outline_color, &Color::to_string, &Color::from_string),
                "test_antigen_fill_color", json::field(&a.test_antigen_fill_color, &Color::to_string, &Color::from_string),
                "vaccine_antigen_outline_color", json::field(&a.vaccine_antigen_outline_color, &Color::to_string, &Color::from_string),
                "tracked_antigen_outline_color", json::field(&a.tracked_antigen_outline_color, &Color::to_string, &Color::from_string),
                "tracked_antigen_colored_by_clade", &a.tracked_antigen_colored_by_clade,
                "sequenced_antigen_outline_width", &a.sequenced_antigen_outline_width,
                "sequenced_antigen_outline_color", json::field(&a.sequenced_antigen_outline_color, &Color::to_string, &Color::from_string),
                "sequenced_antigen_fill_color", json::field(&a.sequenced_antigen_fill_color, &Color::to_string, &Color::from_string),
                "marked_antigens_on_all_maps", &a.marked_antigens_on_all_maps,
                "mark_antigens", &a.mark_antigens,
                "bracket_background_color", json::field(&a.bracket_background_color, &Color::to_string, &Color::from_string),
                "bracket_border_color", json::field(&a.bracket_border_color, &Color::to_string, &Color::from_string),
                "bracket_border_width", &a.bracket_border_width,
                "map_label_color", json::field(&a.map_label_color, &Color::to_string, &Color::from_string),
                "map_label_offset_x", &a.map_label_offset_x,
                "map_label_offset_y", &a.map_label_offset_y,
                "map_label_size", &a.map_label_size,
                "mode", json::field(&a.mode, &SettingsAntigenicMaps::mode_to_string, &SettingsAntigenicMaps::mode_from_string)
                                   );
        }

}; // class SettingsAntigenicMaps

// ----------------------------------------------------------------------

class SettingsTitle
{
 public:
    inline SettingsTitle()
        : offset_x(10), offset_y(20), size(20), color(BLACK), rotation(0)
        {}

    double offset_x, offset_y, size;
    Color color;
    TextStyle style;
    std::string text;
    double rotation;

    friend inline auto json_fields(SettingsTitle& a)
        {
            return std::make_tuple(
                "offset_x", &a.offset_x,
                "offset_y", &a.offset_y,
                "rotation", &a.rotation,
                "size", &a.size,
                "color", json::field(&a.color, &Color::to_string, &Color::from_string),
                "style", &a.style,
                "text", &a.text
                                   );
        }

}; // class SettingsTitle

// ----------------------------------------------------------------------

class Settings
{
 public:
    inline Settings() {}

    SettingsTitle title;
    SettingsSignaturePage signature_page;
    SettingsDrawTree draw_tree;
    SettingsLegend legend;
    SettingsTimeSeries time_series;
    SettingsClades clades;
    SettingsAntigenicMaps antigenic_maps;

    friend inline auto json_fields(Settings& a)
        {
            return std::make_tuple(
                "signature_page", &a.signature_page,
                "antigenic_maps", &a.antigenic_maps,
                "legend", &a.legend,
                "time_series", &a.time_series,
                "clades", &a.clades,
                "title", &a.title,
                "tree", &a.draw_tree
                );
        }

}; // class Settings

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
