//gcc -DPANGO_TEST -O3 -g -Wall main.c -o main -lcairo -lX11-xcb -lX11 -lxcb -lxcb-sync -lm -lpango-1.0 -lpangocairo-1.0 -I/usr/include/pango-1.0 -I/usr/include/cairo -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
/*
 * Copiright (C) 2018 Santiago León O.
 */

#ifdef PANGO_TEST
#include <pango/pangocairo.h>

void print_font_options (cairo_font_options_t *font_options)
{
    printf ("Antialias: ");
    switch (cairo_font_options_get_antialias (font_options)) {
        case CAIRO_ANTIALIAS_DEFAULT:
            printf ("CAIRO_ANTIALIAS_DEFAULT\n");
            break;
        case CAIRO_ANTIALIAS_NONE:
            printf ("CAIRO_ANTIALIAS_NONE\n");
            break;
        case CAIRO_ANTIALIAS_GRAY:
            printf ("CAIRO_ANTIALIAS_GRAY\n");
            break;
        case CAIRO_ANTIALIAS_SUBPIXEL:
            printf ("CAIRO_ANTIALIAS_SUBPIXEL\n");
            break;
        case CAIRO_ANTIALIAS_FAST:
            printf ("CAIRO_ANTIALIAS_FAST\n");
            break;
        case CAIRO_ANTIALIAS_GOOD:
            printf ("CAIRO_ANTIALIAS_GOOD\n");
            break;
        case CAIRO_ANTIALIAS_BEST:
            printf ("CAIRO_ANTIALIAS_BEST\n");
            break;
    }
    printf ("Subpixel Order: ");
    switch (cairo_font_options_get_subpixel_order (font_options)) {
        case CAIRO_SUBPIXEL_ORDER_DEFAULT:
            printf ("CAIRO_SUBPIXEL_ORDER_DEFAULT\n");
            break;
        case CAIRO_SUBPIXEL_ORDER_RGB:
            printf ("CAIRO_SUBPIXEL_ORDER_RGB\n");
            break;
        case CAIRO_SUBPIXEL_ORDER_BGR:
            printf ("CAIRO_SUBPIXEL_ORDER_BGR\n");
            break;
        case CAIRO_SUBPIXEL_ORDER_VRGB:
            printf ("CAIRO_SUBPIXEL_ORDER_VRGB\n");
            break;
        case CAIRO_SUBPIXEL_ORDER_VBGR:
            printf ("CAIRO_SUBPIXEL_ORDER_VBGR\n");
            break;
    }
    printf ("Hint Style: ");
    switch (cairo_font_options_get_hint_style (font_options)) {
        case CAIRO_HINT_STYLE_DEFAULT:
            printf ("CAIRO_HINT_STYLE_DEFAULT\n");
            break;
        case CAIRO_HINT_STYLE_NONE:
            printf ("CAIRO_HINT_STYLE_NONE\n");
            break;
        case CAIRO_HINT_STYLE_SLIGHT:
            printf ("CAIRO_HINT_STYLE_SLIGHT\n");
            break;
        case CAIRO_HINT_STYLE_MEDIUM:
            printf ("CAIRO_HINT_STYLE_MEDIUM\n");
            break;
        case CAIRO_HINT_STYLE_FULL:
            printf ("CAIRO_HINT_STYLE_FULL\n");
            break;
    }
    printf ("Hint Metrics: ");
    switch (cairo_font_options_get_hint_metrics (font_options)) {
        case CAIRO_HINT_METRICS_DEFAULT:
            printf ("CAIRO_HINT_METRICS_DEFAULT\n");
            break;
        case CAIRO_HINT_METRICS_OFF:
            printf ("CAIRO_HINT_METRICS_OFF\n");
            break;
        case CAIRO_HINT_METRICS_ON:
            printf ("CAIRO_HINT_METRICS_ON\n");
            break;
    }
}

cairo_font_options_t* create_nice_font_options ()
{
    cairo_font_options_t *font_options = cairo_font_options_create ();
    cairo_font_options_set_antialias (font_options, CAIRO_ANTIALIAS_NONE);
    cairo_font_options_set_subpixel_order (font_options, CAIRO_SUBPIXEL_ORDER_RGB);
    cairo_font_options_set_hint_style (font_options, CAIRO_HINT_STYLE_SLIGHT);
    cairo_font_options_set_hint_metrics (font_options, CAIRO_HINT_METRICS_ON);
    return font_options;
}

void text_with_info (cairo_t *cr, PangoLayout *text_layout, double x, double y, char *str)
{
    pango_layout_set_text (text_layout, str, -1);
    PangoRectangle ink, logical;
    pango_layout_get_pixel_extents (text_layout, &ink, &logical);
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

    cairo_move_to (cr, x, y);
    pango_cairo_show_layout (cr, text_layout);

    cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.3);
    cairo_rectangle (cr, x+ink.x, y+ink.y, ink.width, ink.height);
    cairo_fill (cr);

    cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 0.3);
    cairo_rectangle (cr, x+logical.x, y+logical.y, logical.width, logical.height);
    cairo_fill (cr);

    cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
    cairo_rectangle (cr, x, y, 1, 1);
    cairo_fill (cr);

    int baseline = pango_layout_get_baseline (text_layout)/PANGO_SCALE;
    cairo_move_to (cr, x+logical.x, y-0.5+baseline);
    cairo_line_to (cr, x+logical.x+logical.width, y-0.5+baseline);
    cairo_set_line_width (cr, 1);
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_stroke (cr);
}

void cairo_clear (cairo_t *cr)
{
    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);
}

bool draw (app_graphics_t *gr, bool redraw)
{
    static bool draw_once = true;
    if (draw_once || redraw) {
        draw_once = false;
        cairo_t *cr = gr->cr;
        PangoLayout *text_layout = pango_cairo_create_layout (cr);
        PangoFontMap *fontmap = pango_cairo_font_map_get_default ();
        PangoContext *context = pango_font_map_create_context (fontmap);
        cairo_font_options_t *font_options = create_nice_font_options ();
        pango_cairo_context_set_font_options (context, font_options);

        //PangoFontFamily **ff;
        //int num_ff;
        //pango_context_list_families (context, &ff, &num_ff);

        //printf ("num_families: %d\n", num_ff);
        //int i;
        //for (i=0; i<num_ff; i++) {
        //    const char *str = pango_font_family_get_name (ff[i]); 
        //    printf ("%s\n", str);
        //}

        PangoFontDescription *font_desc = pango_font_description_from_string ("Open Sans 32");
        pango_layout_set_font_description (text_layout, font_desc);
        pango_font_description_free (font_desc);

        cairo_clear (cr);
        cairo_set_source_rgba (cr,1,1,1,0.6);
        cairo_paint (cr);
        text_with_info (cr, text_layout, 10, 50, "KorplgI");
        text_with_info (cr, text_layout, 160, 50, "nnpq");

        //cairo_font_options_t *resulting_fo = pango_cairo_context_get_font_options (context);
        //print_font_options (resulting_fo);

        cairo_surface_flush (cairo_get_target(gr->cr));
        return true;
    } else {
        return false;
    }
}
#endif // PANGO_TEST

