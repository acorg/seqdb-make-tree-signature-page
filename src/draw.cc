#include "draw.hh"
#include "float.hh"

// ----------------------------------------------------------------------

Surface::Surface(std::string aFilename, double aWidth, double aHeight)
    : mContext(nullptr)
{
    setup(aFilename, Size(aWidth, aHeight));
}

// ----------------------------------------------------------------------

void Surface::setup(std::string aFilename, const Size& aCanvasSize)
{
    auto surface = cairo_pdf_surface_create(aFilename.c_str(), aCanvasSize.width, aCanvasSize.height);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
        throw SurfaceError("cannot create pdf surface");
    mContext = cairo_create(surface);
    cairo_surface_destroy(surface);
    if (cairo_status(mContext) != CAIRO_STATUS_SUCCESS)
        throw SurfaceError("cannot create mContext");

    mCanvasSize = aCanvasSize;

} // Surface::setup

// ----------------------------------------------------------------------

void Surface::line(const Location& a, const Location& b, Color aColor, double aWidth, cairo_line_cap_t aLineCap)
{
    PushContext pc(*this);
    cairo_set_line_width(mContext, aWidth);
    set_source_rgba(aColor);
    cairo_set_line_cap(mContext, aLineCap);
    cairo_move_to(mContext, a.x, a.y);
    cairo_line_to(mContext, b.x, b.y);
    cairo_stroke(mContext);

} // Surface::line

// ----------------------------------------------------------------------

void Surface::path_outline(std::vector<Location>::const_iterator first, std::vector<Location>::const_iterator last, Color aOutlineColor, double aOutlineWidth, bool aClose, cairo_line_cap_t aLineCap)
{
    PushContext pc(*this);
    cairo_new_path(mContext);
    cairo_set_line_cap(mContext, aLineCap);
    cairo_set_line_join(mContext, CAIRO_LINE_JOIN_MITER);
    cairo_set_line_width(mContext, aOutlineWidth);
    set_source_rgba(aOutlineColor);
    cairo_move_to(mContext, first->x, first->y);
    while (++first != last) {
        cairo_line_to(mContext, first->x, first->y);
    }
    if (aClose) {
        cairo_close_path(mContext);
    }
    cairo_stroke(mContext);

} // Surface::path_outline

// ----------------------------------------------------------------------

void Surface::path_fill(std::vector<Location>::const_iterator first, std::vector<Location>::const_iterator last, Color aFillColor)
{
    PushContext pc(*this);
    cairo_new_path(mContext);
    set_source_rgba(aFillColor);
    cairo_move_to(mContext, first->x, first->y);
    while (++first != last) {
        cairo_line_to(mContext, first->x, first->y);
    }
    cairo_close_path(mContext);
    cairo_fill(mContext);

} // Surface::path_fill

// ----------------------------------------------------------------------

void Surface::grid(const Viewport& aViewport, double aStep, Color aLineColor, double aLineWidth)
{
    PushContext pc(*this);
    cairo_set_line_width(mContext, aLineWidth);
    set_source_rgba(aLineColor);
    cairo_set_line_cap(mContext, CAIRO_LINE_CAP_BUTT);
    for (double x = aViewport.origin.x; x < aViewport.right(); x += aStep) {
        cairo_move_to(mContext, x, aViewport.origin.y);
        cairo_line_to(mContext, x, aViewport.bottom());
    }
    for (double y = aViewport.origin.y; y < aViewport.bottom(); y += aStep) {
        cairo_move_to(mContext, aViewport.origin.x, y);
        cairo_line_to(mContext, aViewport.right(), y);
    }
    cairo_stroke(mContext);

} // Surface::grid

// ----------------------------------------------------------------------

void Surface::rectangle(const Location& a, const Size& s, Color aColor, double aWidth, cairo_line_cap_t aLineCap)
{
    PushContext pc(*this);
    cairo_set_line_width(mContext, aWidth);
    cairo_set_line_cap(mContext, aLineCap);
    cairo_rectangle(mContext, a.x, a.y, s.width, s.height);
    set_source_rgba(aColor);
    cairo_stroke(mContext);

} // Surface::rectangle

