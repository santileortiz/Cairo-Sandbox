#include <X11/Xlib-xcb.h>
#include <xcb/sync.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cairo/cairo-xlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "slo_timers.h"
#include "common.h"

#define WINDOW_HEIGHT 700
#define WINDOW_WIDTH 700

typedef struct {
    cairo_t *cr;
    uint16_t width;
    uint16_t height;
} app_graphics_t;

int end_execution = 0;

#include "arc_clip_bug.c"
#include "pango_test.c"
#include "negative_arcs.c"
#include "gaussian_blur.c"

xcb_atom_t get_x11_atom (xcb_connection_t *c, const char *value)
{
    xcb_atom_t res;
    xcb_generic_error_t *err = NULL;
    xcb_intern_atom_cookie_t ck = xcb_intern_atom (c, 0, strlen(value), value);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply (c, ck, &err);
    if (err != NULL) {
        printf ("Error while requesting atom.\n");
    }
    res = reply->atom;
    free(reply);
    return res;
}

xcb_visualtype_t* get_visual_struct_from_visualid (xcb_connection_t *c, xcb_screen_t *screen, xcb_visualid_t id)
{
    xcb_visualtype_t  *visual_type = NULL;    /* the returned visual type */

    /* you init the connection and screen_nbr */

    xcb_depth_iterator_t depth_iter;
    if (screen) {
        depth_iter = xcb_screen_allowed_depths_iterator (screen);
        for (; depth_iter.rem; xcb_depth_next (&depth_iter)) {
            xcb_visualtype_iterator_t visual_iter;

            visual_iter = xcb_depth_visuals_iterator (depth_iter.data);
            for (; visual_iter.rem; xcb_visualtype_next (&visual_iter)) {
                if (id == visual_iter.data->visual_id) {
                    visual_type = visual_iter.data;
                    //break;
                }
            }
        }
    }
    return visual_type;
}

xcb_visualtype_t* get_visual_max_depth (xcb_connection_t *c, xcb_screen_t *screen, uint8_t *found_depth)
{
    xcb_visualtype_t  *visual_type = NULL;    /* the returned visual type */

    /* you init the connection and screen_nbr */

    *found_depth = 0;
    xcb_depth_iterator_t depth_iter;
    if (screen) {
        depth_iter = xcb_screen_allowed_depths_iterator (screen);
        for (; depth_iter.rem; xcb_depth_next (&depth_iter)) {
            xcb_visualtype_iterator_t visual_iter;
            visual_iter = xcb_depth_visuals_iterator (depth_iter.data);
            if (visual_iter.rem) {
                if (*found_depth < depth_iter.data->depth) {
                    *found_depth = depth_iter.data->depth;
                    visual_type = visual_iter.data;
                }
            }
        }
    }
    return visual_type;
}

Visual* Visual_from_visualid (Display *dpy, xcb_visualid_t visualid)
{
    XVisualInfo templ = {0};
    templ.visualid = visualid;
    int n;
    XVisualInfo *found = XGetVisualInfo (dpy, VisualIDMask, &templ, &n);
    return found->visual;
}

void increment_sync_counter (xcb_sync_int64_t *counter)
{
    counter->lo++;
    if (counter->lo == 0) {
        counter->hi++;
    }
}

