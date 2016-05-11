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
    aSurface.line(aViewport.origin, aViewport.bottom_right(), 0xFFA500, 3);

} // AntigenicMaps::draw

// ----------------------------------------------------------------------

Size AntigenicMaps::size(Surface& aSurface, const SettingsAntigenicMaps& aSettings) const
{
    return Size(0, 0);

} // AntigenicMaps::size

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
