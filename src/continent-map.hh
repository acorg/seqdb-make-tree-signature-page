#pragma once

#include "coloring.hh"
#include "legend.hh"

// ----------------------------------------------------------------------

struct ContinentMapPathElement;

// ----------------------------------------------------------------------

class ColoringByContinentMapLegend : public Legend
{

 public:
    inline ColoringByContinentMapLegend(const ColoringByContinent& aColoring) : Legend(), mColoring(aColoring) {}

    virtual void draw(Surface& aSurface, const Viewport& aViewport, const SettingsLegend& aSettings) const;
    virtual Size size(Surface& aSurface, const SettingsLegend& aSettings) const;

 private:
    const ColoringByContinent& mColoring;

    cairo_path_t* outline(Surface& aSurface, const std::map<std::string, std::vector<ContinentMapPathElement>>& aPath, std::string aContinent) const;

}; // class ColoringByContinentMapLegend

// ----------------------------------------------------------------------
