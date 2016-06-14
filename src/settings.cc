#include "settings.hh"
#include "draw-clades.hh"
#include "tree.hh"

// ----------------------------------------------------------------------

HzLineSection::HzLineSection(const Node& aNode, Color aColor)
    : first_name(aNode.name), first_line(aNode.line_no), color(aColor), line_width(2)
{
} // HzLineSection::HzLineSection

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

void SettingsVaccinesOnTree::add(std::string aId, std::string aLabel)
{
    const auto found = std::find_if(begin(), end(), [&](const auto& e) { return e.id == aId; });
    if (found == end())
        emplace_back(aId, aLabel);

} // SettingsVaccinesOnTree::add

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
