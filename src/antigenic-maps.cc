#include "antigenic-maps.hh"
#include "tree.hh"
#include "draw-tree.hh"
#include "chart.hh"
#include "float.hh"

// ----------------------------------------------------------------------

AntigenicMaps& AntigenicMaps::prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings)
{
    if (!aSections.empty()) {
        for (size_t section_no = 0; section_no < (aSections.size() - 1); ++section_no) {
            mNamesPerMap.push_back(aTree.names_between(aSections[section_no].first_name, aSections[section_no + 1].first_name));
        }
        mNamesPerMap.push_back(aTree.names_between(aSections.back().first_name, "after-the-last-name"));
    }
    else {
        mNamesPerMap.push_back(aTree.names());
    }

    mLinesOfSequencedAntigensInChart = aChart->sequenced_antigens(aTree.leaves());
    mLeftOffset = aSections.empty() ? 0.0 : aSections[0].line_width * 2.0;
    mGap = aSettings.gap_between_maps * aPageArea.size.width;

    iterate_leaf(aTree, [this](const Node& aNode) { for (const auto& hi_name: aNode.hi_names) { mNodeByName.emplace(hi_name, &aNode); } });

    return *this;

} // AntigenicMaps::prepare

// ----------------------------------------------------------------------

size_t AntigenicMaps::number_of_shown_maps(const HzLineSections& aSections) const
{
    size_t shown_maps = 0;
    for (const auto& section: aSections) {
        if (section.show_map)
            ++shown_maps;
    }
    return shown_maps;

} // AntigenicMaps::number_of_shown_maps

// ----------------------------------------------------------------------

void AntigenicMaps::draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings) const
{
    for (size_t section_no = 0; section_no < number_of_maps(); ++section_no) {
        const Viewport map_viewport = viewport_of(aViewport, aSections, section_no);
        if (map_viewport.size.width > 0) {
            std::cout << "Drawing map for section " << DrawHzLines::section_label(aSections, section_no, true) << " "
                      << "line:" << aSections[section_no].first_line << " " << aSections[section_no].first_name << " " << section_color(aSections, section_no) << std::endl;

            Surface::PushContext pc(aSurface);
            aSurface.rectangle_filled(map_viewport, aSettings.border_color, aSettings.border_width, aSettings.background_color);

            const auto chart_viewport = aChart->viewport();

              //const double scale = aSurface.set_clip_region(map_viewport, chart_viewport.size.width);
            const double scale = aSurface.set_clip_region(map_viewport, chart_viewport);
              // std::cerr << "map scale " << scale << std::endl;
            aSurface.grid(chart_viewport, 1, aSettings.grid_color, aSettings.grid_line_width * scale);

            aChart->draw_points_reset(aSettings);
            size_t num_antigens;
            if (aSettings.tracked_antigen_colored_by_clade)
                num_antigens = aChart->tracked_antigens_colored_by_clade(names_per_map()[section_no], mNodeByName, aSettings);
            else
                num_antigens = aChart->tracked_antigens(names_per_map()[section_no], section_color(aSections, section_no), aSettings);
            aChart->marked_antigens(aSettings.mark_antigens, names_per_map()[section_no], section_no, aSettings);
            aChart->draw(aSurface, scale, aSettings);
            std::cout << "    Names: " << names_per_map()[section_no].size() << " antigens: " << num_antigens << std::endl;
              // std::cout << "Section " << aSections[section_no].first_line << " " << aSections[section_no].first_name << " " << section_color(aSections, section_no) << " names: " << names_per_map()[section_no].size() << " antigens: " << num_antigens << std::endl;
        }
        else {
            std::cout << "Section " << aSections[section_no].first_line << " " << aSections[section_no].first_name << " " << section_color(aSections, section_no) << " names: " << names_per_map()[section_no].size() << " no tracked antigens" << std::endl;
        }
    }

} // AntigenicMaps::draw

// ----------------------------------------------------------------------

AntigenicMapsGrid& AntigenicMapsGrid::prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings)
{
    AntigenicMaps::prepare(aTree, aPageArea, aChart, aSections, aSettings);

    std::tie(mGridWidth, mGridHeight) = grid(aSettings, aSections);

    return *this;

} // AntigenicMapsGrid::prepare

// ----------------------------------------------------------------------

