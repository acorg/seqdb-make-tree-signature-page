#include "draw.hh"
#include "float.hh"

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
    cairo_save(mContext);
    cairo_set_line_width(mContext, aWidth);
    set_source_rgba(aColor);
    cairo_set_line_cap(mContext, aLineCap);
    cairo_move_to(mContext, a.x, a.y);
    cairo_line_to(mContext, b.x, b.y);
    cairo_stroke(mContext);
    cairo_restore(mContext);

} // Surface::line

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

    cairo_save(mContext);
    set_source_rgba(aColor);
    cairo_set_line_join(mContext, CAIRO_LINE_JOIN_MITER);
    cairo_move_to(mContext, a.x, a.y);
    cairo_line_to(mContext, c.x, c.y);
    cairo_line_to(mContext, d.x, d.y);
    cairo_close_path(mContext);
    cairo_fill(mContext);
    cairo_restore(mContext);

    return b;

} // Surface::arrow_head

// ----------------------------------------------------------------------

void Surface::text(const Location& a, std::string aText, Color aColor, double aSize, const TextStyle& aTextStyle, double aRotation)
{
    cairo_save(mContext);
    prepare_for_text(aSize, aTextStyle);
    cairo_move_to(mContext, a.x, a.y);
    cairo_rotate(mContext, aRotation);
    set_source_rgba(aColor);
    cairo_show_text(mContext, aText.c_str());
    cairo_restore(mContext);

} // Surface::text

// ----------------------------------------------------------------------

void Surface::prepare_for_text(double aSize, const TextStyle& aTextStyle)
{
    switch (aTextStyle.font_style()) {
      case FontStyle::Monospace:
          cairo_select_font_face(mContext, "monospace", aTextStyle.slant(), aTextStyle.weight());
          break;
      case FontStyle::Default:
          cairo_select_font_face(mContext, "sans-serif", aTextStyle.slant(), aTextStyle.weight());
          break;
    }
    cairo_set_font_size(mContext, aSize);

} // Surface::prepare_for_text

// ----------------------------------------------------------------------

Size Surface::text_size(std::string aText, double aSize, const TextStyle& aTextStyle, double* x_bearing)
{
    cairo_save(mContext);
    prepare_for_text(aSize, aTextStyle);
    cairo_text_extents_t text_extents;
    cairo_text_extents(mContext, aText.c_str(), &text_extents);
      // std::cout << "text_extents y_bearing:" << text_extents.y_bearing << " height:" << text_extents.height << " y_advance:" << text_extents.y_advance << std::endl;
    cairo_restore(mContext);
    if (x_bearing != nullptr)
        *x_bearing = text_extents.x_bearing;
    return {text_extents.x_advance, - text_extents.y_bearing};

} // Surface::text_size

// ----------------------------------------------------------------------

jsonw::IfPrependComma TextStyle::json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const
{
    comma = json_begin(target, comma, '{', indent, prefix);
    switch (font_style()) {
      case FontStyle::Default:
          comma = jsonw::json(target, comma, "font", "default", indent, prefix);
          break;
      case FontStyle::Monospace:
          comma = jsonw::json(target, comma, "font", "monospace", indent, prefix);
          break;
    }
    switch (slant()) {
      case CAIRO_FONT_SLANT_NORMAL:
          comma = jsonw::json(target, comma, "slant", "normal", indent, prefix);
          break;
      case CAIRO_FONT_SLANT_ITALIC:
          comma = jsonw::json(target, comma, "slant", "italic", indent, prefix);
          break;
      case CAIRO_FONT_SLANT_OBLIQUE:
          comma = jsonw::json(target, comma, "slant", "oblique", indent, prefix);
          break;
    }
    switch (weight()) {
      case CAIRO_FONT_WEIGHT_NORMAL:
          comma = jsonw::json(target, comma, "weight", "normal", indent, prefix);
          break;
      case CAIRO_FONT_WEIGHT_BOLD:
          comma = jsonw::json(target, comma, "weight", "bold", indent, prefix);
          break;
    }
    comma = jsonw::json(target, comma, "?", "font: default monospace; slant: normal italic oblique; weight: normal bold", indent, prefix);
    return jsonw::json_end(target, '}', indent, prefix);

} // TextStyle::json

// ----------------------------------------------------------------------

FontStyle TextStyle::font_style_from_string(std::string source)
{
    if (source == "monospace")
        return FontStyle::Monospace;
    else if (source == "default")
        return FontStyle::Default;
    else
        throw std::invalid_argument("cannot infer font style from " + source);

} // TextStyle::font_style_from_string

// ----------------------------------------------------------------------

cairo_font_slant_t TextStyle::slant_from_string(std::string source)
{
    if (source == "italic")
        return CAIRO_FONT_SLANT_ITALIC;
    else if (source == "oblique")
        return CAIRO_FONT_SLANT_OBLIQUE;
    else if (source == "normal")
        return CAIRO_FONT_SLANT_NORMAL;
    else
        throw std::invalid_argument("cannot infer font slant from " + source);

} // TextStyle::slant_from_string

// ----------------------------------------------------------------------

cairo_font_weight_t TextStyle::weight_from_string(std::string source)
{
    if (source == "bold")
        return CAIRO_FONT_WEIGHT_BOLD;
    else if (source == "normal")
        return CAIRO_FONT_WEIGHT_NORMAL;
    else
        throw std::invalid_argument("cannot infer font weight from " + source);

} // TextStyle::weight_from_string

// ----------------------------------------------------------------------
