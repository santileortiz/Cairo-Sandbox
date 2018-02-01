//gcc -DNEGATIVE_ARCS -mssse3 -O3 -g -Wall main.c -o main -lcairo -lX11-xcb -lX11 -lxcb -lxcb-sync -lm
/*
 * Copiright (C) 2018 Santiago León O.
 */

#ifdef NEGATIVE_ARCS
bool draw (app_graphics_t *gr, bool redraw)
{
    static bool draw_once = true;
    if (draw_once || redraw) {
        draw_once = false;
        cairo_t *cr = gr->cr;
        cairo_set_source_rgb (cr, 1,1,1);
        cairo_paint (cr);

        cairo_arc (cr, 100, 100, 100, 0, 2*M_PI);
        //cairo_arc_negative (cr, 100, 100, 50, 2*M_PI, 0);
        cairo_arc_negative (cr, 175, 100, 100, 2*M_PI, 0);
        cairo_set_source_rgb (cr, 0,0,0);
        cairo_fill (cr);

        cairo_surface_flush (cairo_get_target(gr->cr));
        return true;
    } else {
        return false;
    }
}

#endif // NEGATIVE_ARCS
