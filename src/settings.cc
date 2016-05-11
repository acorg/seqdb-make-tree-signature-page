#include "settings.hh"
#include "draw-clades.hh"

// ----------------------------------------------------------------------

jsonw::IfPrependComma SettingsDrawTree::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = jsonw::json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json(target, comma, "root_edge", root_edge, indent, prefix);
    comma = jsonw::json(target, comma, "line_color", line_color, indent, prefix);
    comma = jsonw::json(target, comma, "line_width", line_width, indent, prefix);
    comma = jsonw::json(target, comma, "label_style", label_style, indent, prefix);
    comma = jsonw::json(target, comma, "name_offset", name_offset, indent, prefix);
    comma = jsonw::json(target, comma, SettingsAATransition::json_name, aa_transition, indent, prefix);
    comma = jsonw::json(target, comma, "grid_step", grid_step, indent, prefix);
    comma = jsonw::json(target, comma, "grid_color", grid_color, indent, prefix);
    comma = jsonw::json(target, comma, "grid_width", grid_width, indent, prefix);
    comma = jsonw::json(target, comma, "?", "grid_step: 0 - off, N - use N*vertical_step as grid cell height", indent, prefix);
    return  jsonw::json_end(target, '}', indent, prefix);

} // SettingsDrawTree::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma SettingsLegend::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = jsonw::json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json(target, comma, "font_size", font_size, indent, prefix);
    comma = jsonw::json(target, comma, "interline", interline, indent, prefix);
    comma = jsonw::json(target, comma, "style", style, indent, prefix);
    return  jsonw::json_end(target, '}', indent, prefix);

} // SettingsLegend::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma SettingsSignaturePage::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = jsonw::json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json(target, comma, "outer_padding", outer_padding, indent, prefix);
    comma = jsonw::json(target, comma, "tree_time_series_space", tree_time_series_space, indent, prefix);
    comma = jsonw::json(target, comma, "time_series_clades_space", time_series_clades_space, indent, prefix);
    comma = jsonw::json(target, comma, "clades_antigenic_maps_space", clades_antigenic_maps_space, indent, prefix);
    comma = jsonw::json(target, comma, "?", "outer_padding: size of space around the image, fraction of canvas width, default: 0.01; tree_time_series_space: fraction of canvas width", indent, prefix);
    return  jsonw::json_end(target, '}', indent, prefix);

} // SettingsSignaturePage::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma SettingsTimeSeries::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = jsonw::json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json(target, comma, "begin", begin, indent, prefix);
    comma = jsonw::json(target, comma, "end", end, indent, prefix);
    comma = jsonw::json(target, comma, "dash_width", dash_width, indent, prefix);
    comma = jsonw::json(target, comma, "dash_line_width", dash_line_width, indent, prefix);
    comma = jsonw::json(target, comma, "max_number_of_months", max_number_of_months, indent, prefix);
    comma = jsonw::json(target, comma, "month_label_scale", month_label_scale, indent, prefix);
    comma = jsonw::json(target, comma, "month_label_style", month_label_style, indent, prefix);
    comma = jsonw::json(target, comma, "month_separator_color", month_separator_color, indent, prefix);
    comma = jsonw::json(target, comma, "month_separator_width", month_separator_width, indent, prefix);
    comma = jsonw::json(target, comma, "month_width", month_width, indent, prefix);
    return  jsonw::json_end(target, '}', indent, prefix);

} // SettingsTimeSeries::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma SettingsClade::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = jsonw::json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json(target, comma, "id", id, indent, prefix);
    comma = jsonw::json(target, comma, "label", label, indent, prefix);
    comma = jsonw::json(target, comma, "show", show, indent, prefix);
    comma = jsonw::json(target, comma, "begin", begin, indent, prefix);
    comma = jsonw::json(target, comma, "end", end, indent, prefix);
    comma = jsonw::json(target, comma, "slot", slot, indent, prefix);
    comma = jsonw::json(target, comma, "label_position", to_string(label_position), indent, prefix);
    comma = jsonw::json(target, comma, "label_position_offset", label_position_offset, indent, prefix);
    comma = jsonw::json(target, comma, "label_rotation", label_rotation, indent, prefix);
    comma = jsonw::json(target, comma, "label_offset", label_offset, indent, prefix);
    return  jsonw::json_end(target, '}', indent, prefix);

} // SettingsClade::json

// ----------------------------------------------------------------------

void SettingsClade::update(const SettingsClade& source)
{
    show = source.show;
    if (source.begin >= 0)
        begin = source.begin;
    if (source.end >= 0)
        end = source.end;
    label = source.label;
    if (source.slot >= 0)
        slot = source.slot;
    label_position = source.label_position;
    label_position_offset = source.label_position_offset;
    label_rotation = source.label_rotation;
    label_offset = source.label_offset;

} // SettingsClade::update

// ----------------------------------------------------------------------

void SettingsClades::extract(const Clades& aClades)
{
    for (const auto& clade: aClades.clades()) {
        const auto found = std::find_if(per_clade.begin(), per_clade.end(), [&](const auto& e) -> bool { return clade.id == e.id; });
        if (found == per_clade.end())
            per_clade.push_back(clade);
        else
            found->update(clade);
    }

} // SettingsClades::extract

// ----------------------------------------------------------------------

jsonw::IfPrependComma SettingsClades::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = jsonw::json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json(target, comma, "slot_width", slot_width, indent, prefix);
    comma = jsonw::json(target, comma, "arrow_color", arrow_color, indent, prefix);
    comma = jsonw::json(target, comma, "arrow_extra", arrow_extra, indent, prefix);
    comma = jsonw::json(target, comma, "arrow_width", arrow_width, indent, prefix);
    comma = jsonw::json(target, comma, "line_width", line_width, indent, prefix);
    comma = jsonw::json(target, comma, "label_color", label_color, indent, prefix);
    comma = jsonw::json(target, comma, "label_size", label_size, indent, prefix);
    comma = jsonw::json(target, comma, "label_style", label_style, indent, prefix);
    comma = jsonw::json(target, comma, "separator_color", separator_color, indent, prefix);
    comma = jsonw::json(target, comma, "separator_width", separator_width, indent, prefix);
    comma = jsonw::json(target, comma, "per_clade", per_clade, indent, prefix);
    comma = jsonw::json(target, comma, "?", "arrow_extra: fraction of vertical_step to extend arrow up and down", indent, prefix);
    return  jsonw::json_end(target, '}', indent, prefix);

} // SettingsClades::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma SettingsAntigenicMaps::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = jsonw::json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json(target, comma, "border_width", border_width, indent, prefix);
    comma = jsonw::json(target, comma, "border_color", border_color, indent, prefix);
      // comma = jsonw::json(target, comma, "?", "", indent, prefix);
    return  jsonw::json_end(target, '}', indent, prefix);

} // SettingsAntigenicMaps::json

// ----------------------------------------------------------------------

jsonw::IfPrependComma Settings::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = jsonw::json_begin(target, comma, '{', indent, prefix);
    comma = jsonw::json(target, comma, "signature_page", signature_page, indent, prefix);
    comma = jsonw::json(target, comma, "tree", draw_tree, indent, prefix);
    comma = jsonw::json(target, comma, "legend", legend, indent, prefix);
    comma = jsonw::json(target, comma, "time_series", time_series, indent, prefix);
    comma = jsonw::json(target, comma, "clades", clades, indent, prefix);
    comma = jsonw::json(target, comma, "antigenic_maps", antigenic_maps, indent, prefix);
    return  jsonw::json_end(target, '}', indent, prefix);

} // Settings::json

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
