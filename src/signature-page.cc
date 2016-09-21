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
        if (aParts & ShowAntigenicMaps) {
            mShowAntigenicMaps = true;
        }
    }
    if (aParts & ShowClades)
        mClades = new Clades();
    return *this;

} // SignaturePage::select_parts

// ----------------------------------------------------------------------

SignaturePage& SignaturePage::title(const Text& aTitle)
{
    if (mParts & ShowTitle) {
        if (mTitle)
            delete mTitle;
        mTitle = new Title(aTitle);
    }
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
        if (mParts & ShowTitle && mTitle == nullptr) {
            mTitle = new Title(aTree.settings().title);
        }
        const double canvas_width = aSurface.canvas_size().width;
        const double padding_left = canvas_width * aTree.settings().signature_page.padding_left;
        const double padding_right = canvas_width * aTree.settings().signature_page.padding_right;
        const double padding_top = canvas_width * aTree.settings().signature_page.padding_top;
        const double padding_bottom = canvas_width * aTree.settings().signature_page.padding_bottom;
        mPageArea.set(Location(padding_left, padding_top), aSurface.canvas_size() - Size(padding_left + padding_right, padding_top + padding_bottom));

        mDrawTree->prepare(aTree, aTree.settings().draw_tree);
        if (mParts & ShowLegend) {
            mLegend = mDrawTree->coloring().legend(aTree.settings().legend);
        }
        if (mTimeSeries)
            mTimeSeries->prepare(aTree, aTree.settings().time_series);
        if (mClades)
            mClades->prepare(aTree, aTree.settings().clades);
        if (mDrawHzLines) {
            mDrawHzLines->prepare(aTree, aTree.settings().draw_tree.hz_line_sections);
            if (mShowAntigenicMaps) {
                switch (aTree.settings().draw_tree.hz_line_sections.mode) {
                  case HzLineSections::ColoredGrid:
                      std::cout << "ColoredGrid mode" << std::endl;
                      mAntigenicMaps = new AntigenicMapsColoredGrid();
                      break;
                  case HzLineSections::BWVpos:
                      std::cout << "BWVpos mode" << std::endl;
                      mAntigenicMaps = new AntigenicMapsVpos();
                      break;
                  case HzLineSections::NamedGrid:
                      std::cout << "NamedGrid mode" << std::endl;
                      mAntigenicMaps = new AntigenicMapsNamedGrid();
                      break;
                }
                if (aChart == nullptr)
                    throw std::runtime_error("Antigenic maps part requested but no chart provided to SignaturePage.prepare");
                aChart->preprocess(aTree.settings().antigenic_maps);
                mAntigenicMaps->prepare(aTree, mPageArea, aChart, aTree.settings().draw_tree.hz_line_sections, aTree.settings().antigenic_maps);
            }
        }
    }
    calculate_viewports(aTree, aSurface, aChart);
    return *this;

} // SignaturePage::prepare

// ----------------------------------------------------------------------

