#include <unistd.h>
#include <sys/wait.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb_cursor.h>

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
#define ROOT_WINDOW(n) (monitors[n].rootWindow)
#define GRAPHICS_CONTEXT_M(n) (SCREEN(n).graphicsContext)
#define GRAPHICS_CONTEXT_S(n) (screens[n].graphicsContext)

static Display* xDisplay;
static xcb_connection_t* connection;

struct xTitlebar {
    xcb_drawable_t drawable;
    xcb_pixmap_t pixmap;
    XftDraw* draw;
    unsigned titlebar_height;
};

struct xRootWindow {
    xcb_window_t xcb_window;
};

struct xMonitor {
    xcb_randr_monitor_info_t* monitor_info;
    unsigned screen;
    xTitlebar titlebar;
    xRootWindow rootWindow;
};

struct xGraphicsContext {
    xcb_gcontext_t xcb_gc;
    Visual* visual;
    XftFont* font;
    Colormap colormap;
    std::unordered_map<std::string, XftColor> colors;
};

struct xScreen {
    xcb_screen_t* xcb_screen;
    Extends extends;
    xGraphicsContext graphicsContext;
    //bool graphics_init = false;
};

static std::vector<xMonitor> monitors;
static std::vector<xScreen> screens;

bool clientIsValid(unsigned client) {
    if(client == 0) return false;
    for(xScreen screen : screens) if(screen.xcb_screen->root == client) return false;
    for(xMonitor monitor : monitors) if(monitor.titlebar.drawable == client || monitor.rootWindow.xcb_window == client) return false;
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
        case XCB_ENTER_NOTIFY: {
            auto window = ((xcb_enter_notify_event_t*)e)->event;
            for(unsigned monitor_id = 0; monitor_id < monitors.size(); monitor_id++) if(window == ROOT_WINDOW(monitor_id).xcb_window) std::cout << "ENTER MONITOR: " << monitor_id + 1 << std::endl;
        } break;
        case XCB_LEAVE_NOTIFY:
            //handleLeaveNotify(((xcb_leave_notify_event_t*)e)->event);
        break;
        case XCB_DESTROY_NOTIFY: {
            unsigned client = ((xcb_destroy_notify_event_t*)e)->window;
            if(eventCallbackMap.find(DESTORY_NOTIFY) != eventCallbackMap.end()) eventCallbackMap[DESTORY_NOTIFY](client);
        } break;
        case XCB_PROPERTY_NOTIFY: {
            unsigned client = ((xcb_property_notify_event_t*)e)->window;
            std::string title = getTitle(client);
            if(title != titles[client]) {
                titles[client] = title;
                if(eventCallbackMap.find(TITLE_CHANGE) != eventCallbackMap.end()) eventCallbackMap[TITLE_CHANGE](client);
            }
        } break;
        case XCB_MOTION_NOTIFY: {
            xcb_motion_notify_event_t* event = (xcb_motion_notify_event_t*) e;
            //std::cout << "MOTION NOTIFY -   x: " <<  event->root_x << "; y: " << event->root_y << std::endl;
        } break;
    }
}