// ----------------------------------------------------------------------

void Surface::rectangle_filled(const Location& a, const Size& s, Color aOutlineColor, double aWidth, Color aFillColor, cairo_line_cap_t aLineCap)
{
    PushContext pc(*this);
    cairo_set_line_width(mContext, aWidth);
    cairo_set_line_cap(mContext, aLineCap);
    cairo_rectangle(mContext, a.x, a.y, s.width, s.height);
    set_source_rgba(aFillColor);
    cairo_fill_preserve(mContext);
    set_source_rgba(aOutlineColor);
    cairo_stroke(mContext);

} // Surface::rectangle_filled

// ----------------------------------------------------------------------

void Surface::square_filled(const Location& aCenter, double aSide, double aAspect, double aAngle, Color aOutlineColor, double aOutlineWidth, Color aFillColor, cairo_line_cap_t aLineCap)
{
    PushContext pc(*this);
    cairo_set_line_width(mContext, aOutlineWidth);
    cairo_set_line_cap(mContext, aLineCap);
    cairo_translate(mContext, aCenter.x, aCenter.y);
    cairo_rotate(mContext, aAngle);
    cairo_rectangle(mContext, - aSide / 2 * aAspect, - aSide / 2, aSide * aAspect, aSide);
    set_source_rgba(aFillColor);
    cairo_fill_preserve(mContext);
    set_source_rgba(aOutlineColor);
    cairo_stroke(mContext);

} // Surface::square_filled

// ----------------------------------------------------------------------

void Surface::triangle_filled(const Location& aCenter, double aSide, double aAspect, double aAngle, Color aOutlineColor, double aOutlineWidth, Color aFillColor, cairo_line_cap_t aLineCap)
{
    const auto cos_pi_6 = std::cos(M_PI / 6.0);
    const auto radius = aSide * cos_pi_6;
    PushContext pc(*this);
    cairo_set_line_width(mContext, aOutlineWidth);
    cairo_set_line_cap(mContext, aLineCap);
    cairo_translate(mContext, aCenter.x, aCenter.y);
    cairo_rotate(mContext, aAngle);
    cairo_move_to(mContext, 0, - radius);
    cairo_line_to(mContext, - radius * cos_pi_6 * aAspect, radius * 0.5);
    cairo_line_to(mContext,   radius * cos_pi_6 * aAspect, radius * 0.5);
    cairo_close_path(mContext);
    set_source_rgba(aFillColor);
    cairo_fill_preserve(mContext);
    set_source_rgba(aOutlineColor);
    cairo_stroke(mContext);

} // Surface::triangle_filled

// ----------------------------------------------------------------------

void Surface::circle_filled(const Location& aCenter, double aDiameter, double aAspect, double aAngle, Color aOutlineColor, double aOutlineWidth, Color aFillColor)
{
    PushContext pc(*this);
    cairo_set_line_width(mContext, aOutlineWidth);
    cairo_translate(mContext, aCenter.x, aCenter.y);
    cairo_rotate(mContext, aAngle);
    cairo_scale(mContext, aAspect, 1.0);
    cairo_arc(mContext, 0, 0, aDiameter / 2, 0.0, 2.0 * M_PI);
    set_source_rgba(aFillColor);
    cairo_fill_preserve(mContext);
    set_source_rgba(aOutlineColor);
    cairo_stroke(mContext);

} // Surface::circle_filled

// ----------------------------------------------------------------------

void Surface::double_arrow(const Location& a, const Location& b, Color aColor, double aLineWidth, double aArrowWidth)
{
    const bool x_eq = float_equal(b.x, a.x);
    const double sign2 = x_eq ? (a.y < b.y ? 1.0 : -1.0) : (b.x < a.x ? 1.0 : -1.0);
    const double angle = x_eq ? -M_PI_2 : std::atan((b.y - a.y) / (b.x - a.x));
    auto const la = arrow_head(a, angle, - sign2, aColor, aArrowWidth);
    auto const lb = arrow_head(b, angle,   sign2, aColor, aArrowWidth);
    line(la, lb, aColor, aLineWidth);

} // Surface::double_arrow

