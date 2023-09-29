#include <unistd.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>

#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <vector>

#include "X_Connection.h"

static Display* xDisplay;
static xcb_connection_t* connection;

struct Titlebar {
    xcb_drawable_t drawable;
    xcb_pixmap_t pixmap;
    xcb_gcontext_t graphics_context;
    XftFont* font;
    XftDraw* draw;
    Visual* visual;
    Colormap colormap;

    unsigned titlebar_height;
    std::unordered_map<std::string, XftColor> colors;
};

struct Monitor {
    int index;
    xcb_screen_t* xcb_screen;
    Titlebar titlebar;
};

static std::vector<Monitor> monitors;

void checkIfValid(unsigned client) {
    if (client == 0) throw std::runtime_error("Invalid Client");
    for(Monitor monitor : monitors) if(monitor.xcb_screen->root == client) throw std::runtime_error("Invalid Client");
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

int MONITOR_AMOUNT = 0;

void connect() {
    xDisplay = XOpenDisplay(0);  //alles nöd nur ein einzige Monition
    connection = XGetXCBConnection(xDisplay);
    if (xcb_connection_has_error(connection) > 0) throw std::runtime_error("Connection Error");
    
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(connection));
    do {
        Monitor m = {iter.index, iter.data, {}};
        monitors.push_back(m);
        xcb_screen_next(&iter);
    } while (iter.rem > 0);

    MONITOR_AMOUNT = monitors.size();

    std::vector<xcb_randr_monitor_info_t*> monitor_infos;

    for (Monitor monitor : monitors) {
        std::cout << "Screen " << monitor.index << ";" << std::endl;

        xcb_randr_monitor_info_iterator_t monitorIterator = xcb_randr_get_monitors_monitors_iterator(xcb_randr_get_monitors_reply(connection, xcb_randr_get_monitors(connection, monitor.xcb_screen->root, 1), NULL));
        monitor_infos.push_back(monitorIterator.data);
        while (monitorIterator.rem > 0) {
            xcb_randr_monitor_info_next(&monitorIterator);
            if(monitorIterator.data->width != 0 && monitorIterator.data->height != 0 )monitor_infos.push_back(monitorIterator.data);
        }
    }

    std::cout << "MONITOR AMOUNT " << monitor_infos.size() << std::endl;
    for(auto m : monitor_infos) {
        std::cout << "x: " << m->x << "; y: " << m->y << "; width: " << m->width << "; height: " << m->height << std::endl;
    }
    

    uint32_t values[1];
    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
        | XCB_EVENT_MASK_PROPERTY_CHANGE;
    for(Monitor monitor : monitors) {
        xcb_change_window_attributes_checked(connection, monitor.xcb_screen->root, XCB_CW_EVENT_MASK, values);
        xcb_ungrab_key(connection, XCB_GRAB_ANY, monitor.xcb_screen->root, XCB_MOD_MASK_ANY);
        xcb_grab_key(connection, 1, monitor.xcb_screen->root, XCB_MOD_MASK_4, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }
    xcb_flush(connection);
}

void disconnect() {
    xcb_disconnect(connection);
}

Extends monitorGetExtends(int monitor_id) {
    if(monitor_id >= monitors.size()) throw std::runtime_error("Monitor does not exist");
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, monitors[monitor_id].xcb_screen->root), NULL);
    return {
        reply->x,
        reply->y,
        reply->width,
        reply->height
    };
}

void clientKill(unsigned int client) {
    xcb_kill_client(connection, client);
}

