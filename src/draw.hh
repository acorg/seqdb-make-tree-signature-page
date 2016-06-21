#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <string>
#include <stdexcept>
#include <cmath>

#include "cairo.hh"
#include "json-struct.hh"

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif

// ----------------------------------------------------------------------

class Surface;

// ----------------------------------------------------------------------

template <typename T> inline auto operator << (std::ostream& out, const T& a) -> decltype(std::declval<T>().to_string(), out)
{
    return out << a.to_string();
}

// ----------------------------------------------------------------------

class Location
{
 public:
    inline Location() : x(0), y(0) {}
    inline Location(double aX, double aY) : x(aX), y(aY) {}
    double x, y;

    inline Location& operator -= (const Location& a) { x -= a.x; y -= a.y; return *this; }
    inline std::string to_string() const { return "Location(" + std::to_string(x) + ", " + std::to_string(y) + ")"; }

    inline void min(const Location& a) { x = std::min(x, a.x); y = std::min(y, a.y); }
    inline void max(const Location& a) { x = std::max(x, a.x); y = std::max(y, a.y); }
    static inline Location center_of(const Location& a, const Location& b) { return {(a.x + b.x) / 2.0, (a.y + b.y) / 2.0}; }

    inline std::vector<double> to_vector() const { return {x, y}; }
    inline void from_vector(const std::vector<double>& source) { x = source[0]; y = source[1]; }

}; // class Location

// ----------------------------------------------------------------------

class Size
{
 public:
    inline Size() : width(0), height(0) {}
    inline Size(double aWidth, double aHeight) : width(aWidth), height(aHeight) {}
    inline Size(const Location& a, const Location& b) : width(std::abs(a.x - b.x)), height(std::abs(a.y - b.y)) {}
    inline void set(double aWidth, double aHeight) { width = aWidth; height = aHeight; }
    double width, height;

    inline std::string to_string() const { return "Size(" + std::to_string(width) + ", " + std::to_string(height) + ")"; }

}; // class Size

inline Location operator + (const Location& a, const Size& s)
{
    return {a.x + s.width, a.y + s.height};
}

inline Location operator + (const Location& a, const Location& b)
{
    return {a.x + b.x, a.y + b.y};
}

inline Location operator - (const Location& a, const Size& s)
{
    return {a.x - s.width, a.y - s.height};
}

inline Size operator - (const Location& a, const Location& b)
{
    return {a.x - b.x, a.y - b.y};
}

inline Size operator - (const Size& a, const Location& b)
{
    return {a.width - b.x, a.height - b.y};
}

inline Size operator - (const Size& a, const Size& b)
{
    return {a.width - b.width, a.height - b.height};
}

inline Size operator * (const Size& a, double v)
{
    return {a.width * v, a.height * v};
}

// ----------------------------------------------------------------------

class Viewport
{
 public:
    inline Viewport() : origin(0, 0), size(0, 0) {}
    inline Viewport(const Location& a, const Size& s) : origin(a), size(s) {}
    inline Viewport(const Location& a, const Location& b) : origin(a), size(b - a) {}

    inline void set(const Location& a, const Size& s) { origin = a; size = s; }
    inline void set(const Location& a, const Location& b) { origin = a; size = b - a; }
    inline void zoom(double scale) { const Size new_size = size * scale; origin = center() - new_size * 0.5; size = new_size; }

      // make viewport a square by extending the smaller side from center
    inline void square()
        {
            if (size.width < size.height) {
                origin.x -= (size.height - size.width) / 2;
                size.width = size.height;
            }
            else {
                origin.y -= (size.width - size.height) / 2;
                size.height = size.width;
            }
        }

      // zoom out viewport to make width a whole number)
    inline void whole_width() { zoom(std::ceil(size.width) / size.width); }

    inline double right() const { return origin.x + size.width; }
    inline double bottom() const { return origin.y + size.height; }
    inline Location top_right() const { return origin + Size(size.width, 0); }
    inline Location bottom_right() const { return origin + size; }
    inline Location bottom_left() const { return origin + Size(0, size.height); }
    inline Location center() const { return origin + size * 0.5; }

    Location origin;
    Size size;

