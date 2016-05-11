#pragma once

#include "draw.hh"

// ----------------------------------------------------------------------

class Title;
class DrawTree;
class Legend;
class TimeSeries;
class Clades;
class SettingsSignaturePage;

class Surface;
class Tree;
class Node;

// ----------------------------------------------------------------------

class SignaturePage
{
 public:
    enum Parts : int { ShowTitle = 1, ShowTree = 2, ShowLegend = 4, ShowTimeSeries = 8, ShowClades = 16, ShowAntigenicMaps = 32 };

    inline SignaturePage() : mParts(ShowTree), mTitle(nullptr), mDrawTree(nullptr), mLegend(nullptr), mTimeSeries(nullptr), mClades(nullptr) {}
    ~SignaturePage();

    SignaturePage& select_parts(int aParts); // use ORed enum Parts above
    SignaturePage& title(const Text& aTitle);
    SignaturePage& color_by_continent(bool aColorByContinent);
    SignaturePage& color_by_pos(int aPos);
    SignaturePage& prepare(Tree& aTree);
    void draw(const Tree& aTree, Surface& aSurface);

 private:
    int mParts;
    Title* mTitle;
    DrawTree* mDrawTree;
    Legend* mLegend;
    TimeSeries* mTimeSeries;
    Clades* mClades;

      // to implement clone all m pointers
    SignaturePage(const SignaturePage&) = default;

}; // class DrawTree

// ----------------------------------------------------------------------

class Title
{
 public:
    inline Title(const Text& aTitle) : mTitle(aTitle) {}

    void draw(Surface& aSurface, const Viewport& aViewport);
    inline Location right_bottom(Surface& aSurface, const Viewport& aViewport) const { return mTitle.right_bottom(aSurface, aViewport); }

 private:
    Text mTitle;

}; // class Title

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
