#pragma once

#include "draw.hh"
#include "settings.hh"

// ----------------------------------------------------------------------

class Surface;
class Viewport;
class Tree;
class Chart;
class DrawTree;

// ----------------------------------------------------------------------

class AntigenicMaps
{
 public:
    inline AntigenicMaps() : mGap(0) {}
    virtual inline ~AntigenicMaps() {}

    virtual AntigenicMaps& prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings);
    virtual void draw(Surface& aSurface, const Viewport& aViewport, const Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings) const;

    virtual inline Size size(const Viewport& aPageArea, const SettingsAntigenicMaps& aSettings) const = 0;

    inline const std::vector<size_t>& lines_of_sequenced_antigens_in_chart() const { return mLinesOfSequencedAntigensInChart; }

 protected:
    double left_offset() const { return mLeftOffset; }
    const auto& names_per_map() const { return mNamesPerMap; }
    virtual Viewport viewport_of(const Viewport& aViewport, size_t map_no) const = 0;
    double gap_between_maps() const { return mGap; }

 private:
    std::vector<std::vector<std::string>> mNamesPerMap;
    double mLeftOffset;
    std::vector<size_t> mLinesOfSequencedAntigensInChart;
    double mGap;

}; // class AntigenicMaps

// ----------------------------------------------------------------------

class AntigenicMapsGrid : public AntigenicMaps
{
 public:
    virtual inline Size size(const Viewport& aPageArea, const SettingsAntigenicMaps& /*aSettings*/) const
        {
            return Size(mCellSize.width * mGridWidth + gap_between_maps() * (mGridWidth - 1) + left_offset(), aPageArea.size.height);
        }

    virtual AntigenicMapsGrid& prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings);

 protected:
    virtual Viewport viewport_of(const Viewport& aViewport, size_t map_no) const;

 private:
    size_t mGridWidth, mGridHeight;
    Size mCellSize;

    std::pair<size_t, size_t> grid() const;

}; // class AntigenicMapsGrid

// ----------------------------------------------------------------------

class AntigenicMapsVpos : public AntigenicMaps
{
 public:
    virtual inline Size size(const Viewport& aPageArea, const SettingsAntigenicMaps& /*aSettings*/) const;
    virtual AntigenicMapsVpos& prepare(const Tree& aTree, const Viewport& aPageArea, Chart* aChart, const HzLineSections& aSections, const SettingsAntigenicMaps& aSettings);

 protected:
    virtual inline Viewport viewport_of(const Viewport& aViewport, size_t map_no) const;

 private:
    std::vector<Viewport> mViewports;
    double mCellHeight;

}; // class AntigenicMapsVpos

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