// ----------------------------------------------------------------------

Location Surface::arrow_head(const Location& a, double angle, double sign, Color aColor, double aArrowWidth)
{
    constexpr double ARROW_WIDTH_TO_LENGTH_RATIO = 2.0;

    const double arrow_length = aArrowWidth * ARROW_WIDTH_TO_LENGTH_RATIO;
    const Location b(a.x + sign * arrow_length * std::cos(angle), a.y + sign * arrow_length * std::sin(angle));
    const Location c(b.x + sign * aArrowWidth * std::cos(angle + M_PI_2) * 0.5, b.y + sign * aArrowWidth * std::sin(angle + M_PI_2) * 0.5);
    const Location d(b.x + sign * aArrowWidth * std::cos(angle - M_PI_2) * 0.5, b.y + sign * aArrowWidth * std::sin(angle - M_PI_2) * 0.5);

    PushContext pc(*this);
    set_source_rgba(aColor);
    cairo_set_line_join(mContext, CAIRO_LINE_JOIN_MITER);
    cairo_move_to(mContext, a.x, a.y);
    cairo_line_to(mContext, c.x, c.y);
    cairo_line_to(mContext, d.x, d.y);
    cairo_close_path(mContext);
    cairo_fill(mContext);

    return b;

} // Surface::arrow_head

// ----------------------------------------------------------------------

void Surface::text(const Location& a, std::string aText, Color aColor, double aSize, const TextStyle& aTextStyle, double aRotation)
{
    PushContext pc(*this);
    prepare_for_text(aSize, aTextStyle);
    cairo_move_to(mContext, a.x, a.y);
    cairo_rotate(mContext, aRotation);
    set_source_rgba(aColor);
    cairo_show_text(mContext, aText.c_str());

} // Surface::text

// ----------------------------------------------------------------------

void Surface::prepare_for_text(double aSize, const TextStyle& aTextStyle)
{
    cairo_select_font_face(mContext, aTextStyle.font_style(), aTextStyle.slant(), aTextStyle.weight());
    cairo_set_font_size(mContext, aSize);

} // Surface::prepare_for_text

// ----------------------------------------------------------------------

Size Surface::text_size(std::string aText, double aSize, const TextStyle& aTextStyle, double* x_bearing)
{
    PushContext pc(*this);
    prepare_for_text(aSize, aTextStyle);
    cairo_text_extents_t text_extents;
    cairo_text_extents(mContext, aText.c_str(), &text_extents);
      // std::cout << "text_extents y_bearing:" << text_extents.y_bearing << " height:" << text_extents.height << " y_advance:" << text_extents.y_advance << std::endl;
    if (x_bearing != nullptr)
        *x_bearing = text_extents.x_bearing;
    return {text_extents.x_advance, - text_extents.y_bearing};

} // Surface::text_size

// ----------------------------------------------------------------------

double Surface::set_clip_region(const Viewport& aViewport, double aWidthScale)
{
    cairo_reset_clip(mContext);
    const auto center = aViewport.center();
    cairo_translate(mContext, center.x, center.y);
    const double scale = aViewport.size.width / aWidthScale;
    cairo_scale(mContext, scale, scale);
    cairo_new_path(mContext);
    const double y_d = aViewport.size.height / scale;
    cairo_rectangle(mContext, - aWidthScale / 2, - y_d / 2, aWidthScale, y_d);
    cairo_clip(mContext);
    return 1.0 / scale;

} // Surface::set_clip_region

// ----------------------------------------------------------------------

