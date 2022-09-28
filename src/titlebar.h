#ifndef TITLEBAR
#define TITLEBAR

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>

void createTitlebar(xcb_connection_t* connection, xcb_screen_t* screen, int h, Display* display);
void handleExpose(xcb_expose_event_t* exposeEvent);

#endif