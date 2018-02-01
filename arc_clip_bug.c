/*
 * Copiright (C) 2018 Santiago León O.
 */

// CAIRO BUG!!!
// Clipping with some path that has an arc disables RGB antialiasing of fonts.
bool draw (app_graphics_t *gr, bool redraw) {
    static bool draw_once = true;
    if (draw_once || redraw) {
        draw_once = false;
        cairo_t *cr = gr->cr;
        PangoLayout *text_layout = pango_cairo_create_layout (cr);
        PangoFontMap *fontmap = pango_cairo_font_map_get_default ();
        PangoContext *context = pango_font_map_create_context (fontmap);
        cairo_font_options_t *font_options = create_nice_font_options ();
        pango_cairo_context_set_font_options (context, font_options);
        PangoFontDescription *font_desc = pango_font_description_from_string ("Open Sans 9");
        pango_layout_set_font_description (text_layout, font_desc);
        pango_font_description_free (font_desc);

        cairo_clear (cr);
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

