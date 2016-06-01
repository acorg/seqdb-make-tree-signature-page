#include "signature-page.hh"
#include "draw-tree.hh"
#include "legend.hh"
#include "time-series.hh"
#include "draw-clades.hh"
#include "tree.hh"
#include "antigenic-maps.hh"
#include "settings.hh"
#include "chart.hh"

// ----------------------------------------------------------------------

SignaturePage::~SignaturePage()
{
    delete mTitle;
    delete mDrawTree;
    delete mLegend;
    delete mTimeSeries;
    delete mClades;
    delete mAntigenicMaps;
    delete mDrawHzLines;

} // SignaturePage::~SignaturePage

// ----------------------------------------------------------------------

SignaturePage& SignaturePage::select_parts(int aParts)
{
    mParts = aParts;
    if (aParts & ShowTree)
        mDrawTree = new DrawTree();
    if (aParts & ShowTimeSeries) {
        mTimeSeries = new TimeSeries();
        mDrawHzLines = new DrawHzLines();
    }
    if (aParts & ShowClades)
        mClades = new Clades();
    if (aParts & ShowAntigenicMaps)
        mAntigenicMaps = new AntigenicMaps();
    return *this;

} // SignaturePage::select_parts

// ----------------------------------------------------------------------

SignaturePage& SignaturePage::title(const Text& aTitle)
{
    if (mParts & ShowTitle)
        mTitle = new Title(aTitle);
    return *this;

} // SignaturePage::title

// ----------------------------------------------------------------------

SignaturePage& SignaturePage::color_by_continent(bool aColorByContinent)
{
    if (mDrawTree)
        mDrawTree->color_by_continent(aColorByContinent);
    return *this;

} // SignaturePage::color_by_continent

// ----------------------------------------------------------------------

SignaturePage& SignaturePage::color_by_pos(int aPos)
{
    if (mDrawTree)
        mDrawTree->color_by_pos(aPos);
    return *this;

} // SignaturePage::color_by_pos

// ----------------------------------------------------------------------

SignaturePage& SignaturePage::prepare(Tree& aTree, Surface& aSurface, Chart* aChart)
{
    if (mDrawTree) {
        const double padding = aSurface.canvas_size().width * aTree.settings().signature_page.outer_padding;
        mPageArea.set(Location(padding, padding), aSurface.canvas_size() - Size(padding + padding, padding + padding));

        mDrawTree->prepare(aTree);
        if (mParts & ShowLegend) {
            mLegend = mDrawTree->coloring().legend();
        }
        if (mTimeSeries)
            mTimeSeries->prepare(aTree, aTree.settings().time_series);
        if (mClades)
            mClades->prepare(aTree, aTree.settings().clades);
        if (mDrawHzLines)
            mDrawHzLines->prepare(aTree, aTree.settings().draw_tree.hz_line_sections);
        if (mAntigenicMaps) {
            if (aChart == nullptr)
                throw std::runtime_error("Antigenic maps part requested but no chart provided to SignaturePage.prepare");
            aChart->preprocess(aTree.settings().antigenic_maps);
            mAntigenicMaps->prepare(aTree, mPageArea, aChart, aTree.settings().draw_tree.hz_line_sections, aTree.settings().antigenic_maps);
        }
    }
    return *this;

} // SignaturePage::prepare

// ----------------------------------------------------------------------

void SignaturePage::draw(const Tree& aTree, Surface& aSurface, const Chart* aChart)
{
    const double canvas_width = aSurface.canvas_size().width;
    const double padding = canvas_width * aTree.settings().signature_page.outer_padding;

    Viewport tree_viewport;
    Viewport legend_viewport;
    Viewport time_series_viewport;
    Viewport clades_viewport;
    Viewport antigenic_maps_viewport;

    double tree_top = padding;
    if (mTitle) {
        const Viewport title_viewport(mPageArea.origin, Size(1, 1));
        mTitle->draw(aSurface, title_viewport);
        tree_top += mTitle->right_bottom(aSurface, title_viewport).y + padding * 0.5;
    }

    if (mDrawTree) {
        const auto time_series_label_height = mTimeSeries ? mTimeSeries->label_height(aSurface, aTree.settings().time_series) : 0;
        if (tree_top < time_series_label_height)
            tree_top = time_series_label_height;

        const auto tree_height = std::min(mPageArea.size.height - tree_top, aSurface.canvas_size().height - tree_top - time_series_label_height - padding * 0.2);

        double left = mPageArea.right();
        if (mAntigenicMaps) {
            const Size size = mAntigenicMaps->size(mPageArea, aTree.settings().antigenic_maps);
            antigenic_maps_viewport.set(mPageArea.top_right() - Size(size.width, 0), size);
            left = antigenic_maps_viewport.origin.x;
        }

        if (mClades) {
            clades_viewport.size = Size(mClades->size(aSurface, aTree.settings().clades).width, tree_height);
            clades_viewport.origin = Location(left - aTree.settings().signature_page.clades_antigenic_maps_space * canvas_width - clades_viewport.size.width, tree_top);
            left = clades_viewport.origin.x;
        }
        else {
            left -= aTree.settings().signature_page.clades_antigenic_maps_space * canvas_width;
        }

        if (mTimeSeries) {
            time_series_viewport.size = Size(mTimeSeries->size(aSurface, aTree.settings().time_series).width, tree_height);
            time_series_viewport.origin = Location(left - aTree.settings().signature_page.time_series_clades_space * canvas_width - time_series_viewport.size.width, tree_top);
            left = time_series_viewport.origin.x;
        }

        tree_viewport.set(Location(mPageArea.origin.x, tree_top), Size(left - mPageArea.origin.x - aTree.settings().signature_page.tree_time_series_space * canvas_width, tree_height));
        mDrawTree->draw(aTree, aSurface, tree_viewport, aTree.settings().draw_tree);

          // Note legend must be drawn after tree, because ColoringByPos needs to collect data to be drawn in the legend
        if (mLegend) {
            const auto legend_size = mLegend->size(aSurface, aTree.settings().legend);
            legend_viewport.set({mPageArea.origin.x, mPageArea.bottom() - legend_size.height}, legend_size);
            mLegend->draw(aSurface, legend_viewport, aTree.settings().legend);
        }

        if (mTimeSeries) {
            mTimeSeries->draw(aSurface, time_series_viewport, aTree, *mDrawTree, aTree.settings().time_series);
        }
        if (mClades) {
            mClades->draw(aSurface, clades_viewport, time_series_viewport, *mDrawTree, aTree.settings().clades);
        }
        if (mDrawHzLines) {
            mDrawHzLines->draw(aSurface, time_series_viewport, antigenic_maps_viewport, *mDrawTree, aTree.settings().draw_tree.hz_line_sections);
        }
        if (mAntigenicMaps) {
            mAntigenicMaps->draw(aSurface, antigenic_maps_viewport, aChart, aTree.settings().draw_tree.hz_line_sections, aTree.settings().antigenic_maps);
        }
    }

} // SignaturePage::draw

// ----------------------------------------------------------------------

void Title::draw(Surface& aSurface, const Viewport& aViewport)
{
    mTitle.draw(aSurface, aViewport);

} // Title::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
