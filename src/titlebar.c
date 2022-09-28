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
    //XVisualInfo xv; 
    //xv.depth = 32;
    //int result = 0;
    visual = DefaultVisual(display, screen_nmbr);
    colormap = ScreenOfDisplay(display, screen_nmbr)->cmap;
    //visual = XGetVisualInfo(display, VisualDepthMask, &xv, &result)->visual;

    const char font_name[] = "Open Sans";
    XftFont* font  = XftFontOpenName(display, screen_nmbr, font_name);

    XftColor fg_color;

    XftColorAllocName(display, visual, colormap, "#ff00ff", &fg_color);

    XftColor bg_color;

    XftColorAllocName(display, visual, colormap, "#004400", &bg_color);

    graphics_context = xcb_generate_id (connection);
    uint32_t mask       = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values[2]  = {Color_Highlight->pixel, 0};
    xcb_create_gc (connection, graphics_context, screen->root, mask, values);

    pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, screen->root_depth, pixmap, titlebar, screen->width_in_pixels, height);

    XftDraw* draw = XftDrawCreate(display, pixmap, visual, colormap);
    FcChar16 chars[] = {'l','e','c','k',' ','m','i','c','h'};
    XftDrawRect(draw, &bg_color, 10, 10, 900, 70);
    XftDrawString16(draw, &fg_color, font, 10, 20, chars, 9);

        xcb_copy_area(connection, pixmap, titlebar, graphics_context, 0, 0, 0, 0, screen->width_in_pixels, height);
}


// TODO :  https://github.com/drscream/lemonbar-xft/blob/xft-port/lemonbar.c Xft font


void generateGraphics(xcb_expose_event_t* exposeEvent) {

    xcb_rectangle_t      rectangles[] = {
            { 50, 0, 40, (uint16_t)height},
            { 200, 0, 40, (uint16_t)height}};

    


    xcb_poly_fill_rectangle (connection, titlebar, foreground, 2, rectangles);

    uint8_t ka[] = {'l','e','c','k',' ','m','i','c','h'};    

    xcb_image_text_8(connection, 28 , titlebar, text, 10, 30, "100000 Euro auf Bravolotto!");
    //xcb_poly_text_16(connection, titlebar, text, 150, 10, 28, (uint8_t*)"100000 Euro auf Bravolotto!");
    //xcb_poly_text_8(connection, titlebar, text, 300, 10, sizeof(ka)/sizeof(uint8_t), ka);

    xcb_copy_area(connection, pixmap, titlebar, graphics_context, exposeEvent->x, exposeEvent->x, 0, 0, screen->width_in_pixels, height);

    xcb_flush(connection);
} 

void createTitlebar(xcb_connection_t* c, xcb_screen_t* s, int h, Display* d) {
    connection  = c;
    display = d;
    screen = s;
    height = h;

    colormap = screen->default_colormap;

    xcb_alloc_color_cookie_t cookie_base = xcb_alloc_color(connection, colormap, 65535, 30000, 0);
    xcb_alloc_color_cookie_t cookie_highlight = xcb_alloc_color(connection, colormap, 0, 65535, 0);

    Color_Base = xcb_alloc_color_reply(connection, cookie_base, NULL);
    Color_Highlight = xcb_alloc_color_reply(connection, cookie_highlight, NULL);

    xcb_free_colormap(connection, colormap);

    xcb_font_t font = xcb_generate_id(connection);
    //xcb_open_font(connection, font, 64,"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");
    xcb_open_font(connection, font, 64,"-*-open sans-*-r-*-*-*-*-*-*-*-*-iso8859-*");


    //xcb_drawable_t drawable  = screen->root;
    foreground = xcb_generate_id (connection);
    uint32_t mask       = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values[2]  = {Color_Highlight->pixel, 0};
    xcb_create_gc (connection, foreground, screen->root, mask, values);

    text = xcb_generate_id (connection);
    uint32_t mask_2       = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values_2[4]  = {screen->white_pixel, screen->black_pixel, font, 1};
    xcb_create_gc (connection, text, screen->root, mask_2, values_2);


    xcb_close_font(connection, font);

        // Create a window 
    titlebar = xcb_generate_id (connection);

    mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    //values[0] = screen->white_pixel;
    values[0] = Color_Base->pixel;
    values[1] = XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window (connection,                    // connection          
                       XCB_COPY_FROM_PARENT,          // depth               
                       titlebar,                        // window Id           
                       screen->root,                  // parent window       
                       0, 0,                          // x, y                
                       screen->width_in_pixels, height,                      // width, height       
                       0,                            // border_width        
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, // class               
                       screen->root_visual,           // visual              
                       mask, values );                // masks 


        // Map the window on the screen and flush
    xcb_map_window (connection, titlebar);

    Xft_setup();

    xcb_flush (connection);
    
}

void handleExpose(xcb_expose_event_t* exposeEvent) {
    generateGraphics(exposeEvent);
}