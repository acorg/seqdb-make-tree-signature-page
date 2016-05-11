#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <string>
#include <stdexcept>
// #include <functional>

#include "cairo.hh"
#include "json-read.hh"
#include "json-write.hh"
// #include "date.hh"

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

    std::string to_string() const { return "Location(" + std::to_string(x) + ", " + std::to_string(y) + ")"; }

}; // class Location

// ----------------------------------------------------------------------

class Size
{
 public:
    inline Size() : width(0), height(0) {}
    inline Size(double aWidth, double aHeight) : width(aWidth), height(aHeight) {}
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
    inline Viewport() {}
    inline Viewport(const Location& a, const Size& s) : origin(a), size(s) {}
    inline Viewport(const Location& a, const Location& b) : origin(a), size(b - a) {}
    inline double right() const { return origin.x + size.width; }
    inline double bottom() const { return origin.y + size.height; }
    inline Location top_right() const { return origin + Size(size.width, 0); }
    inline Location bottom_right() const { return origin + size; }
    inline Location bottom_left() const { return origin + Size(0, size.height); }

    Location origin;
    Size size;

    inline std::string to_string() const { return "Viewport(" + origin.to_string() + ", " + size.to_string() + ")"; }

}; // class Viewport

// ----------------------------------------------------------------------

class Color
{
 public:
    inline Color() : mColor(0xFF00FF) {}
    template <typename Uint, typename std::enable_if<std::is_integral<Uint>::value>::type* = nullptr> inline Color(Uint aColor) : mColor(static_cast<uint32_t>(aColor)) {}
    inline Color(std::string aColor) { from_string(aColor); }
      // inline Color(const Color&) = default;
      // inline Color& operator=(const Color& aSrc) = default;
    template <typename Uint, typename std::enable_if<std::is_integral<Uint>::value>::type* = nullptr> inline Color& operator=(Uint aColor) { mColor = static_cast<uint32_t>(aColor); return *this; }
    inline Color& operator=(std::string aColor) { from_string(aColor); return *this; }

    // inline bool operator == (const Color& aColor) const { return mColor == aColor.mColor; }
    // inline bool operator != (const Color& aColor) const { return ! operator==(aColor); }

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
            if ((aColor.size() == 7 || aColor.size() == 9) && aColor[0] == '#') {
                try {
                    mColor = static_cast<uint32_t>(std::stoul(std::string(aColor, 1), nullptr, 16));
                }
                catch (std::exception& err) {
                    throw std::invalid_argument("cannot read Color from " + aColor + ": " + err.what());
                }
            }
            else
                throw std::invalid_argument("cannot read Color from " + aColor);
        }

 private:
    uint32_t mColor; // 4 bytes, most->least significant: transparency-red-green-blue, 0x00FF0000 - opaque red, 0xFF000000 - fully transparent

}; // class Color

// ----------------------------------------------------------------------

enum class FontStyle { Default, Monospace };

class TextStyle
{
 private:
    class json_parser_t AXE_RULE
        {
          public:
            inline json_parser_t(TextStyle& aTextStyle) : mTextStyle(aTextStyle) {}

            template<class Iterator> inline axe::result<Iterator> operator()(Iterator i1, Iterator i2) const
            {
                auto r_font = jsonr::object_enum_value("font", mTextStyle.mFontStyle, &TextStyle::font_style_from_string);
                auto r_slant = jsonr::object_enum_value("slant", mTextStyle.mSlant, &TextStyle::slant_from_string);
                auto r_weight = jsonr::object_enum_value("weight", mTextStyle.mWeight, &TextStyle::weight_from_string);
                auto r_comment = jsonr::object_string_ignore_value("?");
                return jsonr::object(r_font | r_slant | r_weight | r_comment)(i1, i2);
            }

          private:
            TextStyle& mTextStyle;
        };

 public:
    inline TextStyle() : mFontStyle(FontStyle::Default), mSlant(CAIRO_FONT_SLANT_NORMAL), mWeight(CAIRO_FONT_WEIGHT_NORMAL) {}
    inline TextStyle(FontStyle aFontStyle) : mFontStyle(aFontStyle), mSlant(CAIRO_FONT_SLANT_NORMAL), mWeight(CAIRO_FONT_WEIGHT_NORMAL) {}

    inline FontStyle font_style() const { return mFontStyle; }
    inline cairo_font_slant_t slant() const { return mSlant; }
    inline cairo_font_weight_t weight() const { return mWeight; }

    jsonw::IfPrependComma json(std::string& target, jsonw::IfPrependComma comma, size_t indent, size_t prefix) const;
    inline auto json_parser() { return json_parser_t(*this); }

 private:
    FontStyle mFontStyle;
    cairo_font_slant_t mSlant;
    cairo_font_weight_t mWeight;

    static FontStyle font_style_from_string(std::string source);
    static cairo_font_slant_t slant_from_string(std::string source);
    static cairo_font_weight_t weight_from_string(std::string source);

    friend class json_parser_t;

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
    inline Surface(std::string aFilename, double aWidth, double aHeight) : mContext(nullptr) { setup(aFilename, {aWidth, aHeight}); }
      // inline Surface(std::string aFilename, const Size& aCanvasSize) : mContext(nullptr) { setup(aFilename, aCanvasSize); }

    inline ~Surface() { if (mContext != nullptr) cairo_destroy(mContext); }

    void setup(std::string aFilename, const Size& aCanvasSize);

    const Size& canvas_size() const { return mCanvasSize; }

    void line(const Location& a, const Location& b, Color aColor, double aWidth, cairo_line_cap_t aLineCap = CAIRO_LINE_CAP_BUTT);
    void double_arrow(const Location& a, const Location& b, Color aColor, double aLineWidth, double aArrowWidth);
    void text(const Location& a, std::string aText, Color aColor, double aSize, const TextStyle& aTextStyle = TextStyle(), double aRotation = 0);
    inline void text(const Text& aText, const Viewport& aViewport) { aText.draw(*this, aViewport); }

    Size text_size(std::string aText, double aSize, const TextStyle& aTextStyle, double* x_bearing = nullptr);

    // void test();

 private:
    cairo_t* mContext;
    Size mCanvasSize;

    Location arrow_head(const Location& a, double angle, double sign, Color aColor, double aArrowWidth);
    void prepare_for_text(double aSize, const TextStyle& aTextStyle);

    inline void set_source_rgba(Color aColor) const
        {
            cairo_set_source_rgba(mContext, aColor.red(), aColor.green(), aColor.blue(), aColor.alpha());
        }

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
