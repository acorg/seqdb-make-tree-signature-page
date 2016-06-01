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
    inline AntigenicMaps() : mGap(0) {}

    AntigenicMaps& prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings);
    void draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings) const;

    inline Size size(const Viewport& aPageArea, const SettingsAntigenicMaps& /*aSettings*/) const
        {
            return Size(mCellSize.width * mGridWidth + mGap * (mGridWidth - 1) + mLeftOffset, aPageArea.size.height);
        }

    inline const std::vector<size_t>& lines_of_sequenced_antigens_in_chart() const { return mLinesOfSequencedAntigensInChart; }

 private:
    std::vector<std::vector<std::string>> mNamesPerMap;
    size_t mGridWidth, mGridHeight;
    Size mCellSize;
    double mGap;
    double mLeftOffset;
    std::vector<size_t> mLinesOfSequencedAntigensInChart;

    Viewport viewport_of(const Viewport& aViewport, size_t map_no) const;
    std::pair<size_t, size_t> grid() const;

}; // class AntigenicMaps

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
