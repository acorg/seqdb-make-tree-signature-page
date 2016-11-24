#pragma once

#include "draw.hh"
#include "date.hh"

class DrawTree;
class SettingsTimeSeries;
class Tree;
class Coloring;

// ----------------------------------------------------------------------

class TimeSeries
{
 public:
    inline TimeSeries() {}

    TimeSeries& prepare(const Tree& aTree, const SettingsTimeSeries& aSettings);
    void draw(Surface& aSurface, const Viewport& aViewport, const Tree& aTree, const DrawTree& aDrawTree, const SettingsTimeSeries& aSettings) const;
    double label_height(Surface& aSurface, const SettingsTimeSeries& aSettings) const;
    Size size(Surface& aSurface, const SettingsTimeSeries& aSettings) const;

 private:
    Date mBegin, mEnd;
    size_t mNumberOfMonths;

    void draw_labels(Surface& aSurface, const Viewport& aViewport, const SettingsTimeSeries& aSettings) const;
    void draw_labels_at_side(Surface& aSurface, const Location& aOrigin, double label_font_size, double month_max_width, const SettingsTimeSeries& aSettings) const;
    void draw_month_separators(Surface& aSurface, const Viewport& aViewport, const SettingsTimeSeries& aSettings) const;
    void draw_dashes(Surface& aSurface, const Viewport& aViewport, const Tree& aTree, const DrawTree& aDrawTree, const SettingsTimeSeries& aSettings) const;

}; // class TimeSeries

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
