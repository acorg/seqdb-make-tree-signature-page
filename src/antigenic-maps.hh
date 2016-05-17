#pragma once

#include "draw.hh"
#include "settings.hh"

// ----------------------------------------------------------------------

class Surface;
class Viewport;
class Tree;
class Chart;

// ----------------------------------------------------------------------

class AntigenicMaps
{
 public:
    inline AntigenicMaps() {}

    AntigenicMaps& prepare(const Tree& aTree, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings);
    void draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings) const;
      // Size size(Surface& aSurface, const SettingsAntigenicMaps& aSettings) const;

 private:
    std::vector<std::vector<std::string>> mNamesPerMap;

    Viewport viewport_of(const Viewport& aViewport, size_t map_no, const HzLineSections& aSections) const;

}; // class AntigenicMaps

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
