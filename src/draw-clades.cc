#include <map>

#include "draw-clades.hh"
#include "tree.hh"
#include "draw-tree.hh"

// ----------------------------------------------------------------------

Clades& Clades::prepare(const Tree& aTree, const SettingsClades& aSettings)
{
      // extract clades from aTree
    std::map<std::string, std::pair<size_t, size_t>> clades; // clade name -> (first line, last-line)
    auto scan = [&clades](const Node& aNode) {
        for (auto& c: aNode.clades) {
            auto p = clades.insert(std::make_pair(c, std::make_pair(aNode.line_no, aNode.line_no)));
            if (!p.second && p.first->second.second < aNode.line_no) {
                p.first->second.second = aNode.line_no;
            }
        }
    };
    iterate_leaf(aTree, scan);

    for (auto& c: clades) {
          // std::cerr << c.first << ' ' << c.second.first << ' ' << c.second.second << std::endl;
        add_clade(static_cast<int>(c.second.first), static_cast<int>(c.second.second), c.first, c.first, aSettings);
    }
    assign_slots(aSettings);

    return *this;

} // Clades::prepare

// ----------------------------------------------------------------------

void Clades::add_clade(int aBegin, int aEnd, std::string aLabel, std::string aId, const SettingsClades& aSettings)
{
    SettingsClade clade(aBegin, aEnd, aLabel, aId);
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
    std::sort(mClades.begin(), mClades.end(), [](const auto& a, const auto& b) -> bool { return a.begin == b.begin ? a.end > b.end : a.begin < b.begin; });
    int slot = 0;
    for (auto& clade: mClades) {
        if (clade.slot < 0 && clade.show)
            clade.slot = slot;
        ++slot;
    }

} // Clades::assign_slots

// ----------------------------------------------------------------------

void Clades::draw(Surface& aSurface, const Viewport& aViewport, const Viewport& aTimeSeriesViewport, const DrawTree& aDrawTree, const SettingsClades& aSettings) const
{
    for (const auto& clade: mClades) {
        if (clade.show) {
            const auto x = aViewport.origin.x + clade.slot * aSettings.slot_width;
            const auto base_y = aViewport.origin.y;
            const auto vertical_step = aDrawTree.vertical_step();
            const auto top = base_y + vertical_step * clade.begin -  aSettings.arrow_extra * vertical_step;
            const auto bottom = base_y + vertical_step * clade.end + aSettings.arrow_extra * vertical_step;

            double label_vpos;
            switch (clade.label_position) {
              case CladeLabelPosition::Top:
                  label_vpos = top;
                  break;
              case CladeLabelPosition::Bottom:
                  label_vpos = bottom;
                  break;
              case CladeLabelPosition::Middle:
                  label_vpos = (top + bottom) / 2.0;
                  break;
            }
            auto const label_size = aSurface.text_size(clade.label, aSettings.label_size, aSettings.label_style);
            label_vpos += label_size.height / 2.0 + clade.label_position_offset;

            aSurface.double_arrow({x, top}, {x, bottom}, aSettings.arrow_color, aSettings.line_width, aSettings.arrow_width);
            aSurface.text({x + clade.label_offset, label_vpos}, clade.label, aSettings.label_color, aSettings.label_size, aSettings.label_style, clade.label_rotation);
            if (clade.begin > 0)
                aSurface.line({x, top}, {aTimeSeriesViewport.origin.x, top}, aSettings.separator_color, aSettings.separator_width);
            if (clade.end < static_cast<int>(aDrawTree.number_of_lines() - 1))
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
