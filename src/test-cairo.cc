#include <iostream>
#include <string>

#include "cairo.hh"

// ----------------------------------------------------------------------

int main(int /*argc*/, const char */*argv*/[])
{
    int exit_code = 0;
    try {
        // Surface surface("/tmp/tc.pdf", 1000, 500);
        //   //surface.line(Location(100, 100), Location(200, 100), 0xFFA500, 2);
        // TextStyle style;
        // double tsize = 10;
        // Size sz = surface.text_size("Jopa", tsize, style);
        // surface.text(Location(200, 100 + sz.height / 2), "Jopa", 0x00FF00, tsize, style);

        auto surface = cairo_pdf_surface_create("/tmp/tc.pdf", 1000, 500);
        auto mContext = cairo_create(surface);
        cairo_surface_destroy(surface);

    cairo_set_source_rgb(mContext, 0, 1, 0);
    // cairo_move_to(mContext, 0, 0);
    // cairo_line_to(mContext, 1, 1);
    // cairo_stroke(mContext);
    cairo_move_to(mContext, 100, 200);
    cairo_set_font_size(mContext, 30);
    cairo_show_text(mContext, "JOPA");

    cairo_line_to(mContext, 200, 300);
    cairo_stroke(mContext);
    cairo_move_to(mContext, 200, 300);
    cairo_show_text(mContext, "APOJ");

    cairo_destroy(mContext);

    }
    catch (std::exception& err) {
        std::cerr << err.what() << std::endl;
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