    inline std::string to_string() const { return "Viewport(" + origin.to_string() + ", " + size.to_string() + ")"; }

}; // class Viewport

// ----------------------------------------------------------------------

class Color
{
 public:
    static constexpr uint32_t _not_set = 0xFFFFFFFF;

    inline Color() : mColor(0xFF00FF) {}
    template <typename Uint, typename std::enable_if<std::is_integral<Uint>::value>::type* = nullptr> constexpr inline Color(Uint aColor) : mColor(static_cast<uint32_t>(aColor)) {}
    inline Color(std::string aColor) { from_string(aColor); }
      // inline Color(const Color&) = default;
      // inline Color& operator=(const Color& aSrc) = default;
    template <typename Uint, typename std::enable_if<std::is_integral<Uint>::value>::type* = nullptr> inline Color& operator=(Uint aColor) { mColor = static_cast<uint32_t>(aColor); return *this; }
    inline Color& operator=(std::string aColor) { from_string(aColor); return *this; }

    inline bool operator == (const Color& aColor) const { return mColor == aColor.mColor; }
    inline bool operator != (const Color& aColor) const { return ! operator==(aColor); }

    inline double alpha() const { return double(0xFF - ((mColor >> 24) & 0xFF)) / 255.0; }
    inline double red() const { return double((mColor >> 16) & 0xFF) / 255.0; }
    inline double green() const { return double((mColor >> 8) & 0xFF) / 255.0; }
    inline double blue() const { return double(mColor & 0xFF) / 255.0; }

    inline size_t alphaI() const { return static_cast<size_t>((mColor >> 24) & 0xFF); }
    inline void alphaI(uint32_t v) { mColor = (mColor & 0xFFFFFF) | ((v & 0xFF) << 24); }
    inline size_t rgbI() const { return static_cast<size_t>(mColor & 0xFFFFFF); }

    // inline void set_transparency(double aTransparency) { mColor = (mColor & 0x00FFFFFF) | ((int(aTransparency * 255.0) & 0xFF) << 24); }

    inline std::string to_string() const
        {
            std::stringstream s;
            s << '#' << std::hex << std::setw(6) << std::setfill('0') << mColor;
            return s.str();
        }

    inline void from_string(std::string aColor)
        {
            if (aColor[0] != '#')
                throw std::invalid_argument("cannot read Color from " + aColor + ": first symbol must be # got " + std::string(1, aColor[0]));
            if (aColor.size() != 7 && aColor.size() != 9)
                throw std::invalid_argument("cannot read Color from " + aColor + ": invalid string length, must be 7 or 9 but got " + std::to_string(aColor.size()));
            try {
                mColor = static_cast<uint32_t>(std::stoul(std::string(aColor, 1), nullptr, 16));
            }
            catch (std::exception& err) {
                throw std::invalid_argument("cannot read Color from " + aColor + ": " + err.what());
            }
        }

      // to avoid serialing not set colors
    inline operator bool() const { return mColor != _not_set; }

 private:
    uint32_t mColor; // 4 bytes, most->least significant: transparency-red-green-blue, 0x00FF0000 - opaque red, 0xFF000000 - fully transparent

}; // class Color

constexpr const Color BLACK = 0;
constexpr const Color WHITE = 0xFFFFFF;
constexpr const Color GREY = 0xA0A0A0;
constexpr const Color LIGHT_GREY = 0xE0E0E0;
constexpr const Color TRANSPARENT = 0xFF000000;
constexpr const Color COLOR_NOT_SET = Color::_not_set;

// ----------------------------------------------------------------------

class FontStyle
{
 public:
    static constexpr const char* default_family = "sans-serif";

    inline FontStyle() : cairo_family(default_family) {}
    inline FontStyle(std::string aFamily) { from_string(aFamily); }
    inline FontStyle& operator=(std::string aFamily) { from_string(aFamily); return *this; }

    inline void from_string(std::string aFamily)
        {
            if (aFamily == "default")
                cairo_family = default_family;
            else
                cairo_family = aFamily;
        }

    inline std::string to_string() const
        {
            return cairo_family;
        }

    inline operator const char*() const { return cairo_family.c_str(); }

