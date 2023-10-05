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
#include <optional>
#include <array>

#include "X_Connection.h"
#include "Util.h"

#define SCREEN(n) (screens[monitors[n].screen])
#define TITLEBAR(n) (monitors[n].titlebar)
#define MON_INFO(n) (monitors[n].monitor_info)

static Display* xDisplay;
static xcb_connection_t* connection;

struct xTitlebar {
    xcb_drawable_t drawable;
    xcb_pixmap_t pixmap;
    XftDraw* draw;
    unsigned titlebar_height;
};

struct xMonitor {
    xcb_randr_monitor_info_t* monitor_info;
    unsigned screen;
    xTitlebar titlebar;
};

struct xScreen {
    xcb_screen_t* xcb_screen;
    Extends extends;
    xcb_gcontext_t graphics_context;
    Visual* visual;
    XftFont* font;
    Colormap colormap;
    std::unordered_map<std::string, XftColor> colors;
    bool graphics_init = false;
};

static std::vector<xMonitor> monitors;
static std::vector<xScreen> screens;

bool clientIsValid(unsigned client) {
    if(client == 0) return false;
    for(xScreen screen : screens) if(screen.xcb_screen->root == client) return false;
    for(xMonitor monitor : monitors) if(monitor.titlebar.drawable == client) return false;
    return true;
}

void checkIfValid(unsigned client) {
    if (!clientIsValid(client)) throw std::runtime_error("Invalid Client");
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
void registerShortCuts(ShortCut shortCuts[], unsigned size) {
    for(unsigned i = 0; i < size; i++) 
        keyPressCallbackMap[getKeyCombination(shortCuts[i].ModMask, shortCuts[i].KeyStroke)] = shortCuts[i].callback;
}

std::unordered_map<EventType, ClientCallback> eventCallbackMap;
void registerEventCallbacks(EventCallback eventCallbacks[], unsigned size) {
    for(unsigned i = 0; i < size; i++)
        eventCallbackMap[eventCallbacks[i].eventType] = eventCallbacks[i].callback;
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

            if(eventCallbackMap.find(MAP_REQEST) != eventCallbackMap.end()) eventCallbackMap[MAP_REQEST](map_request_event->window);
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
        case XCB_PROPERTY_NOTIFY: {
            unsigned client = ((xcb_property_notify_event_t*)e)->window;
            std::string title = getTitle(client);
            if(title != titles[client]) {
                titles[client] = title;
                if(eventCallbackMap.find(TITLE_CHANGE) != eventCallbackMap.end()) eventCallbackMap[TITLE_CHANGE](client);
            }
        } break;
    }
}

int MONITOR_AMOUNT = 0;

