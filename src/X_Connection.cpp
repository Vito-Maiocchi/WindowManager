#include <unistd.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>

#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <iomanip>

#include "X_Connection.h"

static Display* xDisplay;
static xcb_connection_t* connection;
static xcb_screen_t* screen;

int screen_width = -1;
int screen_height = -1;

void checkIfValid(unsigned client) {
    if ((client == 0) || (client == screen->root)) throw std::runtime_error("Invalid Client");
}

std::string getTitle(unsigned client) {
   xcb_get_property_reply_t*  reply = xcb_get_property_reply(connection, xcb_get_property(connection, 0, client, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 0, 100), NULL); //100 char array
   std::string title((char*) xcb_get_property_value(reply), xcb_get_property_value_length(reply));
   return title;
}

std::unordered_map<unsigned, std::string> titles;
std::string clientGetTitle(unsigned client) {
    checkIfValid(client);
    if(titles.find(client) == titles.end()) throw std::runtime_error("Could not find Client Title");
    return titles[client];
}

static xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode) {
    xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(connection);
    xcb_keysym_t        keysym;
    keysym = (!(keysyms) ? 0 : xcb_key_symbols_get_keysym(keysyms, keycode, 0));
    xcb_key_symbols_free(keysyms);
    return keysym;
}

unsigned getKeyCombination(short modMask, short key) {
    return (static_cast<int>(modMask) << 16) | static_cast<int>(key);
}

std::unordered_map<unsigned, ClientCallback> keyPressCallbackMap;
void addKeyPressCallback(short modMask, short key, ClientCallback clientCallback) {
    keyPressCallbackMap[getKeyCombination(modMask, key)] = clientCallback;
}

VoidCallback exposeCallback = nullptr;
void setExposeCallback(VoidCallback callback) {
    exposeCallback = callback;
}

ClientCallback mapRequestCallback = nullptr;
void setMapRequestCallback(ClientCallback callback) {
    mapRequestCallback = callback;
}

ClientCallback titleChangeCallback = nullptr;
void setTitleChangeCallback(ClientCallback callback) {
    titleChangeCallback = callback;
}



