#include "antigenic-maps.hh"
#include "tree.hh"
#include "chart.hh"

// ----------------------------------------------------------------------

AntigenicMaps& AntigenicMaps::prepare(const Tree& aTree, const SettingsAntigenicMaps& aSettings)
{
    return *this;

} // AntigenicMaps::prepare

// ----------------------------------------------------------------------

void AntigenicMaps::draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const SettingsAntigenicMaps& aSettings) const
{
    Viewport map_viewport(aViewport.origin, Size(aViewport.size.width/2, aViewport.size.width/2));

    aSurface.rectangle(map_viewport, aSettings.border_color, aSettings.border_width);
    // aSurface.line(map_viewport.origin, map_viewport.top_right(), aSettings.border_color, aSettings.border_width);
    // aSurface.line(map_viewport.origin, map_viewport.bottom_left(), aSettings.border_color, aSettings.border_width);
    // aSurface.line(map_viewport.bottom_right(), map_viewport.top_right(), aSettings.border_color, aSettings.border_width);
    // aSurface.line(map_viewport.bottom_right(), map_viewport.bottom_left(), aSettings.border_color, aSettings.border_width);

    const auto chart_viewport = aChart->viewport();
    const double scale = aSurface.set_clip_region(map_viewport, chart_viewport.size.width);
    std::cerr << "map scale " << scale << std::endl;
    aSurface.grid(chart_viewport, 1, 0x8000FF00, scale / 2);

    aSurface.circle_filled(Location(0, 0), scale * 5, 1.0, 0.0, 0xFF0000, scale * 0.5, 0xE0E0FF);
    aSurface.circle_filled(Location(1, 1), scale * 10, 1.0, 0.0, 0x000000, scale * 0.5, 0xE0E0FF);

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

// Size AntigenicMaps::size(Surface& aSurface, const SettingsAntigenicMaps& aSettings) const
// {
//     return Size(0, 0);

// } // AntigenicMaps::size

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
