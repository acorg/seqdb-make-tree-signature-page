#pragma once

#include "coloring.hh"
#include "legend.hh"

// ----------------------------------------------------------------------

struct GeographicMapPathElement;

// ----------------------------------------------------------------------

class ColoringByGeographyMapLegend : public Legend
{

 public:
    inline ColoringByGeographyMapLegend(const ColoringByContinent& aColoring) : Legend(), mColoring(aColoring) {}

    virtual void draw(Surface& aSurface, const Viewport& aViewport, const SettingsLegend& aSettings) const;
    virtual Size size(Surface& aSurface, const SettingsLegend& aSettings) const;

 private:
    const ColoringByContinent& mColoring;

    cairo_path_t* outline(Surface& aSurface, const std::vector<GeographicMapPathElement>& aPath) const;

}; // class ColoringByGeographyMapLegend

// ----------------------------------------------------------------------
