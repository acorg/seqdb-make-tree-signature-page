#include <map>

#include "draw-clades.hh"
#include "tree.hh"
#include "draw-tree.hh"

// ----------------------------------------------------------------------

Clades& Clades::prepare(const Tree& aTree, const SettingsClades& aSettings)
{
      // extract clades from aTree
    std::map<std::string, std::pair<std::pair<std::string, size_t>, std::pair<std::string, size_t>>> clades; // clade name -> ((first node name, line_no), (last node name, line_no))
    auto scan = [&clades](const Node& aNode) {
        for (auto& c: aNode.clades) {
            const auto element = std::make_pair(aNode.name, aNode.line_no);
            auto p = clades.insert(std::make_pair(c, std::make_pair(element, element)));
            if (!p.second) { // the clade is already present, extend its range
                p.first->second.second = element;
            }
        }
    };
    iterate_leaf(aTree, scan);

    for (auto& c: clades) {
          // std::cerr << c.first << ' ' << c.second.first << ' ' << c.second.second << std::endl;
        add_clade(c.second.first, c.second.second, c.first, c.first, aSettings);
    }
    assign_slots(aSettings);

    return *this;

} // Clades::prepare

// ----------------------------------------------------------------------

void Clades::add_clade(const std::pair<std::string, size_t>& aBegin, const std::pair<std::string, size_t>& aEnd, std::string aLabel, std::string aId, const SettingsClades& aSettings)
{
    SettingsClade clade(aBegin.first, aEnd.first, aBegin.second, aEnd.second, aLabel, aId);
    const auto found = std::find_if(aSettings.per_clade.begin(), aSettings.per_clade.end(), [&](const auto& e) -> bool { return aId == e.id; });
    if (found != aSettings.per_clade.end())
        clade.update(*found);
    else
        hide_old_clades(clade);
    mClades.push_back(clade);

} // Clades::add_clade

// ----------------------------------------------------------------------

void Clades::hide_old_clades(SettingsClade& aClade)
{
      // Hide currently  unused H3 clades by default
    if (aClade.id == "gly" || aClade.id == "no-gly" || aClade.id == "3C3b")
        aClade.show = false;

} // Clades::hide_old_clades

// ----------------------------------------------------------------------

void Clades::assign_slots(const SettingsClades& /*aSettings*/)
{
    std::sort(mClades.begin(), mClades.end(), [](const auto& a, const auto& b) -> bool { return a.begin_line == b.begin_line ? a.end_line > b.end_line : a.begin_line < b.begin_line; });
    decltype(mClades.front().slot) slot = 0;
    size_t last_end = 0;
    for (auto& clade: mClades) {
        if (clade.slot < 0 && clade.show) {
            if (last_end > clade.begin_line)
                ++slot;         // increase slot in case of overlapping
            clade.slot = slot;
            last_end = clade.end_line;
        }
    }

} // Clades::assign_slots

// ----------------------------------------------------------------------

void Clades::draw(Surface& aSurface, const Tree& aTree, const Viewport& aViewport, const Viewport& aTimeSeriesViewport, const DrawTree& aDrawTree, const SettingsClades& aSettings) const
{
    for (const auto& clade: mClades) {
        if (clade.show) {
            const auto clade_begin = aTree.find_node_by_name(clade.begin);
            if (clade_begin == nullptr)
                throw std::runtime_error("cannot find clade first node by name: " + clade.begin);
            const auto clade_end = aTree.find_node_by_name(clade.end);
            if (clade_end == nullptr)
                throw std::runtime_error("cannot find clade last node by name: " + clade.end);
            const auto x = aViewport.origin.x + clade.slot * aSettings.slot_width;
            const auto base_y = aViewport.origin.y;
            const auto vertical_step = aDrawTree.vertical_step();
            const auto top = base_y + vertical_step * clade_begin->line_no -  aSettings.arrow_extra * vertical_step;
            const auto bottom = base_y + vertical_step * clade_end->line_no + aSettings.arrow_extra * vertical_step;

            double label_vpos = 0; // initialized to avoid gcc-6 complaining about using uninitialized in the line with += below
            switch (clade.label_position) {
              case SettingsClade::LabelPosition::Top:
                  label_vpos = top;
                  break;
              case SettingsClade::LabelPosition::Bottom:
                  label_vpos = bottom;
                  break;
              case SettingsClade::LabelPosition::Middle:
                  label_vpos = (top + bottom) / 2.0;
                  break;
            }
            auto const label_size = aSurface.text_size(clade.label, aSettings.label_size, aSettings.label_style);
            label_vpos += label_size.height / 2.0 + clade.label_position_offset;

            aSurface.double_arrow({x, top}, {x, bottom}, aSettings.arrow_color, aSettings.line_width, aSettings.arrow_width);
            aSurface.text({x + clade.label_offset, label_vpos}, clade.label, aSettings.label_color, aSettings.label_size, aSettings.label_style, clade.label_rotation);
            if (clade_begin->line_no > 0)
                aSurface.line({x, top}, {aTimeSeriesViewport.origin.x, top}, aSettings.separator_color, aSettings.separator_width);
            if (clade_end->line_no < (aDrawTree.number_of_lines() - 1))
                aSurface.line({x, bottom}, {aTimeSeriesViewport.origin.x, bottom}, aSettings.separator_color, aSettings.separator_width);
        }
    }

} // Clades::draw

// ----------------------------------------------------------------------

Size Clades::size(Surface& aSurface, const SettingsClades& aSettings) const
{
    double width = 0;
    for (const auto& clade: mClades) {
        const auto c_width = clade.show ? clade.slot * aSettings.slot_width + clade.label_offset + aSurface.text_size(clade.label, aSettings.label_size, aSettings.label_style).width : 0;
        if (c_width > width)
            width = c_width;
    }
    return Size(width, 1);

} // Clades::size

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