      // to avoid saving to json, e.g. for settings.SettingsAATransition.TransitionData
    inline operator bool() const { return cairo_family != default_family; }

 private:
    std::string cairo_family;
};

// ----------------------------------------------------------------------

class TextStyle
{
 public:
    inline TextStyle() : mSlant(CAIRO_FONT_SLANT_NORMAL), mWeight(CAIRO_FONT_WEIGHT_NORMAL) {}
    inline TextStyle(std::string aFontFamily) : mFontStyle(aFontFamily), mSlant(CAIRO_FONT_SLANT_NORMAL), mWeight(CAIRO_FONT_WEIGHT_NORMAL) {}

    inline const FontStyle& font_style() const { return mFontStyle; }
    inline cairo_font_slant_t slant() const { return mSlant; }
    inline cairo_font_weight_t weight() const { return mWeight; }

      // to avoid saving to json, e.g. for settings.SettingsAATransition.TransitionData
    inline operator bool() const { return mSlant != CAIRO_FONT_SLANT_NORMAL || mWeight != CAIRO_FONT_WEIGHT_NORMAL || !mFontStyle; }

 private:
    FontStyle mFontStyle;
    cairo_font_slant_t mSlant;
    cairo_font_weight_t mWeight;

    static std::string slant_to_string(const cairo_font_slant_t* a);
    static std::string weight_to_string(const cairo_font_weight_t* a);
    static void slant_from_string(cairo_font_slant_t* a, std::string source);
    static void weight_from_string(cairo_font_weight_t* a, std::string source);

    friend inline auto json_fields(TextStyle& a)
        {
            return std::make_tuple(
                "?", json::comment("font: default monospace; slant: normal italic oblique; weight: normal bold"),
                "font", json::field(&a.mFontStyle, &FontStyle::to_string, &FontStyle::from_string),
                "slant", json::field(&a.mSlant, &TextStyle::slant_to_string, &TextStyle::slant_from_string),
                "weight", json::field(&a.mWeight, &TextStyle::weight_to_string, &TextStyle::weight_from_string)
                                   );
        }

}; // class TextStyle

// ----------------------------------------------------------------------

class Text
{
 public:
    inline Text() = default;
    inline Text(const Location& aOrigin, std::string aText, Color aColor, double aSize, const TextStyle& aStyle, double aRotation = 0)
        : mOrigin(aOrigin), mText(aText), mColor(aColor), mSize(aSize), mStyle(aStyle), mRotation(aRotation) {}

    inline void draw(Surface& aSurface, const Viewport& aViewport) const;
    inline Size size(Surface& aSurface) const;
    inline Location right_bottom(Surface& aSurface, const Viewport& aViewport) const;

 private:
    Location mOrigin;
    std::string mText;
    Color mColor;
    double mSize;
    TextStyle mStyle;
    double mRotation;

}; // class Text

// ----------------------------------------------------------------------

class SurfaceError : public std::runtime_error
{
 public: using std::runtime_error::runtime_error;
};

// ----------------------------------------------------------------------

class Surface
{
 public:
    inline Surface() : mContext(nullptr) {}
    Surface(std::string aFilename, double aWidth, double aHeight);

    inline ~Surface() { if (mContext != nullptr) cairo_destroy(mContext); }

    void setup(std::string aFilename, const Size& aCanvasSize);

    const Size& canvas_size() const { return mCanvasSize; }