void AntigenicMapsGrid::calculate_viewports(Tree& /*aTree*/, Chart* /*aChart*/, const Viewport& aViewport, const Viewport& /*aPageArea*/, const DrawTree& /*aDrawTree*/, const HzLineSections& /*aSections*/, const SettingsAntigenicMaps& /*aSettings*/)
{
    if (float_equal(mCellSize.width, 0.0)) {
        const double size = (aViewport.size.height - gap_between_maps() * (mGridHeight - 1)) / mGridHeight;
        mCellSize.set(size, size);
    }

} // AntigenicMapsGrid::calculate_viewports

// ----------------------------------------------------------------------

Viewport AntigenicMapsGrid::viewport_of(const Viewport& aViewport, const HzLineSections& aSections, size_t map_no) const
{
    Viewport viewport;
    if (aSections[map_no].show_map) {
        size_t index = DrawHzLines::section_index(aSections, map_no, size_t(0));
        const size_t cell_x = index % mGridWidth;
        const size_t cell_y = index / mGridWidth;
        viewport.set(Location(aViewport.origin.x + left_offset() + (mCellSize.width + gap_between_maps()) * cell_x, aViewport.origin.y + (mCellSize.height + gap_between_maps()) * cell_y), mCellSize);
    }
    return viewport;

} // AntigenicMapsGrid::viewport_of

// ----------------------------------------------------------------------

std::pair<size_t, size_t> AntigenicMapsGrid::grid(const SettingsAntigenicMaps& aSettings, const HzLineSections& aSections) const
{
    size_t grid_width = aSettings.grid_width;
    const size_t nm = number_of_shown_maps(aSections);
    if (grid_width == 0) {
        if (nm < 4)
            grid_width = 1;
        else if (nm < 7)
            grid_width = 2;
        else
            grid_width = 3;
    }
    return std::make_pair(grid_width, nm / grid_width + ((nm % grid_width) ? 1 : 0));

} // AntigenicMapsGrid::grid

// ----------------------------------------------------------------------

// std::pair<size_t, size_t> AntigenicMapsGrid::grid(const SettingsAntigenicMaps& /*aSettings*/) const
// {
//     size_t grid_w, grid_h;
//     switch (number_of_maps()) {
//       case 1: grid_w = grid_h = 1; break;
//       case 2: grid_w = 1; grid_h = 2; break;
//       case 3: grid_w = 2; grid_h = 2; break;
//       case 4: grid_w = 2; grid_h = 2; break;
//       case 5: grid_w = 2; grid_h = 3; break;
//       case 6: grid_w = 2; grid_h = 3; break;
//       case 7: grid_w = 3; grid_h = 3; break;
//       case 8: grid_w = 3; grid_h = 3; break;
//       case 9: grid_w = 3; grid_h = 3; break;
//       default: throw std::runtime_error("AntigenicMaps: unsupported number of maps: " + std::to_string(number_of_maps()));
//     }

//     return std::make_pair(grid_w, grid_h);

// } // AntigenicMapsGrid::grid

// ----------------------------------------------------------------------

Color AntigenicMapsNamedGrid::section_color(const HzLineSections& aSections, size_t /*section_no*/) const
{
    return aSections.this_section_antigen_color;

} // AntigenicMapsNamedGrid::section_color

// ----------------------------------------------------------------------

void AntigenicMapsNamedGrid::draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings) const
{
    AntigenicMapsGrid::draw(aSurface, aViewport, aChart, aSections, aSettings);

      // Draw section label
    for (size_t section_no = 0; section_no < number_of_maps(); ++section_no) {
        const Viewport map_viewport = viewport_of(aViewport, aSections, section_no);
        if (map_viewport.size.width > 0) {
            std::string label = DrawHzLines::section_label(aSections, section_no, false);
            Surface::PushContext pc(aSurface);
            aSurface.text(map_viewport.origin + Size(aSettings.map_label_offset_x, aSettings.map_label_offset_y), label, aSettings.map_label_color, aSettings.map_label_size);
        }
    }

} // AntigenicMapsNamedGrid::draw

// ----------------------------------------------------------------------

Color AntigenicMapsColoredGrid::section_color(const HzLineSections& aSections, size_t section_no) const
{
    return aSections[section_no].color;

} // AntigenicMapsColoredGrid::section_color

// ----------------------------------------------------------------------

Size AntigenicMapsVpos::size(const Viewport& /*aPageArea*/, const SettingsAntigenicMaps& /*aSettings*/) const
{
    Location bottom_right;
    for (const Viewport& viewport: mViewports) {
        bottom_right.max(viewport.bottom_right());
    }
    return {bottom_right.x, bottom_right.y};

} // AntigenicMapsVpos::size

// ----------------------------------------------------------------------

