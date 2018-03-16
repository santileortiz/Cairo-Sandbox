//gcc -DCAIRO_ANTIALIAS_BUG -O3 -g -Wall main.c -o main -lcairo -lX11-xcb -lX11 -lxcb -lxcb-sync -lm -lpango-1.0 -lpangocairo-1.0 -I/usr/include/pango-1.0 -I/usr/include/cairo -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
/*
 * Copiright (C) 2018 Santiago León O.
 */

#ifdef CAIRO_ANTIALIAS_BUG
#include <pango/pangocairo.h>

// CAIRO BUG!!!
// Clipping with some path that has an arc disables RGB antialiasing of fonts.
bool draw (app_graphics_t *gr, bool redraw) {
    static bool draw_once = true;
    if (draw_once || redraw) {
        draw_once = false;
        cairo_t *cr = gr->cr;
        PangoLayout *text_layout = pango_cairo_create_layout (cr);
        PangoFontDescription *font_desc = pango_font_description_from_string ("Open Sans 9");
        pango_layout_set_font_description (text_layout, font_desc);
        pango_font_description_free (font_desc);

        cairo_set_source_rgb (cr,0,0,0);
        cairo_paint (cr);

        cairo_save (cr);
        cairo_arc (cr, 40, 20, 30, 0, 2*M_PI);
        cairo_clip (cr);

        cairo_set_source_rgb (cr,1,1,1);
        cairo_paint (cr);

        pango_layout_set_text (text_layout, "Arrange", -1);
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_move_to (cr, 15, 15);
        pango_cairo_show_layout (cr, text_layout);
        cairo_restore (cr);

        cairo_save (cr);
        cairo_rectangle (cr, 10, 55, 60, 30);
        cairo_clip (cr);

        cairo_set_source_rgb (cr,1,1,1);
        cairo_paint (cr);

        pango_layout_set_text (text_layout, "Arrange", -1);
        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_move_to (cr, 15, 60);
        pango_cairo_show_layout (cr, text_layout);
        cairo_restore (cr);

        cairo_surface_flush (cairo_get_target(gr->cr));
        return true;
    } else {
        return false;
    }
}

#endif // CAIRO_ANTIALIAS_BUG