void eventListen() {
    xcb_flush(connection);

    xcb_generic_event_t* e = xcb_wait_for_event(connection);
    int type = e->response_type;
    switch (type){
        case XCB_KEY_PRESS: {
            xcb_key_press_event_t* key_press_event = (xcb_key_press_event_t*) e;
            unsigned key = getKeyCombination(key_press_event->state, xcb_get_keysym(key_press_event->detail));

            if(keyPressCallbackMap.find(key) != keyPressCallbackMap.end()) {
                keyPressCallbackMap[key](key_press_event->child);
            }
        } break;
        case XCB_MAP_REQUEST: {
            xcb_map_request_event_t* map_request_event = (xcb_map_request_event_t*) e;
            uint32_t values[1];
            values[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE;
            xcb_change_window_attributes_checked(connection, map_request_event->window, XCB_CW_EVENT_MASK, values);

            titles[map_request_event->window] = getTitle(map_request_event->window);
            if(mapRequestCallback) mapRequestCallback(map_request_event->window);
        } break;
        case XCB_FOCUS_IN:
            //handleFocusIn(((xcb_focus_in_event_t*)e)->event);
        break;
        case XCB_FOCUS_OUT:
            //handleFocusOut(((xcb_focus_out_event_t*)e)->event);
        break;
        case XCB_ENTER_NOTIFY:
            //handleEnterNotify(((xcb_enter_notify_event_t*)e)->event);
        break;
        case XCB_LEAVE_NOTIFY:
            //handleLeaveNotify(((xcb_leave_notify_event_t*)e)->event);
        break;
        case XCB_DESTROY_NOTIFY:
            //handleDestroyNotify(((xcb_destroy_notify_event_t*)e)->event);
        break;
        case XCB_EXPOSE:
            std::cout << "AHHHH EXPOSE" << std::endl;
            exposeCallback();
        break;
        case XCB_PROPERTY_NOTIFY: {
            unsigned client = ((xcb_property_notify_event_t*)e)->window;
            std::string title = getTitle(client);
            if(title != titles[client]) {
                titles[client] = title;
                if(titleChangeCallback) titleChangeCallback(client);
            }
        } break;
    }
}

void connect() {
    xDisplay = XOpenDisplay(0);  //alles nöd nur ein einzige Monition
    connection = XGetXCBConnection(xDisplay);

    if (xcb_connection_has_error(connection) > 0) throw std::runtime_error("Connection Error");
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data; //de main monition

    uint32_t values[1];
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
}

void disconnect() {
    xcb_disconnect(connection);
}

void clientKill(unsigned int client) {
    xcb_kill_client(connection, client);
}

void clientSpawn(std::string command) {
    if (fork() == 0) {
        if (connection != NULL) {
            close(screen->root);
        }
        setsid();
        if (fork() != 0) {
            _exit(0);
        }
        system(command.c_str());
        _exit(0);
    }
    wait(NULL);
}

void clientInputFocus(unsigned int client) {
    checkIfValid(client);
    xcb_set_input_focus(connection, XCB_INPUT_FOCUS_POINTER_ROOT, client, XCB_CURRENT_TIME); 
}

void clientSetDimensions(unsigned int client, unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    checkIfValid(client);
    uint32_t vals[4] = {x, y, width, height};
    xcb_configure_window(connection, client, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, vals);
}

void clientSetBorderWidth(unsigned client, unsigned pixels) {
    checkIfValid(client);
    uint32_t vals[1] = {pixels};
    xcb_configure_window(connection, client, XCB_CONFIG_WINDOW_BORDER_WIDTH, vals);
}

// convert "#FFFFFF" to 0xFFFFFF
unsigned convertColor(std::string color) {
    color.erase(0, 1);
    unsigned colorValue;
    std::istringstream(color) >> std::hex >> colorValue;
    return colorValue;
}

void clientSetBorderColor(unsigned int client, std::string color) {
    checkIfValid(client);
    uint32_t vals[1] = {convertColor(color)};
    xcb_change_window_attributes(connection, client, XCB_CW_BORDER_PIXEL, vals);
}

void clientMap(unsigned int client) {
    xcb_map_window(connection, client);
}

void clientUnMap(unsigned int client) {
    xcb_unmap_window(connection, client);
}




//TITLEBAR

static xcb_drawable_t titlebar;
static xcb_pixmap_t pixmap;
static xcb_gcontext_t graphics_context;
static XftFont* font;
static XftDraw* draw;
static Visual* visual;
static Colormap colormap;

static unsigned titlebar_height;
static std::unordered_map<std::string, XftColor> colors;

XftColor* getXftColor(std::string color) {
    if(colors.find(color) != colors.end()) return &colors[color];
    XftColor xftColor;
    XftColorAllocName(xDisplay, visual, colormap, color.c_str(), &xftColor);
    colors[color] = xftColor;
    return &colors[color];
}

void titlebarInit(unsigned height, double font_size) {
    titlebar_height = height;

    titlebar = xcb_generate_id (connection);
    uint32_t values[2] = {screen->white_pixel, XCB_EVENT_MASK_EXPOSURE};
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    xcb_create_window (connection, XCB_COPY_FROM_PARENT, titlebar, screen->root, 0, 0, screen_width, titlebar_height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values );
    xcb_map_window (connection, titlebar);

    uint screen_nmbr = DefaultScreen(xDisplay);
    visual = DefaultVisual(xDisplay, screen_nmbr);
    colormap = ScreenOfDisplay(xDisplay, screen_nmbr)->cmap;

    font = XftFontOpen(xDisplay, screen_nmbr, XFT_PIXEL_SIZE, XftTypeDouble, font_size, NULL);
    if(!font) throw std::runtime_error("No fitting font found");

    graphics_context = xcb_generate_id (connection);
    uint32_t mask_gc       = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values_gc[2]  = {screen->black_pixel, 0};
    xcb_create_gc (connection, graphics_context, screen->root, mask_gc, values_gc);

    pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, screen->root_depth, pixmap, titlebar, screen_width, titlebar_height);

    xcb_flush(connection);
}

void titlebarDrawStart() {
    draw = XftDrawCreate(xDisplay, pixmap, visual, colormap);
}

void titlebarDrawRectangle(int x, int y, int width, int height, std::string color) {
    XftDrawRect(draw, getXftColor(color), x, y, width, height);
}

void titlebarDrawText(int x, int y, int width, int height, std::string text, std::string color) {
    XGlyphInfo extends;
    XftTextExtentsUtf8(xDisplay, font, (unsigned char*) text.c_str(), text.length(), &extends);
    XftDrawString8(draw, getXftColor(color), font, x+(width-(int)extends.width)/2, y-(height-(int)extends.height)/2, (unsigned char*) text.c_str(), text.length() );
}

void titlebarDrawFinish() {
    xcb_copy_area(connection, pixmap, titlebar, graphics_context, 0, 0, 0, 0, screen->width_in_pixels, titlebar_height);
    XftDrawDestroy(draw);
    xcb_flush(connection);
}