#pragma once

#include <vector>

#include "draw.hh"
#include "settings.hh"

// ----------------------------------------------------------------------

class Surface;
class Viewport;
class Tree;
class DrawTree;

// ----------------------------------------------------------------------

class Clades
{
 public:
    inline Clades() {}

    Clades& prepare(const Tree& aTree, const SettingsClades& aSettings);
    void draw(Surface& aSurface, const Viewport& aViewport, const Viewport& aTimeSeriesViewport, const DrawTree& aDrawTree, const SettingsClades& aSettings) const;
    Size size(Surface& aSurface, const SettingsClades& aSettings) const;
    inline const std::vector<SettingsClade>& clades() const { return mClades; }

 private:
    std::vector<SettingsClade> mClades;

    void add_clade(int aBegin, int aEnd, std::string aLabel, std::string aId, const SettingsClades& aSettings);
    void assign_slots(const SettingsClades& aSettings);

}; // class Clades

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