AntigenicMapsVpos& AntigenicMapsVpos::prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings)
{
    mCellHeight = aPageArea.size.height * aSettings.map_height_fraction_of_page;
    AntigenicMaps::prepare(aTree, aPageArea, aChart, aSections, aSettings);

    return *this;

} // AntigenicMapsVpos::prepare

// ----------------------------------------------------------------------

void AntigenicMapsVpos::calculate_viewports(Tree& aTree, Chart* aChart, const Viewport& aViewport, const Viewport& aPageArea, const DrawTree& aDrawTree, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings)
{
    double vertical_step = aDrawTree.vertical_step();
    const double top_gap = aViewport.origin.y - aPageArea.origin.y;

    if (vertical_step > 0)
        mViewports.clear();         // re-calculate final viewports
    else
        vertical_step = aViewport.size.height / (aTree.height() + 2); // preliminary step to get preliminary maps layout

    std::vector<double> slot_bottom;

    for (size_t section_no = 0; section_no < aSections.size(); ++section_no) {
        const auto num_antigens = aChart->tracked_antigens(names_per_map()[section_no], WHITE, aSettings);
        if (num_antigens > 0 || aSettings.maps_for_sections_without_antigens) {
            const auto& section = aSections[section_no];
            const auto first_line = section.first_line;
            const auto last_line = section_no < (aSections.size() - 1) ? aSections[section_no + 1].first_line - aSections.vertical_gap - 1 : aTree.height() - 1;
            const double middle = (first_line + last_line) / 2.0 * vertical_step;
            double top = middle - mCellHeight / 2.0;
            if (top < - top_gap)
                top = - top_gap;
            else if ((top + mCellHeight) > (aPageArea.size.height - top_gap))
                top = aPageArea.size.height - top_gap - mCellHeight;

            constexpr const size_t SLOT_NOT_CHOSEN = std::numeric_limits<size_t>::max();
            size_t slot_no = SLOT_NOT_CHOSEN;
            for (size_t slo = 0; slo < slot_bottom.size(); ++slo) {
                if ((slot_bottom[slo] + gap_between_maps()) < top) {
                    slot_no = slo;
                    break;
                }
            }
            if (slot_no == SLOT_NOT_CHOSEN) {
                if (slot_bottom.size() < aSettings.max_number_columns) {
                    slot_bottom.push_back(0.0);
                    slot_no = slot_bottom.size() - 1;
                }
                else {
                    const auto best = std::min_element(slot_bottom.begin(), slot_bottom.end());
                    slot_no = static_cast<size_t>(best - slot_bottom.begin());
                    top = slot_bottom[slot_no] + gap_between_maps();
                }
            }
            const double left = left_offset() + slot_no * (mCellHeight + gap_between_maps());
            slot_bottom[slot_no] = top + mCellHeight;

            mViewports.emplace_back(Location(left, top), Size(mCellHeight, mCellHeight));
        }
        else {
            mViewports.emplace_back();
        }
    }

} // AntigenicMapsVpos::calculate_viewports

// ----------------------------------------------------------------------

Viewport AntigenicMapsVpos::viewport_of(const Viewport& aViewport, const HzLineSections& /*aSections*/, size_t map_no) const
{
    const Viewport& viewport = mViewports[map_no];
    return Viewport(aViewport.origin + viewport.origin, viewport.size);

} // AntigenicMapsVpos::viewport_of

// ----------------------------------------------------------------------

void AntigenicMapsVpos::draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings) const
{
    AntigenicMaps::draw(aSurface, aViewport, aChart, aSections, aSettings);

      // breaking the antigenic map box outline where section connecting lines meet the box
    for (size_t section_no = 0; section_no < number_of_maps(); ++section_no) {
        const Viewport map_viewport = viewport_of(aViewport, aSections, section_no);
        if (map_viewport.size.width > 0) {
            const double gap = aViewport.size.height * 0.01;
            const double top_y = map_viewport.center().y - gap;
            const double bottom_y = top_y + gap * 2.0;
            aSurface.line({map_viewport.origin.x, top_y}, {map_viewport.origin.x, bottom_y}, aSections.connecting_pipe_background_color, aSettings.border_width * 1.1);
        }
    }

} // AntigenicMapsVpos::draw

// ----------------------------------------------------------------------

Color AntigenicMapsVpos::section_color(const HzLineSections& aSections, size_t /*section_no*/) const
{
    return aSections.this_section_antigen_color;

} // AntigenicMapsVpos::section_color

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
