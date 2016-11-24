#include "time-series.hh"
#include "settings.hh"
#include "tree.hh"
#include "draw-tree.hh"
#include "stream.hh"

// ----------------------------------------------------------------------

TimeSeries& TimeSeries::prepare(const Tree& aTree, const SettingsTimeSeries& aSettings)
{
    mBegin = aSettings.begin;
    mEnd = aSettings.end;
    auto const sequences_per_month = aTree.sequences_per_month();
    if (mBegin.empty()) {
        for (Date d = sequences_per_month.crbegin()->first; sequences_per_month.find(d) != sequences_per_month.end(); d.decrement_month()) {
            mBegin = d;
        }
    }
    if (mEnd.empty()) {
        for (auto ms = sequences_per_month.crbegin(); mEnd.empty() && ms != sequences_per_month.crend(); ++ms) {
            if (ms->second)
                mEnd = ms->first;
        }
    }

    // auto const mmd = aTree.min_max_date();
    // std::cout << "dates in source tree: " << mmd.first << " .. " << mmd.second << "  months: " << (months_between_dates(mmd) + 1) << std::endl;
    // if (mBegin.empty())
    //     mBegin.assign_and_remove_day(mmd.first);
    // if (mEnd.empty())
    //     mEnd.assign_and_remove_day(mmd.second);

    mNumberOfMonths = static_cast<size_t>(months_between_dates(mBegin, mEnd)) + 1;
    if (mNumberOfMonths > aSettings.max_number_of_months) {
        mBegin.assign_and_subtract_months(mEnd, aSettings.max_number_of_months - 1);
        assert(months_between_dates(mBegin, mEnd) == static_cast<int>(aSettings.max_number_of_months - 1));
        mNumberOfMonths = aSettings.max_number_of_months;
    }
    std::cout << "dates to show: " << mBegin << " .. " << mEnd << "  months: " << mNumberOfMonths << std::endl;

    return *this;

} // TimeSeries::prepare

// ----------------------------------------------------------------------

Size TimeSeries::size(Surface& /*aSurface*/, const SettingsTimeSeries& aSettings) const
{
    return {mNumberOfMonths * aSettings.month_width, 1};

} // TimeSeries::size

// ----------------------------------------------------------------------

void TimeSeries::draw(Surface& aSurface, const Viewport& aViewport, const Tree& aTree, const DrawTree& aDrawTree, const SettingsTimeSeries& aSettings) const
{
    if (mNumberOfMonths > 1) {
        draw_labels(aSurface, aViewport, aSettings);
        draw_month_separators(aSurface, aViewport, aSettings);
        draw_dashes(aSurface, aViewport, aTree, aDrawTree, aSettings);
    }

} // TimeSeries::draw

// ----------------------------------------------------------------------

double TimeSeries::label_height(Surface& aSurface, const SettingsTimeSeries& aSettings) const
{
    const double label_font_size = aSettings.month_width * aSettings.month_label_scale;
    double x_bearing;
    const auto big_label_size = aSurface.text_size("May 99", label_font_size, aSettings.month_label_style, &x_bearing);
    return big_label_size.width + x_bearing;

} // TimeSeries::label_height

// ----------------------------------------------------------------------

void TimeSeries::draw_labels(Surface& aSurface, const Viewport& aViewport, const SettingsTimeSeries& aSettings) const
{
    const double label_font_size = aSettings.month_width * aSettings.month_label_scale;
    const auto month_max_width = aSurface.text_size("May ", label_font_size, aSettings.month_label_style).width;
    double x_bearing;
    const auto big_label_size = aSurface.text_size("May 99", label_font_size, aSettings.month_label_style, &x_bearing);
    const auto text_up = (aSettings.month_width - big_label_size.height) * 0.5;
    const double month_year_to_timeseries_gap = aSettings.month_year_to_timeseries_gap * aSurface.canvas_size().height;

    draw_labels_at_side(aSurface, aViewport.origin + Size(text_up, - big_label_size.width - x_bearing - month_year_to_timeseries_gap), label_font_size, month_max_width, aSettings);
    draw_labels_at_side(aSurface, aViewport.origin + Size(text_up, aViewport.size.height + x_bearing + month_year_to_timeseries_gap), label_font_size, month_max_width, aSettings);

} // TimeSeries::draw_labels

// ----------------------------------------------------------------------

void TimeSeries::draw_labels_at_side(Surface& aSurface, const Location& aOrigin, double label_font_size, double month_max_width, const SettingsTimeSeries& aSettings) const
{
    Date current_month = mBegin;
    for (size_t month_no = 0; month_no < mNumberOfMonths; ++month_no, current_month.increment_month()) {
        const auto left = aOrigin.x + month_no * aSettings.month_width;
        aSurface.text({left, aOrigin.y}, current_month.month_3(), 0, label_font_size, aSettings.month_label_style, M_PI_2);
        aSurface.text({left, aOrigin.y + month_max_width}, current_month.year_2(), 0, label_font_size, aSettings.month_label_style, M_PI_2);
    }

} // TimeSeries::draw_labels_at_side

// ----------------------------------------------------------------------

void TimeSeries::draw_month_separators(Surface& aSurface, const Viewport& aViewport, const SettingsTimeSeries& aSettings) const
{
    const auto bottom = aViewport.bottom();
    for (size_t month_no = 0; month_no <= mNumberOfMonths; ++month_no) {
        const auto left = aViewport.origin.x + month_no * aSettings.month_width;
        aSurface.line({left, aViewport.origin.y}, {left, bottom}, aSettings.month_separator_color, aSettings.month_separator_width);
    }

} // TimeSeries::draw_month_separators

// ----------------------------------------------------------------------

void TimeSeries::draw_dashes(Surface& aSurface, const Viewport& aViewport, const Tree& aTree, const DrawTree& aDrawTree, const SettingsTimeSeries& aSettings) const
{
    const auto base_x = aViewport.origin.x + aSettings.month_width * (1.0 - aSettings.dash_width) / 2;
    const auto vertical_step = aDrawTree.vertical_step();
    const auto& coloring = aDrawTree.coloring();

    auto draw_dash = [&](const Node& aNode) {
        if (!aNode.hidden) {
            const int month_no = aNode.months_from(mBegin);
            if (month_no >= 0) {
                const Location a(base_x + aSettings.month_width * month_no, aViewport.origin.y + vertical_step * aNode.line_no);
                aSurface.line(a, {a.x + aSettings.month_width * aSettings.dash_width, a.y}, coloring.color(aNode), aSettings.dash_line_width, CAIRO_LINE_CAP_ROUND);
            }
        }
    };
    iterate_leaf(aTree, draw_dash);

} // TimeSeries::draw_dashes

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
