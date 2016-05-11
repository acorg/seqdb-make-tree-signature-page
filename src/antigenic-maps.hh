#pragma once

#include "draw.hh"
#include "settings.hh"

// ----------------------------------------------------------------------

class Surface;
class Viewport;
class Tree;

// ----------------------------------------------------------------------

class AntigenicMaps
{
 public:
    inline AntigenicMaps() {}

    AntigenicMaps& prepare(const Tree& aTree, const SettingsAntigenicMaps& aSettings);
    void draw(Surface& aSurface, const Viewport& aViewport, const SettingsAntigenicMaps& aSettings) const;
    Size size(Surface& aSurface, const SettingsAntigenicMaps& aSettings) const;

 private:

}; // class AntigenicMaps

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