void drawBackground(unsigned monitor_id) {
    //SPÖTERES PROBLEM
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

    const double font_size = 12; // CONSTANT FONT SIZE NIX GUT

    for (unsigned i = 0; i < screens.size(); i++) { //SCREEN ITERATOR

        //ESTABLISH GRAPHICS CONTEXT

        GRAPHICS_CONTEXT_S(i).visual = DefaultVisual(xDisplay, i);
        GRAPHICS_CONTEXT_S(i).colormap = ScreenOfDisplay(xDisplay, i)->cmap;

        GRAPHICS_CONTEXT_S(i).font = XftFontOpen(xDisplay, i, XFT_PIXEL_SIZE, XftTypeDouble, font_size, NULL);
        if(!GRAPHICS_CONTEXT_S(i).font) throw std::runtime_error("No fitting font found");

        GRAPHICS_CONTEXT_S(i).xcb_gc = xcb_generate_id (connection);
        uint32_t mask_gc       = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
        uint32_t values_gc[2]  = {screens[i].xcb_screen->black_pixel, 0};
        xcb_create_gc (connection, GRAPHICS_CONTEXT_S(i).xcb_gc, screens[i].xcb_screen->root, mask_gc, values_gc);

        //CREATE MONITORS

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

        xcb_cursor_context_t* ctx;
        xcb_cursor_context_new(connection, screens[i].xcb_screen, &ctx);
        xcb_cursor_t cursor = xcb_cursor_load_cursor(ctx, "left_ptr");
        xcb_change_window_attributes(connection, screens[i].xcb_screen->root, XCB_CW_CURSOR, &cursor);
        
        
    }

    MONITOR_AMOUNT = monitors.size();

    for(unsigned monitor_id = 0; monitor_id < MONITOR_AMOUNT; monitor_id++) { //MONITOR ITERATOR

        //CREATE ROOT WINDOW
        uint32_t values[2] = {SCREEN(monitor_id).xcb_screen->white_pixel, XCB_EVENT_MASK_EXPOSURE};
        uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

        ROOT_WINDOW(monitor_id).xcb_window = xcb_generate_id(connection);
        xcb_create_window(connection, XCB_COPY_FROM_PARENT, ROOT_WINDOW(monitor_id).xcb_window, SCREEN(monitor_id).xcb_screen->root, MON_INFO(monitor_id)->x, MON_INFO(monitor_id)->y, MON_INFO(monitor_id)->width, MON_INFO(monitor_id)->height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, SCREEN(monitor_id).xcb_screen->root_visual, mask, values);
        xcb_map_window (connection, ROOT_WINDOW(monitor_id).xcb_window);

        drawBackground(monitor_id);

        uint32_t eventMask = XCB_EVENT_MASK_ENTER_WINDOW;
        xcb_change_window_attributes_checked(connection, ROOT_WINDOW(monitor_id).xcb_window, XCB_CW_EVENT_MASK, &eventMask);
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

        /*
        xcb_cursor_context_t* ctx;
        xcb_cursor_context_new(connection, screen.xcb_screen, &ctx);
        xcb_cursor_t cursor = xcb_cursor_load_cursor(ctx, "left_ptr");
        xcb_grab_pointer(connection, 1, screen.xcb_screen->root, XCB_EVENT_MASK_POINTER_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, None, cursor, CurrentTime); */
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

    TITLEBAR(monitor_id).pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, SCREEN(monitor_id).xcb_screen->root_depth, TITLEBAR(monitor_id).pixmap, TITLEBAR(monitor_id).drawable, MON_INFO(monitor_id)->width, TITLEBAR(monitor_id).titlebar_height);

    xcb_flush(connection);
}

static unsigned currentMonitor;
static bool drawing = false;

XftColor* getXftColor(std::string color) {
    if(GRAPHICS_CONTEXT_M(currentMonitor).colors.find(color) != GRAPHICS_CONTEXT_M(currentMonitor).colors.end()) return &(GRAPHICS_CONTEXT_M(currentMonitor).colors[color]);
    XftColor xftColor;
    XftColorAllocName(xDisplay, GRAPHICS_CONTEXT_M(currentMonitor).visual, GRAPHICS_CONTEXT_M(currentMonitor).colormap, color.c_str(), &xftColor);
    GRAPHICS_CONTEXT_M(currentMonitor).colors[color] = xftColor;
    return &(GRAPHICS_CONTEXT_M(currentMonitor).colors[color]);
}

void titlebarDrawStart(int monitor_id) {
    if(drawing) throw std::runtime_error("Allready drawing a titlebar");
    if(monitor_id >= monitors.size()) throw std::runtime_error("Monitor does not exist");
    currentMonitor = monitor_id;
    drawing = true;
    TITLEBAR(currentMonitor).draw = XftDrawCreate(xDisplay, TITLEBAR(currentMonitor).pixmap, GRAPHICS_CONTEXT_M(currentMonitor).visual, GRAPHICS_CONTEXT_M(currentMonitor).colormap);
}

void titlebarDrawRectangle(int x, int y, int width, int height, std::string color) { //x,y relative to the monitor
    if(!drawing) throw std::runtime_error("Not currently drawing a titlebar");
    XftDrawRect(TITLEBAR(currentMonitor).draw, getXftColor(color), x, y, width, height);
}

void titlebarDrawText(int x, int y, int width, int height, std::string text, std::string color) { //x,y relative to the monitor
    if(!drawing) throw std::runtime_error("Not currently drawing a titlebar");
    XGlyphInfo extends;
    XftTextExtentsUtf8(xDisplay, GRAPHICS_CONTEXT_M(currentMonitor).font, (unsigned char*) text.c_str(), text.length(), &extends);
    XftDrawString8(TITLEBAR(currentMonitor).draw, getXftColor(color), GRAPHICS_CONTEXT_M(currentMonitor).font, x+(width-(int)extends.width)/2, y-(height-(int)extends.height)/2, (unsigned char*) text.c_str(), text.length() );
}

void titlebarDrawFinish() {
    if(!drawing) throw std::runtime_error("Not currently drawing a titlebar");
    xcb_copy_area(connection, TITLEBAR(currentMonitor).pixmap, TITLEBAR(currentMonitor).drawable, GRAPHICS_CONTEXT_M(currentMonitor).xcb_gc, 0, 0, 0, 0, MON_INFO(currentMonitor)->width, TITLEBAR(currentMonitor).titlebar_height);
    XftDrawDestroy(TITLEBAR(currentMonitor).draw);
    xcb_flush(connection);
    drawing = false;
}