int main (void)
{
    // Setup clocks
    setup_clocks ();

    //////////////////
    // X11 setup

    Display *dpy = XOpenDisplay (NULL);
    if (!dpy) {
        printf ("Could not open display\n");
        return -1;
    }

    xcb_connection_t *connection = XGetXCBConnection (dpy); //just in case we need XCB
    if (!connection) {
        printf ("Could not get XCB connection from Xlib Display\n");
        return -1;
    }
    XSetEventQueueOwner (dpy, XCBOwnsEventQueue);

    /* Get the first screen */
    // TODO: xcb_connect() returns default screen on 2nd argument, maybe
    // use that instead of assuming it's 0.
    // TODO: What happens if there is more than 1 screen?, probably will
    // have to iterate with xcb_setup_roots_iterator(), and xcb_screen_next ().
    xcb_screen_t *screen = xcb_setup_roots_iterator (xcb_get_setup (connection)).data;

    uint8_t depth;
    xcb_visualtype_t *visual = get_visual_max_depth (connection, screen, &depth);
    xcb_colormap_t colormap = xcb_generate_id (connection);
    xcb_create_colormap (connection, XCB_COLORMAP_ALLOC_NONE, colormap, screen->root, visual->visual_id); 

    // Create a window
    uint32_t event_mask = XCB_EVENT_MASK_EXPOSURE|XCB_EVENT_MASK_KEY_PRESS|
                             XCB_EVENT_MASK_STRUCTURE_NOTIFY|XCB_EVENT_MASK_BUTTON_PRESS|
                             XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_POINTER_MOTION;
    uint32_t mask = XCB_CW_EVENT_MASK|
                    // NOTE: These are required to use depth of 32 when the root window
                    // has a different depth.
                    XCB_CW_BORDER_PIXEL|XCB_CW_COLORMAP; 
    uint32_t values[] = {// , // XCB_CW_BACK_PIXMAP
                         // , // XCB_CW_BACK_PIXEL
                         // , // XCB_CW_BORDER_PIXMAP
                         0  , // XCB_CW_BORDER_PIXEL
                         // , // XCB_CW_BIT_GRAVITY
                         // , // XCB_CW_WIN_GRAVITY
                         // , // XCB_CW_BACKING_STORE
                         // , // XCB_CW_BACKING_PLANES
                         // , // XCB_CW_BACKING_PIXEL     
                         // , // XCB_CW_OVERRIDE_REDIRECT
                         // , // XCB_CW_SAVE_UNDER
                         event_mask, //XCB_CW_EVENT_MASK
                         // , // XCB_CW_DONT_PROPAGATE
                         colormap // XCB_CW_COLORMAP
                         //  // XCB_CW_CURSOR
                         };

    xcb_drawable_t window = xcb_generate_id (connection);
    xcb_create_window (connection,                    /* connection          */
                       depth,                         /* depth               */
                       window,                        /* window Id           */
                       screen->root,                  /* parent window       */
                       0, 0,                          /* x, y                */
                       WINDOW_WIDTH, WINDOW_HEIGHT,   /* width, height       */
                       0,                             /* border_width        */
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                       visual->visual_id,             /* visual              */
                       mask, values);                 /* masks */

    // Set window title
    char *title = "Cairo Sandbox";
    xcb_change_property (connection,
            XCB_PROP_MODE_REPLACE,
            window,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            strlen (title),
            title);

    // Set WM_PROTOCOLS property
    xcb_atom_t wm_protocols = get_x11_atom (connection, "WM_PROTOCOLS");
    xcb_atom_t delete_window_atom = get_x11_atom (connection, "WM_DELETE_WINDOW");
    xcb_atom_t net_wm_sync = get_x11_atom (connection, "_NET_WM_SYNC_REQUEST");
    xcb_atom_t atoms[] = {delete_window_atom, net_wm_sync};
    xcb_change_property (connection,
            XCB_PROP_MODE_REPLACE,
            window,
            wm_protocols,
            XCB_ATOM_ATOM,
            32,
            ARRAY_SIZE(atoms),
            atoms);

    xcb_change_property (connection,
            XCB_PROP_MODE_REPLACE,
            window,
            get_x11_atom (connection, "_NET_WM_OPAQUE_REGION"),
            XCB_ATOM_CARDINAL,
            32,
            0,
            NULL);

    // NOTE: These atoms are used to recognize the ClientMessage type
    xcb_atom_t net_wm_frame_drawn = get_x11_atom (connection, "_NET_WM_FRAME_DRAWN");
    xcb_atom_t net_wm_frame_timings = get_x11_atom (connection, "_NET_WM_FRAME_TIMINGS");

    xcb_gcontext_t gc = xcb_generate_id (connection);
    xcb_pixmap_t backbuffer = xcb_generate_id (connection);
    xcb_create_gc (connection, gc, window, 0, NULL);
    xcb_create_pixmap (connection,
            depth,     /* depth of the screen */
            backbuffer,  /* id of the pixmap */
            window,
            WINDOW_WIDTH,     /* pixel width of the window */
            WINDOW_HEIGHT);  /* pixel height of the window */

    // Set up counters for _NET_WM_SYNC_REQUEST protocol on extended mode
    xcb_sync_int64_t counter_val = {0, 0}; //{HI, LO}
    xcb_sync_counter_t counters[2];
    counters[0] = xcb_generate_id (connection);
    xcb_sync_create_counter (connection, counters[0], counter_val);

    counters[1] = xcb_generate_id (connection);
    xcb_sync_create_counter (connection, counters[1], counter_val);
    
    xcb_change_property (connection,
            XCB_PROP_MODE_REPLACE,
            window,
            get_x11_atom (connection, "_NET_WM_SYNC_REQUEST_COUNTER"),
            XCB_ATOM_CARDINAL,
            32,
            ARRAY_SIZE(counters),
            counters);

    xcb_map_window (connection, window);
    xcb_flush (connection);

    // Create a cairo surface on window
    Visual *Xlib_visual = Visual_from_visualid (dpy, visual->visual_id);
    cairo_surface_t *surface = cairo_xlib_surface_create (dpy, backbuffer, Xlib_visual, WINDOW_WIDTH, WINDOW_HEIGHT);
    cairo_t *cr = cairo_create (surface);

    //cairo_select_font_face (cr, "Open Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    //cairo_set_font_size (cr, 12.5);

    // ////////////////
    // Main event loop
    xcb_generic_event_t *event;
    app_graphics_t graphics;
    graphics.cr = cr;
    graphics.width = WINDOW_WIDTH;
    graphics.height = WINDOW_HEIGHT;
    uint16_t pixmap_width = WINDOW_WIDTH;
    uint16_t pixmap_height = WINDOW_HEIGHT;
    bool make_pixmap_bigger = false;

    float frame_rate = 60;
    float target_frame_length_ms = 1000/(frame_rate);
    struct timespec start_ticks;
    struct timespec end_ticks;

    clock_gettime(CLOCK_MONOTONIC, &start_ticks);
    while (!end_execution) {
        bool redraw = false;
        bool force_blit = false;
        while ((event = xcb_poll_for_event (connection))) {
            // NOTE: The most significant bit of event->response_type is set if
            // the event was generated from a SendEvent request, here we don't
            // care about the source of the event.
            switch (event->response_type & ~0x80) {
                case XCB_CONFIGURE_NOTIFY: {
                    uint16_t new_width = ((xcb_configure_notify_event_t*)event)->width;
                    uint16_t new_height = ((xcb_configure_notify_event_t*)event)->height;
                    if (new_width > pixmap_width || new_height > pixmap_height) {
                        make_pixmap_bigger = true;
                    }
                    graphics.width = new_width;
                    graphics.height = new_height;
                    } break;
                case XCB_KEY_PRESS:
                    if (((xcb_key_press_event_t*)event)->detail == 24) { // KEY_Q
                        end_execution = true;
                    }
                    break;
                case XCB_EXPOSE:
                    // We should tell which areas need exposing
                    force_blit = true;
                    break;
                case XCB_CLIENT_MESSAGE: {
                    bool handled = false;
                    xcb_client_message_event_t *client_message = ((xcb_client_message_event_t*)event);
                     
                    // WM_DELETE_WINDOW protocol
                    if (client_message->type == wm_protocols) {
                        if (client_message->data.data32[0] == delete_window_atom) {
                            end_execution = true;
                        }
                        handled = true;
                    }

                    // _NET_WM_SYNC_REQUEST protocol using the extended mode
                    if (client_message->type == wm_protocols && client_message->data.data32[0] == net_wm_sync) {
                        counter_val.lo = client_message->data.data32[2];
                        counter_val.hi = client_message->data.data32[3];
                        if (counter_val.lo % 2 != 0) {
                            increment_sync_counter (&counter_val);
                        }
                        handled = true;
                    } else if (client_message->type == net_wm_frame_drawn) {
                        handled = true;
                    } else if (client_message->type == net_wm_frame_timings) {
                        handled = true;
                    }

                    if (!handled) {
                        printf ("Unrecognized Client Message\n");
                    }
                    } break;
                case 0: { // XCB_ERROR
                    xcb_generic_error_t *error = (xcb_generic_error_t*)event;
                    printf("Received X11 error %d\n", error->error_code);
                    } break;
                default: 
                    /* Unknown event type, ignore it */
                    continue;
            }
            free (event);
        }

        {
            // Notify X11: Start of frame
            increment_sync_counter (&counter_val);
            assert (counter_val.lo % 2 == 1);
            xcb_void_cookie_t ck = xcb_sync_set_counter_checked (connection, counters[1], counter_val);
            xcb_generic_error_t *error; 
            if ((error = xcb_request_check(connection, ck))) { 
                printf("Error setting counter %d\n", error->error_code); 
                free(error); 
            }
        }

        if (make_pixmap_bigger) {
            pixmap_width = graphics.width;
            pixmap_height = graphics.height;
            xcb_free_pixmap (connection, backbuffer);
            backbuffer = xcb_generate_id (connection);
            xcb_void_cookie_t ck = 
                xcb_create_pixmap_checked (connection, depth, backbuffer, window,
                                           pixmap_width, pixmap_height);
            xcb_generic_error_t *error; 
            if ((error = xcb_request_check(connection, ck))) { 
                printf("Error creating pixmap %d\n", error->error_code); 
                free(error); 
            }
            cairo_xlib_surface_set_drawable (cairo_get_target (graphics.cr), backbuffer,
                                             pixmap_width, pixmap_height);
            cairo_surface_flush (cairo_get_target(graphics.cr));
            make_pixmap_bigger = false;
            redraw = true;
        }

        if (redraw) {
            printf ("pixmap size: (%d, %d)\n", pixmap_width, pixmap_height);
        }
        bool blit_needed = draw (&graphics, redraw);
        xcb_flush (connection);

        cairo_status_t cr_stat = cairo_status (graphics.cr);
        if (cr_stat != CAIRO_STATUS_SUCCESS) {
            printf ("Cairo error: %s\n", cairo_status_to_string (cr_stat));
            return 0;
        }

        if (blit_needed || force_blit) {
            xcb_void_cookie_t ck =
                xcb_copy_area_checked (connection,
                                       backbuffer,  /* drawable we want to paste */
                                       window, /* drawable on which we copy the previous Drawable */
                                       gc,
                                       0,0,0,0,
                                       graphics.width,         /* pixel width of the region we want to copy */
                                       graphics.height);      /* pixel height of the region we want to copy */
            xcb_generic_error_t *error; 
            if ((error = xcb_request_check(connection, ck))) { 
                printf("Error blitting backbuffer %d\n", error->error_code); 
                free(error); 
            }
        }

        {
            // Notify X11: End of frame
            increment_sync_counter (&counter_val);
            assert (counter_val.lo % 2 == 0);
            xcb_void_cookie_t ck = xcb_sync_set_counter_checked (connection, counters[1], counter_val);
            xcb_generic_error_t *error; 
            if ((error = xcb_request_check(connection, ck))) { 
                printf("Error setting counter %d\n", error->error_code); 
                free(error); 
            }
        }

        clock_gettime (CLOCK_MONOTONIC, &end_ticks);
        float time_elapsed = time_elapsed_in_ms (&start_ticks, &end_ticks);
        if (time_elapsed < target_frame_length_ms) {
            struct timespec sleep_ticks;
            sleep_ticks.tv_sec = 0;
            sleep_ticks.tv_nsec = (long)((target_frame_length_ms-time_elapsed)*1000000);
            nanosleep (&sleep_ticks, NULL);
        } else {
            printf ("Frame missed! %f ms elapsed\n", time_elapsed);
        }

        clock_gettime(CLOCK_MONOTONIC, &end_ticks);
        //printf ("FPS: %f\n", 1000/time_elapsed_in_ms (&start_ticks, &end_ticks));
        start_ticks = end_ticks;

        xcb_flush (connection);
    }

    // These don't seem to free everything according to Valgrind, so we don't
    // care to free them, the process will end anyway.
    // cairo_surface_destroy(surface);
    // cairo_destroy (cr);
    // xcb_disconnect (connection);

    return 0;
}

