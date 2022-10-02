#include "titlebar.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>

static Display* display;
int screen_nmbr = 0;
Visual* visual;


int height;
xcb_gcontext_t  foreground;
xcb_gcontext_t  text;
xcb_drawable_t titlebar;

xcb_colormap_t  colormap;

xcb_alloc_color_reply_t* Color_Base;
xcb_alloc_color_reply_t* Color_Highlight;

xcb_connection_t* connection;
xcb_screen_t* screen;



xcb_pixmap_t pixmap;
xcb_gcontext_t graphics_context;

void Xft_setup() {
    screen_nmbr = DefaultScreen(display);
    visual = DefaultVisual(display, screen_nmbr);
    colormap = ScreenOfDisplay(display, screen_nmbr)->cmap;

    const char font_name[] = "Open Sans";
    XftFont* font  = XftFontOpenName(display, screen_nmbr, font_name);

    XftColor fg_color;

    XftColorAllocName(display, visual, colormap, "#ff00ff", &fg_color);

    XftColor bg_color;

    XftColorAllocName(display, visual, colormap, "#004400", &bg_color);

    graphics_context = xcb_generate_id (connection);
    uint32_t mask       = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values[2]  = {screen->black_pixel, 0};
    xcb_create_gc (connection, graphics_context, screen->root, mask, values);

    pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, screen->root_depth, pixmap, titlebar, screen->width_in_pixels, height);

    XftDraw* draw = XftDrawCreate(display, pixmap, visual, colormap);
    FcChar16 chars[] = {'l','e','c','k',' ','m','i','c','h'};
    XftDrawRect(draw, &bg_color, 10, 10, 900, 70);
    XftDrawString16(draw, &fg_color, font, 10, 20, chars, 9);
    XftDrawDestroy(draw);

        xcb_copy_area(connection, pixmap, titlebar, graphics_context, 0, 0, 0, 0, screen->width_in_pixels, height);
}



void generateGraphics(xcb_expose_event_t* exposeEvent) {
    xcb_copy_area(connection, pixmap, titlebar, graphics_context, exposeEvent->x, exposeEvent->x, 0, 0, screen->width_in_pixels, height);

    xcb_flush(connection);
} 

void createTitlebar(xcb_connection_t* c, xcb_screen_t* s, int h, Display* d) {
    connection  = c;
    display = d;
    screen = s;
    height = h;
    uint32_t values[2];
    uint32_t mask;

    titlebar = xcb_generate_id (connection);

    mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    values[0] = screen->white_pixel;
    values[1] = XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window (connection, XCB_COPY_FROM_PARENT, titlebar, screen->root, 0, 0, screen->width_in_pixels, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values );
    xcb_map_window (connection, titlebar);

    Xft_setup();

    xcb_flush (connection);
    
}

void handleExpose(xcb_expose_event_t* exposeEvent) {
    generateGraphics(exposeEvent);
}