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
    void draw(Surface& aSurface, const Tree& aTree, const Viewport& aViewport, const Viewport& aTimeSeriesViewport, const DrawTree& aDrawTree, const SettingsClades& aSettings) const;
    Size size(Surface& aSurface, const SettingsClades& aSettings) const;
    inline const std::vector<SettingsClade>& clades() const { return mClades; }

 private:
    std::vector<SettingsClade> mClades;

    void add_clade(const std::pair<std::string, size_t>& aBegin, const std::pair<std::string, size_t>& aEnd, std::string aLabel, std::string aId, const SettingsClades& aSettings);
    void assign_slots(const SettingsClades& aSettings);
    void hide_old_clades(SettingsClade& aClade);

}; // class Clades

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
