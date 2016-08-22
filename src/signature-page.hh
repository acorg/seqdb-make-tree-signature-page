#pragma once

#include "draw.hh"

// ----------------------------------------------------------------------

class Title;
class DrawTree;
class DrawHzLines;
class Legend;
class TimeSeries;
class Clades;
class AntigenicMaps;
class SettingsSignaturePage;
class SettingsTitle;

class Tree;
class Node;
class Chart;

// ----------------------------------------------------------------------

class SignaturePage
{
 public:
    enum Parts : int { ShowTitle = 1, ShowTree = 2, ShowLegend = 4, ShowTimeSeries = 8, ShowClades = 16, ShowAntigenicMaps = 32 };

    inline SignaturePage() : mParts(ShowTree), mTitle(nullptr), mDrawTree(nullptr), mLegend(nullptr), mTimeSeries(nullptr), mClades(nullptr),
                             mAntigenicMaps(nullptr), mShowAntigenicMaps(false), mDrawHzLines(nullptr) {}
    ~SignaturePage();

    SignaturePage& select_parts(int aParts); // use ORed enum Parts above
    SignaturePage& title(const Text& aTitle);
    SignaturePage& color_by_continent(bool aColorByContinent);
    SignaturePage& color_by_pos(int aPos);
    SignaturePage& prepare(Tree& aTree, Surface& aSurface, Chart* aChart);
    void draw(const Tree& aTree, Surface& aSurface, const Chart* aChart);

 private:
    int mParts;
    Title* mTitle;
    DrawTree* mDrawTree;
    Legend* mLegend;
    TimeSeries* mTimeSeries;
    Clades* mClades;
    AntigenicMaps* mAntigenicMaps;
    bool mShowAntigenicMaps;
    DrawHzLines* mDrawHzLines;

    Viewport mPageArea;
    Viewport mTitleViewport;
    Viewport mTreeViewport;
    Viewport mLegendViewport;
    Viewport mTimeSeriesViewport;
    Viewport mCladesViewport;
    Viewport mAntigenicMapsViewport;

      // to implement clone all m pointers
    SignaturePage(const SignaturePage&) = default;
    void calculate_viewports(Tree& aTree, Surface& aSurface, Chart* aChart);

}; // class DrawTree

// ----------------------------------------------------------------------

class Title
{
 public:
    inline Title(const Text& aTitle) : mTitle(aTitle) {}
    Title(const SettingsTitle& aSettingsTitle);

    void draw(Surface& aSurface, const Viewport& aViewport);
    inline Location right_bottom(Surface& aSurface, const Viewport& aViewport) const { return mTitle.right_bottom(aSurface, aViewport); }

 private:
    Text mTitle;

}; // class Title

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