    void line(const Location& a, const Location& b, Color aColor, double aWidth, cairo_line_cap_t aLineCap = CAIRO_LINE_CAP_BUTT);
    void rectangle(const Location& a, const Size& s, Color aColor, double aWidth, cairo_line_cap_t aLineCap = CAIRO_LINE_CAP_BUTT);
    inline void rectangle(const Viewport& v, Color aColor, double aWidth, cairo_line_cap_t aLineCap = CAIRO_LINE_CAP_BUTT) { rectangle(v.origin, v.size, aColor, aWidth, aLineCap); }
    void rectangle_filled(const Location& a, const Size& s, Color aOutlineColor, double aWidth, Color aFillColor, cairo_line_cap_t aLineCap = CAIRO_LINE_CAP_BUTT);
    inline void rectangle_filled(const Viewport& v, Color aOutlineColor, double aWidth, Color aFillColor, cairo_line_cap_t aLineCap = CAIRO_LINE_CAP_BUTT) { rectangle_filled(v.origin, v.size, aOutlineColor, aWidth, aFillColor, aLineCap); }
    void square_filled(const Location& aCenter, double aSide, double aAspect, double aAngle, Color aOutlineColor, double aOutlineWidth, Color aFillColor, cairo_line_cap_t aLineCap = CAIRO_LINE_CAP_BUTT);
    void triangle_filled(const Location& aCenter, double aSide, double aAspect, double aAngle, Color aOutlineColor, double aOutlineWidth, Color aFillColor, cairo_line_cap_t aLineCap = CAIRO_LINE_CAP_BUTT);
    void circle_filled(const Location& aCenter, double aDiameter, double aAspect, double aAngle, Color aOutlineColor, double aOutlineWidth, Color aFillColor);
    void double_arrow(const Location& a, const Location& b, Color aColor, double aLineWidth, double aArrowWidth);
    void text(const Location& a, std::string aText, Color aColor, double aSize, const TextStyle& aTextStyle = TextStyle(), double aRotation = 0);
    inline void text(const Text& aText, const Viewport& aViewport) { aText.draw(*this, aViewport); }
    void grid(const Viewport& aViewport, double aStep, Color aLineColor, double aLineWidth);

      // Sets the cairo clip region to viewport, viewport top left
      // corner will have coordinates (0, 0), viewport top right
      // corner will have coordinates (0, aWidthScale), drawing
      // outside the viewport will not be possible. Returns the ratio
      // of the new scal to the old scale, i.e. to draw a line of some
      // width in the new region you need to draw the line of
      // width*ratio.
    double set_clip_region(const Viewport& aViewport, double aWidthScale);

    Size text_size(std::string aText, double aSize, const TextStyle& aTextStyle, double* x_bearing);
    inline Size text_size(std::string aText, double aSize, const TextStyle& aTextStyle) { return text_size(aText, aSize, aTextStyle, nullptr); } // for pybind11 to avoid exposing double* to python

    class PushContext
    {
     public:
        inline PushContext(Surface& aSurface) : mContext(aSurface.mContext) { cairo_save(mContext); }
        inline ~PushContext() { cairo_restore(mContext); }

     private:
        cairo_t* mContext;
    };

    // void test();

    inline void new_path() { cairo_new_path(mContext); }
    inline void destroy_path(cairo_path_t* aPath) { cairo_path_destroy(aPath); }
    inline void close_path() { cairo_close_path(mContext); }
    inline void move_to(double x, double y) { cairo_move_to(mContext, x, y); }
    inline void line_to(double x, double y) { cairo_line_to(mContext, x, y); }
    inline cairo_path_t* copy_path() { return cairo_copy_path(mContext); }

    inline cairo_t* context() { return mContext; }

 private:
    cairo_t* mContext;
    Size mCanvasSize;

    Location arrow_head(const Location& a, double angle, double sign, Color aColor, double aArrowWidth);
    void prepare_for_text(double aSize, const TextStyle& aTextStyle);

    inline void set_source_rgba(Color aColor) const
        {
            cairo_set_source_rgba(mContext, aColor.red(), aColor.green(), aColor.blue(), aColor.alpha());
        }

    friend class PushContext;

}; // class Surface

// ----------------------------------------------------------------------

inline void Text::draw(Surface& aSurface, const Viewport& aViewport) const
{
    if (!mText.empty()) {
        aSurface.text(mOrigin + aViewport.origin, mText, mColor, mSize, mStyle, mRotation);
    }

} // Text::draw

// ----------------------------------------------------------------------

inline Location Text::right_bottom(Surface& aSurface, const Viewport& aViewport) const
{
    return mOrigin + aViewport.origin + Size(size(aSurface).width, 0);

} // Text::right_bottom

// ----------------------------------------------------------------------

inline Size Text::size(Surface& aSurface) const
{
    return mText.empty() ? Size(0, 0) : aSurface.text_size(mText, mSize, mStyle);

} // Text::size

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