void SignaturePage::calculate_viewports(Tree& aTree, Surface& aSurface, Chart* aChart)
{
    const double canvas_width = aSurface.canvas_size().width;
    const double padding_top = canvas_width * aTree.settings().signature_page.padding_top;

    double tree_top = padding_top;

    if (mTitle) {
        mTitleViewport.set(Location(), Size(1, 1));
    }

    if (mDrawTree) {
        const auto time_series_label_height = mTimeSeries ? mTimeSeries->label_height(aSurface, aTree.settings().time_series) : 0;
        if (tree_top < time_series_label_height)
            tree_top = time_series_label_height;
        const Location tree_origin {mPageArea.origin.x, tree_top};

        const auto tree_height = std::min(mPageArea.size.height, aSurface.canvas_size().height - time_series_label_height * 2);

        double left = mPageArea.right();
        if (mAntigenicMaps) {
            mAntigenicMaps->calculate_viewports(aTree, aChart, Viewport(tree_origin, Size(mPageArea.size.width, tree_height)), mPageArea, *mDrawTree, aTree.settings().draw_tree.hz_line_sections, aTree.settings().antigenic_maps);
            const Size size = mAntigenicMaps->size(mPageArea, aTree.settings().antigenic_maps);
            mAntigenicMapsViewport.set(mPageArea.top_right() - Size(size.width, 0), size);
            left = mAntigenicMapsViewport.origin.x;
        }

        if (mTimeSeries) {
            if (mClades) {
                mCladesViewport.size = Size(mClades->size(aSurface, aTree.settings().clades).width, tree_height);
            }

            mTimeSeriesViewport.size = Size(mTimeSeries->size(aSurface, aTree.settings().time_series).width, tree_height);
        }

        switch (aTree.settings().signature_page.layout) {
          case SettingsSignaturePage::TreeTimeseriesCladesMaps:
              if (mTimeSeries) {
                  if (mClades) {
                      mCladesViewport.origin = Location(left - aTree.settings().signature_page.clades_antigenic_maps_space * canvas_width - mCladesViewport.size.width, tree_top);
                      left = mCladesViewport.origin.x;
                  }
                  else {
                      left -= aTree.settings().signature_page.clades_antigenic_maps_space * canvas_width;
                  }
                  mTimeSeriesViewport.origin = Location(left - aTree.settings().signature_page.time_series_clades_space * canvas_width - mTimeSeriesViewport.size.width, tree_top);
                  left = mTimeSeriesViewport.origin.x;
              }
              break;
          case SettingsSignaturePage::TreeCladesTimeseriesMaps:
              if (mTimeSeries) {
                  mTimeSeriesViewport.origin = Location(left - aTree.settings().signature_page.clades_antigenic_maps_space * canvas_width - mTimeSeriesViewport.size.width, tree_top);
                  left = mTimeSeriesViewport.origin.x;
                  if (mClades) {
                      mCladesViewport.origin = Location(left - aTree.settings().signature_page.time_series_clades_space * canvas_width - mCladesViewport.size.width, tree_top);
                      left = mCladesViewport.origin.x;
                  }
                  else {
                      left -= aTree.settings().signature_page.time_series_clades_space * canvas_width;
                  }
              }
              break;
        }

        mTreeViewport.set(tree_origin, Size(left - mPageArea.origin.x - aTree.settings().signature_page.tree_time_series_space * canvas_width, tree_height));

          // Note legend must be drawn after tree, because ColoringByPos needs to collect data to be drawn in the legend
        if (mLegend) {
            const auto legend_size = mLegend->size(aSurface, aTree.settings().legend);
            const auto& legend_settings = aTree.settings().legend;
            mLegendViewport.set({mPageArea.origin.x + legend_settings.offset_x, mPageArea.bottom() - legend_size.height + legend_settings.offset_y}, legend_size);
        }

        mDrawTree->calculate_viewports(aTree, mTreeViewport);
          // Calculate individual map viewports again after mDrawTree->calculate_viewports
        mAntigenicMapsViewport.origin.y = mTreeViewport.origin.y;
        if (mAntigenicMaps) {
            mAntigenicMaps->calculate_viewports(aTree, aChart, mAntigenicMapsViewport, mPageArea, *mDrawTree, aTree.settings().draw_tree.hz_line_sections, aTree.settings().antigenic_maps);
        }
    }

} // SignaturePage::calculate_viewports

// ----------------------------------------------------------------------

void SignaturePage::draw(const Tree& aTree, Surface& aSurface, const Chart* aChart)
{
    if (mTitle) {
        mTitle->draw(aSurface, mTitleViewport);
    }

    if (mDrawTree) {
        mDrawTree->draw(aTree, aSurface, mTreeViewport, aTree.settings().draw_tree);

          // Note legend must be drawn after tree, because ColoringByPos needs to collect data to be drawn in the legend
        if (mLegend) {
            mLegend->draw(aSurface, mLegendViewport, aTree.settings().legend);
        }

        if (mTimeSeries) {
            mTimeSeries->draw(aSurface, mTimeSeriesViewport, aTree, *mDrawTree, aTree.settings().time_series);
        }
        if (mClades) {
            mClades->draw(aSurface, aTree, mCladesViewport, mTimeSeriesViewport, *mDrawTree, aTree.settings().clades, aTree.settings().signature_page.layout);
        }
        if (mDrawHzLines) {
            mDrawHzLines->draw(aSurface, mTimeSeriesViewport, mAntigenicMapsViewport, *mDrawTree, mAntigenicMaps, aTree.settings().antigenic_maps, aTree.settings().draw_tree.hz_line_sections, aTree.settings().signature_page.layout);
        }
        if (mAntigenicMaps) {
            mAntigenicMaps->draw(aSurface, mAntigenicMapsViewport, aChart, aTree.settings().draw_tree.hz_line_sections, aTree.settings().antigenic_maps);
        }
    }

} // SignaturePage::draw

// ----------------------------------------------------------------------

Title::Title(const SettingsTitle& aSettingsTitle)
    : mTitle(Location(aSettingsTitle.offset_x, aSettingsTitle.offset_y), aSettingsTitle.text, aSettingsTitle.color, aSettingsTitle.size, aSettingsTitle.style, aSettingsTitle.rotation)
{
} // Title::Title

// ----------------------------------------------------------------------

void Title::draw(Surface& aSurface, const Viewport& aViewport)
{
    mTitle.draw(aSurface, aViewport);

} // Title::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
