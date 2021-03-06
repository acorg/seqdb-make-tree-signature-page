#pragma once

#include "draw.hh"
#include "coloring.hh"
#include "settings.hh"

// ----------------------------------------------------------------------

class Tree;
class Node;
class AntigenicMaps;

// ----------------------------------------------------------------------

class DrawTree
{
 public:
    inline DrawTree() : mColoring(new ColoringBlack()), mVerticalStep(0) {}
    inline ~DrawTree() { delete mColoring; }

    DrawTree& prepare(Tree& aTree, const SettingsDrawTree& aSettings);
    void calculate_viewports(const Tree& aTree, const Viewport& aViewport);
    inline DrawTree& color_by_continent(bool aColorByContinent) { if (aColorByContinent) { delete mColoring; mColoring = new ColoringByContinent(); } return *this; }
    inline DrawTree& color_by_pos(int aPos) { if (aPos >= 0) { delete mColoring; mColoring = new ColoringByPos(static_cast<size_t>(aPos)); } return *this; }

    inline const Coloring& coloring() const { return *mColoring; }
    inline double vertical_step() const { return mVerticalStep; }
    inline size_t number_of_lines() const { return mNumberOfLines; }

    void draw(const Tree& aTree, Surface& aSurface, const Viewport& aViewport, const SettingsDrawTree& aSettings);

 private:
    Coloring* mColoring;

      // receive their values in draw()
    double mWidth;
    double mHorizontalStep;
    double mVerticalStep;
    double mLineWidth;
    double mLabelScale;
    size_t mNumberOfLines;

    Viewport mViewport;         // to avoid passing via draw_node recusrsive calls

    void add_hz_line_sections_gap(Tree& aTree, const HzLineSections& aSections);
    void set_label_scale(Surface& surface, const Tree& aTree, const Viewport& aViewport, const SettingsDrawTree& aSettings);
    void set_horizontal_step(Surface& surface, const Tree& aTree, const Viewport& aViewport, const SettingsDrawTree& aSettings);
    double tree_width(Surface& surface, const Node& aNode, const SettingsDrawTree& aSettings, double aEdgeLength = -1.0) const;
    void draw_node(const Node& aNode, Surface& surface, const Location& aOrigin, const SettingsDrawTree& aSettings, double aEdgeLength = -1.0);
    void draw_aa_transition(const Node& aNode, Surface& aSurface, const Viewport& aViewport, const SettingsAATransition& aSettings);
    void draw_grid(Surface& aSurface, const Viewport& aViewport, const SettingsDrawTree& aSettings);
    void mark_nodes(Surface& aSurface);

      // to implement clone mColoring
    DrawTree(const DrawTree&) = default;

    class NodeToMark
    {
     public:
        inline NodeToMark(const SettingsMarkNodeOnTree& aMarkData) : node(nullptr), mark_data(aMarkData) {}
        inline void set(const Location& aLocation, const Node& aNode) { location = aLocation; node = &aNode; }

        const Node* node;
        Location location;
        const SettingsMarkNodeOnTree& mark_data;

    }; // class NodeToMark

    std::map<std::string, NodeToMark> mNodesToMark;

}; // class DrawTree

// ----------------------------------------------------------------------

class DrawHzLines
{
 public:
    inline DrawHzLines() {}

    DrawHzLines& prepare(Tree& aTree, HzLineSections& aSections);
    void draw(Surface& aSurface, const Viewport& aTimeSeries, const Viewport& aAntigenicMapsViewport, const DrawTree& aDrawTree, const AntigenicMaps* aAntigenicMaps, const SettingsAntigenicMaps& aAntigenicMapsSettings, const HzLineSections& aSections, SettingsSignaturePage::Layout aLayout) const;

    static std::string section_label(const HzLineSections& aSections, size_t aSectionNo, bool aJustIndex);

    template <typename Index> static inline Index section_index(const HzLineSections& aSections, size_t aSectionNo, Index aStart)
        {
            for (size_t no = 0; no < aSectionNo; ++no) {
                if (aSections[no].show_map)
                    ++aStart;
            }
            return aStart;
        }

 private:
    void draw_section_lines_right(Surface& aSurface, const Viewport& aTimeSeriesViewport, const Viewport& aAntigenicMapsViewport, const Viewport& aMapViewport, const SettingsAntigenicMaps& aAntigenicMapsSettings, double first_y, double last_y, double vertical_step, const HzLineSections& aSections) const;
    void draw_section_lines_left(Surface& aSurface, const Viewport& aTimeSeriesViewport, const Viewport& aAntigenicMapsViewport, const Viewport& aMapViewport, const SettingsAntigenicMaps& aAntigenicMapsSettings, double first_y, double last_y, double vertical_step, const HzLineSections& aSections) const;
    void draw_sequenced_right(Surface& aSurface, const Viewport& aTimeSeriesViewport, const AntigenicMaps& aAntigenicMaps, const Viewport& aAntigenicMapsViewport, const HzLineSections& aSections, double vertical_step) const;
    void draw_sequenced_left(Surface& aSurface, const Viewport& aTimeSeriesViewport, const AntigenicMaps& aAntigenicMaps, const Viewport& aAntigenicMapsViewport, const HzLineSections& aSections, double vertical_step) const;
    void draw_section_bracket(Surface& aSurface, const Viewport& aTimeSeriesViewport, const SettingsAntigenicMaps& aAntigenicMapsSettings, double first_y, double last_y, double vertical_step, const HzLineSections& aSections) const;
    void draw_section_label(Surface& aSurface, const Viewport& aTimeSeriesViewport, double first_y, double last_y, const HzLineSections& aSections, size_t aSectionNo) const;

}; // class DrawHzLines

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
