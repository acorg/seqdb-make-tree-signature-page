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
    virtual inline ~AntigenicMaps() {}

    virtual AntigenicMaps& prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings);
    virtual void draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings) const;

    virtual inline Size size(const Viewport& aPageArea, const SettingsAntigenicMaps& aSettings) const = 0;

    inline const std::vector<size_t>& lines_of_sequenced_antigens_in_chart() const { return mLinesOfSequencedAntigensInChart; }

 protected:
    double left_offset() const { return mLeftOffset; }
    const auto& names_per_map() const { return mNamesPerMap; }

 private:
    std::vector<std::vector<std::string>> mNamesPerMap;
    double mLeftOffset;
    std::vector<size_t> mLinesOfSequencedAntigensInChart;

    virtual Viewport viewport_of(const Viewport& aViewport, size_t map_no) const = 0;

}; // class AntigenicMaps

// ----------------------------------------------------------------------

class AntigenicMapsGrid : public AntigenicMaps
{
 public:
    inline AntigenicMapsGrid() : mGap(0) {}

    virtual inline Size size(const Viewport& aPageArea, const SettingsAntigenicMaps& /*aSettings*/) const
        {
            return Size(mCellSize.width * mGridWidth + mGap * (mGridWidth - 1) + left_offset(), aPageArea.size.height);
        }

    virtual AntigenicMapsGrid& prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings);

 private:
    size_t mGridWidth, mGridHeight;
    Size mCellSize;
    double mGap;

    virtual Viewport viewport_of(const Viewport& aViewport, size_t map_no) const;
    std::pair<size_t, size_t> grid() const;

}; // class AntigenicMapsGrid

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
