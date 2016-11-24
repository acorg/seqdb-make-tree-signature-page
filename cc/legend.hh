#pragma once

#include "draw.hh"

// ----------------------------------------------------------------------

class SettingsLegend;

// ----------------------------------------------------------------------

class Legend
{
 public:
    inline Legend() {}
    virtual inline ~Legend() = default;

    virtual void draw(Surface& aSurface, const Viewport& aViewport, const SettingsLegend& aSettings) const = 0;
    virtual Size size(Surface& aSurface, const SettingsLegend& aSettings) const = 0;

}; // class Legend

// ----------------------------------------------------------------------
