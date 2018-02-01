//gcc -g -pg `pkg-config --cflags gtk+-3.0` -o main_gtk main_gtk.c `pkg-config --libs gtk+-3.0`
#include <cairo.h>
#include <gtk/gtk.h>

static gboolean draw_callback (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{      
    PangoLayout *text_layout = pango_cairo_create_layout (cr);
    PangoFontDescription *font_desc = pango_font_description_from_string ("Open Sans 9");
    pango_layout_set_font_description (text_layout, font_desc);
    pango_font_description_free (font_desc);

    cairo_font_options_t *font_options = cairo_font_options_create ();
    cairo_surface_get_font_options (cairo_get_target (cr), font_options);

    cairo_set_source_rgb (cr,1,1,1);
    cairo_paint (cr);
    pango_layout_set_text (text_layout, "Documents", -1);
        int w,h;
        pango_layout_get_pixel_size (text_layout, &w, &h);
        printf ("Layout h: %d, w: %d\n", h, w);
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_move_to (cr, 10.4, 50);
    pango_cairo_show_layout (cr, text_layout);

    return FALSE;
}

int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *darea;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(window), "GTK window");

    darea = gtk_drawing_area_new();
    gtk_widget_set_size_request (darea, 700, 700);
    gtk_container_add(GTK_CONTAINER(window), darea);

    g_signal_connect(G_OBJECT(darea), "draw", 
                     G_CALLBACK(draw_callback), NULL); 
    g_signal_connect(window, "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
