#include "antigenic-maps.hh"
#include "tree.hh"
#include "chart.hh"

// ----------------------------------------------------------------------

AntigenicMaps& AntigenicMaps::prepare(const Tree& aTree, const HzLineSections& aSections, const SettingsAntigenicMaps& /*aSettings*/)
{
    if (!aSections.empty()) {
        for (const auto& section: aSections) {
            mNamesPerMap.push_back(aTree.names_between(section.first_name, section.last_name));
        }
    }
    else {
        mNamesPerMap.push_back(aTree.names());
    }
    return *this;

} // AntigenicMaps::prepare

// ----------------------------------------------------------------------

void AntigenicMaps::draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings) const
{
    for (size_t section_no = 0; section_no < mNamesPerMap.size(); ++section_no) {
        const Viewport map_viewport = viewport_of(aViewport, section_no); //(aViewport.origin, Size(aViewport.size.width/2, aViewport.size.width/2));

        aSurface.rectangle(map_viewport, aSettings.border_color, aSettings.border_width);

        const auto chart_viewport = aChart->viewport();

        const double scale = aSurface.set_clip_region(map_viewport, chart_viewport.size.width);
        std::cerr << "map scale " << scale << std::endl;
        aSurface.grid(chart_viewport, 1, aSettings.grid_color, aSettings.grid_line_width * scale);

        aChart->draw(aSurface, scale, aSettings);
        aSurface.reset_clip_region();
    }

      // aSurface.circle_filled(Location(0, 0), scale * 5, 1.0, 0.0, 0xFF0000, scale * 0.5, 0xE0E0FF);
      // aSurface.circle_filled(Location(1, 1), scale * 10, 1.0, 0.0, 0x000000, scale * 0.5, 0xE0E0FF);

      // aSurface.line(Location(0, 0), Location(1.5, 1), 0xFF0000, scale);
      // aSurface.line(Location(0, 0), Location(1, 1.5), 0xFF00A5, scale);
      // aSurface.rectangle(Location(0.1, 0.1), Size(0.8, 0.8), 0xA5FF00, scale);
      //   // aSurface.rectangle_filled(Location(0.05, 0.2), Size(0.1, 0.1), 0xA5FF00, scale, 0x80FFA500);
      // aSurface.square_filled(Location(0.3, 0.3), 0.1, 0.5, M_PI_2 / 3, 0x000000, scale, 0x808080);
      // aSurface.square_filled(Location(0.3, 0.5), 0.1, 0.5, 0.0, 0x000000, scale, 0x808080);
      // aSurface.circle_filled(Location(0.5, 0.3), 0.1, 0.5, M_PI_2 / 3, 0x000000, scale, 0xA0A0FF);
      // aSurface.circle_filled(Location(0.5, 0.5), 0.1, 2.0, 0.0, 0x000000, scale, 0x8080FF);
      // aSurface.circle_filled(Location(0.5, 0.7), 0.1, 1.0, 0.0, 0x000000, scale, 0xE0E0FF);
      // aSurface.triangle_filled(Location(0.7, 0.3), 0.1, 0.5, M_PI_2 / 3, 0x000000, scale, 0xA0FFA0);
      // aSurface.triangle_filled(Location(0.7, 0.5), 0.1, 2.0, 0.0, 0x000000, scale, 0x80FF80);
      // aSurface.triangle_filled(Location(0.7, 0.7), 0.1, 1.0, 0.0, 0x000000, scale, 0xE0FFE0);


} // AntigenicMaps::draw

// ----------------------------------------------------------------------

Viewport AntigenicMaps::viewport_of(const Viewport& aViewport, size_t map_no) const
{
    size_t grid_w, grid_h;
    switch (mNamesPerMap.size()) {
      case 1: grid_w = grid_h = 1; break;
      case 2: grid_w = 1; grid_h = 2; break;
      case 3: grid_w = 2; grid_h = 2; break;
      case 4: grid_w = 2; grid_h = 2; break;
      case 5: grid_w = 3; grid_h = 2; break;
      case 6: grid_w = 3; grid_h = 2; break;
      case 7: grid_w = 3; grid_h = 3; break;
      case 8: grid_w = 3; grid_h = 3; break;
      case 9: grid_w = 3; grid_h = 3; break;
      default: throw std::runtime_error("AntigenicMaps: unsupported number of maps: " + std::to_string(mNamesPerMap.size()));
    }

    const size_t cell_x = map_no % grid_w;
    const size_t cell_y = map_no / grid_w;
    const Size size(aViewport.size.width / grid_w, aViewport.size.width / grid_w);
    std::cout << cell_x << " " << cell_y << " " << aViewport.origin + Size(size.width * cell_x, size.height * cell_y) << " " << size << std::endl;
    return Viewport(aViewport.origin + Size(size.width * cell_x, size.height * cell_y), size);

} // AntigenicMaps::viewport_of

// ----------------------------------------------------------------------

// Size AntigenicMaps::size(Surface& aSurface, const SettingsAntigenicMaps& aSettings) const
// {
//     return Size(0, 0);

// } // AntigenicMaps::size

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
