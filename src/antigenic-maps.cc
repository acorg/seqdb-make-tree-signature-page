#include "antigenic-maps.hh"
#include "tree.hh"
#include "chart.hh"

// ----------------------------------------------------------------------

AntigenicMaps& AntigenicMaps::prepare(const Tree& aTree, const Viewport& aPageArea, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings)
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

    mGap = aSettings.gap_between_maps * aPageArea.size.width;
    std::tie(mGridWidth, mGridHeight) = grid();
    const double size = (aPageArea.size.height - mGap * (mGridHeight - 1)) / mGridHeight;
    mCellSize.set(size, size);
    mLeftOffset = aSections.empty() ? 0.0 : aSections[0].line_width * 2.0;

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

Viewport AntigenicMaps::viewport_of(const Viewport& aViewport, size_t map_no) const
{
    const size_t cell_x = map_no % mGridWidth;
    const size_t cell_y = map_no / mGridWidth;
    return Viewport(Location(aViewport.origin.x + mLeftOffset + (mCellSize.width + mGap) * cell_x, aViewport.origin.y + (mCellSize.height + mGap) * cell_y), mCellSize);

} // AntigenicMaps::viewport_of

// ----------------------------------------------------------------------

std::pair<size_t, size_t> AntigenicMaps::grid() const
{
    size_t grid_w, grid_h;
    switch (mNamesPerMap.size()) {
      case 1: grid_w = grid_h = 1; break;
      case 2: grid_w = 1; grid_h = 2; break;
      case 3: grid_w = 2; grid_h = 2; break;
      case 4: grid_w = 2; grid_h = 2; break;
      case 5: grid_w = 2; grid_h = 3; break;
      case 6: grid_w = 2; grid_h = 3; break;
      case 7: grid_w = 3; grid_h = 3; break;
      case 8: grid_w = 3; grid_h = 3; break;
      case 9: grid_w = 3; grid_h = 3; break;
      default: throw std::runtime_error("AntigenicMaps: unsupported number of maps: " + std::to_string(mNamesPerMap.size()));
    }

    return std::make_pair(grid_w, grid_h);

} // AntigenicMaps::grid

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
