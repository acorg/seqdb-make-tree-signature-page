#include "antigenic-maps.hh"
#include "tree.hh"

// ----------------------------------------------------------------------

AntigenicMaps& AntigenicMaps::prepare(const Tree& aTree, const SettingsAntigenicMaps& aSettings)
{
    return *this;

} // AntigenicMaps::prepare

// ----------------------------------------------------------------------

void AntigenicMaps::draw(Surface& aSurface, const Viewport& aViewport, const SettingsAntigenicMaps& aSettings) const
{
    Viewport map_viewport(aViewport.origin, Size(aViewport.size.width/2, aViewport.size.width/2));

    aSurface.line(map_viewport.origin, map_viewport.top_right(), aSettings.border_color, aSettings.border_width);
    aSurface.line(map_viewport.origin, map_viewport.bottom_left(), aSettings.border_color, aSettings.border_width);
    aSurface.line(map_viewport.bottom_right(), map_viewport.top_right(), aSettings.border_color, aSettings.border_width);
    aSurface.line(map_viewport.bottom_right(), map_viewport.bottom_left(), aSettings.border_color, aSettings.border_width);

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