void clientSpawn(std::string command) {
    if (fork() == 0) {
        if (connection != NULL) {
            for(Monitor monitor : monitors) close(monitor.xcb_screen->root);
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

static Titlebar* currentTitlebar;
static bool drawing = false;

XftColor* getXftColor(std::string color) {
    if(currentTitlebar->colors.find(color) != currentTitlebar->colors.end()) return &(currentTitlebar->colors[color]);
    XftColor xftColor;
    XftColorAllocName(xDisplay, currentTitlebar->visual, currentTitlebar->colormap, color.c_str(), &xftColor);
    currentTitlebar->colors[color] = xftColor;
    return &(currentTitlebar->colors[color]);
}

void titlebarInit(unsigned height, double font_size, int monitor_id) {
    if(monitor_id >= monitors.size()) throw std::runtime_error("Monitor does not exist");
    currentTitlebar = &(monitors[monitor_id].titlebar);

    currentTitlebar->titlebar_height = height;

    currentTitlebar->drawable = xcb_generate_id (connection);
    uint32_t values[2] = {monitors[monitor_id].xcb_screen->white_pixel, XCB_EVENT_MASK_EXPOSURE};
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    xcb_create_window (connection, XCB_COPY_FROM_PARENT, currentTitlebar->drawable, monitors[monitor_id].xcb_screen->root, 0, 0, monitors[monitor_id].xcb_screen->width_in_pixels, currentTitlebar->titlebar_height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, monitors[monitor_id].xcb_screen->root_visual, mask, values );
    xcb_map_window (connection, currentTitlebar->drawable);

    currentTitlebar->visual = DefaultVisual(xDisplay, monitor_id);
    currentTitlebar->colormap = ScreenOfDisplay(xDisplay, monitor_id)->cmap;

    currentTitlebar->font = XftFontOpen(xDisplay, monitor_id, XFT_PIXEL_SIZE, XftTypeDouble, font_size, NULL);
    if(!currentTitlebar->font) throw std::runtime_error("No fitting font found");

    currentTitlebar->graphics_context = xcb_generate_id (connection);
    uint32_t mask_gc       = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values_gc[2]  = {monitors[monitor_id].xcb_screen->black_pixel, 0};
    xcb_create_gc (connection, currentTitlebar->graphics_context, monitors[monitor_id].xcb_screen->root, mask_gc, values_gc);

    currentTitlebar->pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, monitors[monitor_id].xcb_screen->root_depth, currentTitlebar->pixmap, currentTitlebar->drawable, monitors[monitor_id].xcb_screen->width_in_pixels, currentTitlebar->titlebar_height);

    xcb_flush(connection);
}

static unsigned currentWidth;

void titlebarDrawStart(int monitor_id) {
    if(drawing) throw std::runtime_error("Allready drawing a titlebar");
    if(monitor_id >= monitors.size()) throw std::runtime_error("Monitor does not exist");
    currentTitlebar = &(monitors[monitor_id].titlebar);
    drawing = true;
    currentTitlebar->draw = XftDrawCreate(xDisplay, currentTitlebar->pixmap, currentTitlebar->visual, currentTitlebar->colormap);
    currentWidth = monitors[monitor_id].xcb_screen->width_in_pixels;
}

void titlebarDrawRectangle(int x, int y, int width, int height, std::string color) {
    if(!drawing) throw std::runtime_error("Not currently drawing a titlebar");
    XftDrawRect(currentTitlebar->draw, getXftColor(color), x, y, width, height);
}

void titlebarDrawText(int x, int y, int width, int height, std::string text, std::string color) {
    if(!drawing) throw std::runtime_error("Not currently drawing a titlebar");
    XGlyphInfo extends;
    XftTextExtentsUtf8(xDisplay, currentTitlebar->font, (unsigned char*) text.c_str(), text.length(), &extends);
    XftDrawString8(currentTitlebar->draw, getXftColor(color), currentTitlebar->font, x+(width-(int)extends.width)/2, y-(height-(int)extends.height)/2, (unsigned char*) text.c_str(), text.length() );
}

void titlebarDrawFinish() {
    if(!drawing) throw std::runtime_error("Not currently drawing a titlebar");
    xcb_copy_area(connection, currentTitlebar->pixmap, currentTitlebar->drawable, currentTitlebar->graphics_context, 0, 0, 0, 0, currentWidth, currentTitlebar->titlebar_height);
    XftDrawDestroy(currentTitlebar->draw);
    xcb_flush(connection);
    drawing = false;
}