double Surface::set_clip_region(const Viewport& aViewport, const Viewport& aTargetViewport)
{
    cairo_reset_clip(mContext);
    const auto center = aViewport.center();
    cairo_translate(mContext, center.x, center.y);
    cairo_new_path(mContext);
    cairo_rectangle(mContext, - aViewport.size.width / 2, - aViewport.size.height / 2, aViewport.size.width, aViewport.size.height);
    cairo_clip(mContext);

    const double scale = aViewport.size.width / aTargetViewport.size.width;
    cairo_scale(mContext, scale, scale);
    const auto target_center = aTargetViewport.center();
    cairo_translate(mContext, -target_center.x, -target_center.y);

    return 1.0 / scale;

} // Surface::set_clip_region

// ----------------------------------------------------------------------

double Surface::draw_path_scale(cairo_path_t* aPath, const Viewport& aViewport)
{
    PushContext pc(*this);
      // compute scale to fit path into the viewport
    cairo_append_path(mContext, aPath);
    double x1, y1, x2, y2;
    cairo_path_extents(mContext, &x1, &y1, &x2, &y2);
    cairo_new_path(mContext);
    return aViewport.size.width / (x2 - x1);

} // Surface::draw_path_scale

// ----------------------------------------------------------------------

void Surface::draw_path(cairo_path_t* aPath, const Viewport& aViewport, double aScale, Color aColor, double aWidth, cairo_line_cap_t aLineCap)
{
    PushContext pc(*this);
    cairo_translate(mContext, aViewport.origin.x, aViewport.origin.y);
    cairo_scale(mContext, aScale, aScale);
    cairo_set_line_width(mContext, aWidth);
    set_source_rgba(aColor);
    cairo_set_line_cap(mContext, aLineCap);
    cairo_append_path(mContext, aPath);
    cairo_stroke(mContext);

} // Surface::draw_path

// ----------------------------------------------------------------------

void Surface::draw_path_fill(cairo_path_t* aPath, const Viewport& aViewport, double aScale, Color aFillColor)
{
    PushContext pc(*this);
    cairo_translate(mContext, aViewport.origin.x, aViewport.origin.y);
    cairo_scale(mContext, aScale, aScale);
    set_source_rgba(aFillColor);
    cairo_append_path(mContext, aPath);
    cairo_fill(mContext);

} // Surface::draw_path_fill

// ----------------------------------------------------------------------

std::string TextStyle::slant_to_string(const cairo_font_slant_t* a)
{
    switch (*a) {
      case CAIRO_FONT_SLANT_NORMAL:
          return "normal";
      case CAIRO_FONT_SLANT_ITALIC:
          return "italic";
      case CAIRO_FONT_SLANT_OBLIQUE:
          return "oblique";
    }
    return "normal";            // to avoid gcc complaining about having no return

} // TextStyle::slant_to_string

// ----------------------------------------------------------------------

std::string TextStyle::weight_to_string(const cairo_font_weight_t* a)
{
    switch (*a) {
      case CAIRO_FONT_WEIGHT_NORMAL:
          return "normal";
      case CAIRO_FONT_WEIGHT_BOLD:
          return "bold";
    }
    return "normal";            // to avoid gcc complaining about having no return

} // TextStyle::weight_to_string

// ----------------------------------------------------------------------

void TextStyle::slant_from_string(cairo_font_slant_t* a, std::string source)
{
    if (source == "italic")
        *a = CAIRO_FONT_SLANT_ITALIC;
    else if (source == "oblique")
        *a = CAIRO_FONT_SLANT_OBLIQUE;
    else if (source == "normal")
        *a = CAIRO_FONT_SLANT_NORMAL;
    else
        throw std::invalid_argument("cannot infer font slant from " + source);

} // TextStyle::slant_from_string

// ----------------------------------------------------------------------

void TextStyle::weight_from_string(cairo_font_weight_t* a, std::string source)
{
    if (source == "bold")
        *a = CAIRO_FONT_WEIGHT_BOLD;
    else if (source == "normal")
        *a = CAIRO_FONT_WEIGHT_NORMAL;
    else
        throw std::invalid_argument("cannot infer font weight from " + source);

} // TextStyle::weight_from_string

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
