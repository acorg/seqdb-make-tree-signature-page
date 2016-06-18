#include "antigenic-maps.hh"
#include "tree.hh"
#include "draw-tree.hh"
#include "chart.hh"

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

    return *this;

} // AntigenicMaps::prepare

// ----------------------------------------------------------------------

void AntigenicMaps::draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings) const
{
    for (size_t section_no = 0; section_no < mNamesPerMap.size(); ++section_no) {
        const Viewport map_viewport = viewport_of(aViewport, section_no);

        Surface::PushContext pc(aSurface);
        aSurface.rectangle(map_viewport, aSettings.border_color, aSettings.border_width);

        const auto chart_viewport = aChart->viewport();

        const double scale = aSurface.set_clip_region(map_viewport, chart_viewport.size.width);
          // std::cerr << "map scale " << scale << std::endl;
        aSurface.grid(chart_viewport, 1, aSettings.grid_color, aSettings.grid_line_width * scale);

        aChart->draw_points_reset(aSettings);
        const auto num_antigens = aChart->tracked_antigens(mNamesPerMap[section_no], aSections[section_no].color, aSettings);
        aChart->draw(aSurface, scale, aSettings);
        std::cout << "Section " << aSections[section_no].first_line << " " << aSections[section_no].first_name << " " << aSections[section_no].color << " names: " << mNamesPerMap[section_no].size() << " antigens: " << num_antigens << std::endl;
    }

} // AntigenicMaps::draw

// ----------------------------------------------------------------------

AntigenicMapsGrid& AntigenicMapsGrid::prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings)
{
    AntigenicMaps::prepare(aTree, aPageArea, aChart, aSections, aSettings);

    std::tie(mGridWidth, mGridHeight) = grid();
    const double size = (aPageArea.size.height - gap_between_maps() * (mGridHeight - 1)) / mGridHeight;
    mCellSize.set(size, size);

    return *this;

} // AntigenicMapsGrid::prepare

// ----------------------------------------------------------------------

void AntigenicMapsGrid::calculate_viewports(Tree& /*aTree*/, Surface& /*aSurface*/, const Viewport& /*aViewport*/, const DrawTree& /*aDrawTree*/, const HzLineSections& /*aSections*/, const SettingsAntigenicMaps& /*aSettings*/)
{

} // AntigenicMapsGrid::calculate_viewports

// ----------------------------------------------------------------------

Viewport AntigenicMapsGrid::viewport_of(const Viewport& aViewport, size_t map_no) const
{
    const size_t cell_x = map_no % mGridWidth;
    const size_t cell_y = map_no / mGridWidth;
    return Viewport(Location(aViewport.origin.x + left_offset() + (mCellSize.width + gap_between_maps()) * cell_x, aViewport.origin.y + (mCellSize.height + gap_between_maps()) * cell_y), mCellSize);

} // AntigenicMapsGrid::viewport_of

// ----------------------------------------------------------------------

std::pair<size_t, size_t> AntigenicMapsGrid::grid() const
{
    size_t grid_w, grid_h;
    switch (names_per_map().size()) {
      case 1: grid_w = grid_h = 1; break;
      case 2: grid_w = 1; grid_h = 2; break;
      case 3: grid_w = 2; grid_h = 2; break;
      case 4: grid_w = 2; grid_h = 2; break;
      case 5: grid_w = 2; grid_h = 3; break;
      case 6: grid_w = 2; grid_h = 3; break;
      case 7: grid_w = 3; grid_h = 3; break;
      case 8: grid_w = 3; grid_h = 3; break;
      case 9: grid_w = 3; grid_h = 3; break;
      default: throw std::runtime_error("AntigenicMaps: unsupported number of maps: " + std::to_string(names_per_map().size()));
    }

    return std::make_pair(grid_w, grid_h);

} // AntigenicMapsGrid::grid

// ----------------------------------------------------------------------

inline Size AntigenicMapsVpos::size(const Viewport& /*aPageArea*/, const SettingsAntigenicMaps& /*aSettings*/) const
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
      // settings!!
    const double map_height_fraction_of_page = 0.2;

    mCellHeight = aPageArea.size.height * map_height_fraction_of_page;
    AntigenicMaps::prepare(aTree, aPageArea, aChart, aSections, aSettings);

    return *this;

} // AntigenicMapsVpos::prepare

// ----------------------------------------------------------------------

void AntigenicMapsVpos::calculate_viewports(Tree& aTree, Surface& aSurface, const Viewport& aViewport, const DrawTree& aDrawTree, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings)
{
    double vertical_step = aDrawTree.vertical_step();

    if (vertical_step > 0)
        mViewports.clear();         // re-calculate final viewports
    else
        vertical_step = aViewport.size.height / (aTree.height() + 2) * 0.9; // preliminary step to get preliminary maps layout

    std::vector<double> slot_bottom;

    for (size_t section_no = 0; section_no < aSections.size(); ++section_no) {
        const auto& section = aSections[section_no];
        const auto first_line = section.first_line;
        const auto last_line = section_no < (aSections.size() - 1) ? aSections[section_no + 1].first_line - aSections.vertical_gap - 1 : aTree.height() - 1;
        const double middle = (first_line + last_line) / 2.0 * vertical_step;
        double top = middle - mCellHeight / 2.0;
        std::cerr << "section_no:" << section_no << " top:" << top << " first_line:" << first_line << " last_line:" << last_line << " middle:" << middle << std::endl;
        if (top < 0.0)
            top = 0.0;
        else if ((top + mCellHeight) > aViewport.size.height)
            top = aViewport.size.height - mCellHeight;

        constexpr const size_t SLOT_NOT_CHOSEN = std::numeric_limits<size_t>::max();
        size_t slot_no = SLOT_NOT_CHOSEN;
        for (size_t slo = 0; slo < slot_bottom.size(); ++slo) {
            if ((slot_bottom[slo] + gap_between_maps()) < top) {
                slot_no = slo;
                break;
            }
        }
        if (slot_no == SLOT_NOT_CHOSEN) {
            slot_bottom.push_back(0.0);
            slot_no = slot_bottom.size() - 1;
        }
        const double left = left_offset() + slot_no * (mCellHeight + gap_between_maps());
        std::cerr << "slot:" << slot_no << " slot_bottom:" << slot_bottom[slot_no] << " top:" << top << " " << ((slot_bottom[slot_no] + gap_between_maps()) < top) << std::endl;
        slot_bottom[slot_no] = top + mCellHeight;

        mViewports.emplace_back(Location(left, top), Size(mCellHeight, mCellHeight));
    }

    std::cerr << "-------------------------------\n\n" << std::endl;

} // AntigenicMapsVpos::calculate_viewports

// ----------------------------------------------------------------------

Viewport AntigenicMapsVpos::viewport_of(const Viewport& aViewport, size_t map_no) const
{
    const Viewport& viewport = mViewports[map_no];
    return Viewport(aViewport.origin + viewport.origin, viewport.size);

} // AntigenicMapsVpos::viewport_of

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