void connect() {
    xDisplay = XOpenDisplay(0);
    connection = XGetXCBConnection(xDisplay);
    if (xcb_connection_has_error(connection) > 0) throw std::runtime_error("Connection Error");
    
    xcb_screen_iterator_t screenIterator = xcb_setup_roots_iterator(xcb_get_setup(connection));
    while (screenIterator.rem > 0) {
        xScreen s;
        s.xcb_screen = screenIterator.data;
        xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(connection, xcb_get_geometry(connection, s.xcb_screen->root), NULL);
        s.extends = {
            reply->x,
            reply->y,
            reply->width,
            reply->height
        };
        screens.push_back(s);
        xcb_screen_next(&screenIterator);
    }

    for (unsigned i = 0; i < screens.size(); i++) {
        xcb_randr_monitor_info_iterator_t monitorIterator = xcb_randr_get_monitors_monitors_iterator(xcb_randr_get_monitors_reply(connection, xcb_randr_get_monitors(connection, screens[i].xcb_screen->root, 1), NULL));
        while (monitorIterator.rem > 0) {
            if(monitorIterator.data->width != 0 && monitorIterator.data->height != 0 ) {
                xMonitor m;
                m.monitor_info = monitorIterator.data;
                m.screen = i;
                monitors.push_back(m);
            }
            xcb_randr_monitor_info_next(&monitorIterator);
        }
    }

    MONITOR_AMOUNT = monitors.size();   

    //std::cout << "SCREEN AMOUNT " << screens.size() << std::endl;
    //std::cout << "MONITOR AMOUNT " << monitors.size() << std::endl;
    for(xMonitor m : monitors) {
        //std::cout << "x: " << m.monitor_info->x << "; y: " << m.monitor_info->y << "; width: " << m.monitor_info->width << "; height: " << m.monitor_info->height << std::endl;
    } 

    uint32_t values[1];
    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
        | XCB_EVENT_MASK_PROPERTY_CHANGE;
    for (xScreen screen : screens) {
        xcb_change_window_attributes_checked(connection, screen.xcb_screen->root, XCB_CW_EVENT_MASK, values);
        xcb_ungrab_key(connection, XCB_GRAB_ANY, screen.xcb_screen->root, XCB_MOD_MASK_ANY);
        xcb_grab_key(connection, 1, screen.xcb_screen->root, XCB_MOD_MASK_4, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }
    xcb_flush(connection);
}

void disconnect() {
    xcb_disconnect(connection);
}

Extends monitorGetExtends(unsigned monitor_id) {
    if(monitor_id >= monitors.size()) throw std::runtime_error("Monitor does not exist");
    auto info = monitors[monitor_id].monitor_info;
    return {
        info->x + SCREEN(monitor_id).extends.x,
        info->y + SCREEN(monitor_id).extends.y,
        info->width,
        info->height
    };
}

void clientKill(unsigned int client) {
    xcb_kill_client(connection, client);
}

void clientSpawn(std::string command) {
    if (fork() == 0) {
        if (connection != NULL) {
            for (xScreen screen : screens) close(screen.xcb_screen->root);
        }
        setsid();
        if (fork() != 0) {
            _exit(0);
        }
        command += " &>/dev/null"; //disable the std output
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

void clientSetDimensions(unsigned client, unsigned x, unsigned y, unsigned width, unsigned height, unsigned border_width) {
    clientSetDimensions(client, x, y, width - 2*border_width, height - 2*border_width);
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


/*
        TITLEBAR
*/

void titlebarInit(unsigned height, double font_size, unsigned monitor_id) {
    if(monitor_id >= monitors.size()) throw std::runtime_error("Monitor does not exist");
    
    TITLEBAR(monitor_id).titlebar_height = height;

    TITLEBAR(monitor_id).drawable = xcb_generate_id (connection);
    uint32_t values[2] = {SCREEN(monitor_id).xcb_screen->white_pixel, XCB_EVENT_MASK_EXPOSURE};
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    xcb_create_window (connection, XCB_COPY_FROM_PARENT, TITLEBAR(monitor_id).drawable, SCREEN(monitor_id).xcb_screen->root, MON_INFO(monitor_id)->x, MON_INFO(monitor_id)->y, MON_INFO(monitor_id)->width, TITLEBAR(monitor_id).titlebar_height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, SCREEN(monitor_id).xcb_screen->root_visual, mask, values );
    xcb_map_window (connection, TITLEBAR(monitor_id).drawable);

    if(!SCREEN(monitor_id).graphics_init) {
        SCREEN(monitor_id).visual = DefaultVisual(xDisplay, monitors[monitor_id].screen);
        SCREEN(monitor_id).colormap = ScreenOfDisplay(xDisplay, monitors[monitor_id].screen)->cmap;

        SCREEN(monitor_id).font = XftFontOpen(xDisplay, monitors[monitor_id].screen, XFT_PIXEL_SIZE, XftTypeDouble, font_size, NULL);
        if(!SCREEN(monitor_id).font) throw std::runtime_error("No fitting font found");

        SCREEN(monitor_id).graphics_context = xcb_generate_id (connection);
        uint32_t mask_gc       = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
        uint32_t values_gc[2]  = {SCREEN(monitor_id).xcb_screen->black_pixel, 0};
        xcb_create_gc (connection, SCREEN(monitor_id).graphics_context, SCREEN(monitor_id).xcb_screen->root, mask_gc, values_gc);
    }

    TITLEBAR(monitor_id).pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, SCREEN(monitor_id).xcb_screen->root_depth, TITLEBAR(monitor_id).pixmap, TITLEBAR(monitor_id).drawable, MON_INFO(monitor_id)->width, TITLEBAR(monitor_id).titlebar_height);

    xcb_flush(connection);
}

static unsigned currentMonitor;
static bool drawing = false;

XftColor* getXftColor(std::string color) {
    if(SCREEN(currentMonitor).colors.find(color) != SCREEN(currentMonitor).colors.end()) return &(SCREEN(currentMonitor).colors[color]);
    XftColor xftColor;
    XftColorAllocName(xDisplay, SCREEN(currentMonitor).visual, SCREEN(currentMonitor).colormap, color.c_str(), &xftColor);
    SCREEN(currentMonitor).colors[color] = xftColor;
    return &(SCREEN(currentMonitor).colors[color]);
}

void titlebarDrawStart(int monitor_id) {
    if(drawing) throw std::runtime_error("Allready drawing a titlebar");
    if(monitor_id >= monitors.size()) throw std::runtime_error("Monitor does not exist");
    currentMonitor = monitor_id;
    drawing = true;
    TITLEBAR(currentMonitor).draw = XftDrawCreate(xDisplay, TITLEBAR(currentMonitor).pixmap, SCREEN(currentMonitor).visual, SCREEN(currentMonitor).colormap);
}

void titlebarDrawRectangle(int x, int y, int width, int height, std::string color) { //x,y relative to the monitor
    if(!drawing) throw std::runtime_error("Not currently drawing a titlebar");
    XftDrawRect(TITLEBAR(currentMonitor).draw, getXftColor(color), x, y, width, height);
}

void titlebarDrawText(int x, int y, int width, int height, std::string text, std::string color) { //x,y relative to the monitor
    if(!drawing) throw std::runtime_error("Not currently drawing a titlebar");
    XGlyphInfo extends;
    XftTextExtentsUtf8(xDisplay, SCREEN(currentMonitor).font, (unsigned char*) text.c_str(), text.length(), &extends);
    XftDrawString8(TITLEBAR(currentMonitor).draw, getXftColor(color), SCREEN(currentMonitor).font, x+(width-(int)extends.width)/2, y-(height-(int)extends.height)/2, (unsigned char*) text.c_str(), text.length() );
}

void titlebarDrawFinish() {
    if(!drawing) throw std::runtime_error("Not currently drawing a titlebar");
    xcb_copy_area(connection, TITLEBAR(currentMonitor).pixmap, TITLEBAR(currentMonitor).drawable, SCREEN(currentMonitor).graphics_context, 0, 0, 0, 0, MON_INFO(currentMonitor)->width, TITLEBAR(currentMonitor).titlebar_height);
    XftDrawDestroy(TITLEBAR(currentMonitor).draw);
    xcb_flush(connection);
    drawing = false;
}