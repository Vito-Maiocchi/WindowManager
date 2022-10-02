#include "Xconnection.h"
#include <unistd.h>
#include <sys/wait.h>
#include "WindowManager.h"

Display* display;
xcb_connection_t* connection;
xcb_screen_t* screen;
uint32_t values[3];

int screen_width = -1;
int screen_height = -1;
bool running = false;

//UTIL
static xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode) {
    xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(connection);
    xcb_keysym_t        keysym;
    keysym = (!(keysyms) ? 0 : xcb_key_symbols_get_keysym(keysyms, keycode, 0));
    xcb_key_symbols_free(keysyms);
    return keysym;
}

// Base functions
void eventListen() {
    xcb_generic_event_t* e = xcb_wait_for_event(connection);
    int type = e->response_type;
    switch (type){
        case XCB_KEY_PRESS: {
            xcb_key_press_event_t* key_press_event = (xcb_key_press_event_t*) e;
            xcb_keysym_t Key = xcb_get_keysym(key_press_event->detail);
            handleKeyPress(key_press_event->child, Key, key_press_event->state);
        } break;
        case XCB_MAP_REQUEST: {
            xcb_map_request_event_t* map_request_event = (xcb_map_request_event_t*) e;
            values[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE;
            xcb_change_window_attributes_checked(connection, map_request_event->window, XCB_CW_EVENT_MASK, values);
            handleMapRequest(map_request_event->window);
            xcb_map_window(connection, map_request_event->window);
            xcb_flush(connection);
        } break;
        case XCB_FOCUS_IN:
            handleFocusIn(((xcb_focus_in_event_t*)e)->event);
        break;
        case XCB_FOCUS_OUT:
            handleFocusOut(((xcb_focus_out_event_t*)e)->event);
        break;
        case XCB_ENTER_NOTIFY:
            handleEnterNotify(((xcb_enter_notify_event_t*)e)->event);
        break;
        case XCB_DESTROY_NOTIFY:
            handleDestroyNotify(((xcb_destroy_notify_event_t*)e)->event);
        break;
        case XCB_EXPOSE:
            handleExpose();
        break;
    }
}

bool connect() {
    display = XOpenDisplay(0);
    connection = XGetXCBConnection(display);

    if (xcb_connection_has_error(connection) > 0) return false;
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
        | XCB_EVENT_MASK_PROPERTY_CHANGE;
    xcb_change_window_attributes_checked(connection, screen->root,
        XCB_CW_EVENT_MASK, values);
    xcb_ungrab_key(connection, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);
    xcb_grab_key(connection, 1, screen->root, XCB_MOD_MASK_4, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_flush(connection);

    screen_width = screen->width_in_pixels;
    screen_height = screen->height_in_pixels;
    running = true;

    return true;
}

void disconnect() {
    xcb_disconnect(connection);
}

#define MAX_COLORS 10

//Titlebar
xcb_drawable_t titlebar;
xcb_pixmap_t pixmap;
xcb_gcontext_t graphics_context;
XftFont* font;
XftColor colors[MAX_COLORS];
XftDraw* draw;
Visual* visual;
Colormap colormap;
int titlebar_height;

void titlebarInit(int height, char font_name[], char color_names[][8]) {
    titlebar_height = height;

    titlebar = xcb_generate_id (connection);
    uint32_t values[2] = {screen->white_pixel, XCB_EVENT_MASK_EXPOSURE};
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    xcb_create_window (connection, XCB_COPY_FROM_PARENT, titlebar, screen->root, 0, 0, screen_width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values );
    xcb_map_window (connection, titlebar);

    uint screen_nmbr = DefaultScreen(display);
    visual = DefaultVisual(display, screen_nmbr);
    colormap = ScreenOfDisplay(display, screen_nmbr)->cmap;

    font  = XftFontOpenName(display, screen_nmbr, font_name);

    for(int i = 0; i < MAX_COLORS; i++) XftColorAllocName(display, visual, colormap, color_names[i], &(colors[i]));

    graphics_context = xcb_generate_id (connection);
    uint32_t mask_gc       = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values_gc[2]  = {screen->black_pixel, 0};
    xcb_create_gc (connection, graphics_context, screen->root, mask_gc, values_gc);

    pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, screen->root_depth, pixmap, titlebar, screen_width, height);

    xcb_flush(connection);
}

void titlebarDrawStart() {
    draw = XftDrawCreate(display, pixmap, visual, colormap);
}

void titlebarDrawRectangle(int color, int x, int y, int width, int height) {
    XftDrawRect(draw, &(colors[color]), x, y, width, height);
}

void titlebarDrawText(int color, int x, int y, char text[]) {
    XftDrawString8(draw, &(colors[color]), font, x, y, (unsigned char*)text, strlen(text));
}

void titlebarDrawFinalize() {
    xcb_copy_area(connection, pixmap, titlebar, graphics_context, 0, 0, 0, 0, screen->width_in_pixels, titlebar_height);
    XftDrawDestroy(draw);
    xcb_flush(connection);
}

//Windows
void clientKill(unsigned int client) {
    xcb_kill_client(connection, client);
}

void clientSpawn(char command[]) {
        if (fork() == 0) {
        if (connection != NULL) {
            close(screen->root);
        }
        setsid();
        if (fork() != 0) {
            _exit(0);
        }
        system(command);
        //execvp((char*)com[0], (char**)com);
        _exit(0);
    }
    wait(NULL);
}

void clientInputFocus(unsigned int client) {
    if ((client != 0) && (client != screen->root)) xcb_set_input_focus(connection, XCB_INPUT_FOCUS_POINTER_ROOT, client, XCB_CURRENT_TIME); 
}

void clientSetDimensions(unsigned int client, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    uint32_t vals[4] = {x, y, width, height};
    xcb_configure_window(connection, client, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, vals);
    xcb_flush(connection);
}

void clientSetBorderWidth(unsigned int client, unsigned int pixels) {
    uint32_t vals[1] = {pixels};
    xcb_configure_window(connection, client, XCB_CONFIG_WINDOW_BORDER_WIDTH, vals);
    xcb_flush(connection);
}

void clientSetBorderColor(unsigned int client, unsigned int color) {
    if (screen->root != client) {
        uint32_t vals[1] = {color};
        xcb_change_window_attributes(connection, client, XCB_CW_BORDER_PIXEL, vals);
        xcb_flush(connection);
    }
}
