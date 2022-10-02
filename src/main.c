#include <sys/wait.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <stdbool.h>  
#include <stdio.h>
#include <X11/Xlib-xcb.h>
#include "main.h"
#include "titlebar.h"


void print(const char* out) {
  FILE* pFile = fopen("logFile.txt", "a");
  fprintf(pFile, "%s\n",out);
  fclose(pFile);
}

const int titlebar_height = 100;

#define BORDER_WIDTH           1        /* 0 = no border effect */
#define BORDER_COLOR_UNFOCUSED 0x696969 /* 0xRRGGBB */
#define BORDER_COLOR_FOCUSED   0xFFFFFF /* 0xRRGGBB */

const xcb_keysym_t Key_Q = 'q';
const xcb_keysym_t Key_Enter = 0xff0d;
const xcb_keysym_t Key_Space = 0x0020;
const xcb_keysym_t Key_ESC = 0xff1b;

const xcb_keysym_t Key_Shift = 0xffe1;
const xcb_keysym_t Key_Alt = 0xffe9;
const xcb_keysym_t Key_Ctrl = 0xffe3;

static xcb_drawable_t     window;
static uint32_t           values[3];

static Display* display;

bool running = true;

static void handleDestroyNotify(xcb_generic_event_t * ev) {
    xcb_destroy_notify_event_t * e = (xcb_destroy_notify_event_t *) ev;
    xcb_kill_client(connection, e->window);
}

static void setFocus(xcb_drawable_t window) {
    if ((window != 0) && (window != screen->root)) {
        xcb_set_input_focus(connection, XCB_INPUT_FOCUS_POINTER_ROOT, window,
            XCB_CURRENT_TIME);
    }
}

static void handleEnterNotify(xcb_generic_event_t * ev) {
    xcb_enter_notify_event_t * e = ( xcb_enter_notify_event_t *) ev;
    setFocus(e->event);
}

static void setFocusColor(xcb_window_t window, int focus) {
    if ((BORDER_WIDTH > 0) && (screen->root != window) && (0 != window)) {
        uint32_t vals[1];
        vals[0] = focus ? BORDER_COLOR_FOCUSED : BORDER_COLOR_UNFOCUSED;
        xcb_change_window_attributes(connection, window, XCB_CW_BORDER_PIXEL, vals);
        xcb_flush(connection);
    }
}

static void handleFocusIn(xcb_generic_event_t * ev) {
    xcb_focus_in_event_t * e = (xcb_focus_in_event_t *) ev;
    setFocusColor(e->event, 1);
}

static void handleFocusOut(xcb_generic_event_t * ev) {
    xcb_focus_out_event_t * e = (xcb_focus_out_event_t *) ev;
    setFocusColor(e->event, 0);
}

static void spawn(char **com) {
    if (fork() == 0) {
        if (connection != NULL) {
            close(screen->root);
        }
        setsid();
        if (fork() != 0) {
            _exit(0);
        }
        execvp((char*)com[0], (char**)com);
        _exit(0);
    }
    wait(NULL);
}

static xcb_keycode_t * xcb_get_keycodes(xcb_keysym_t keysym) {
    xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(connection);
    xcb_keycode_t     * keycode;
    keycode = (!(keysyms) ? NULL : xcb_key_symbols_get_keycode(keysyms, keysym));
    xcb_key_symbols_free(keysyms);
    return keycode;
}

static xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode) {
    xcb_key_symbols_t * keysyms = xcb_key_symbols_alloc(connection);
    xcb_keysym_t        keysym;
    keysym = (!(keysyms) ? 0 : xcb_key_symbols_get_keysym(keysyms, keycode, 0));
    xcb_key_symbols_free(keysyms);
    return keysym;
}

static void handleMapRequest(xcb_map_request_event_t * event) {
    xcb_map_window(connection, event->window); //das au nöd
    uint32_t vals[5];
    vals[0] = 0;
    vals[1] = titlebar_height;
    vals[2] = screen->width_in_pixels;
    vals[3] = screen->height_in_pixels-titlebar_height;
    vals[4] = 2;
    xcb_configure_window(connection, event->window, XCB_CONFIG_WINDOW_X |
        XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
        XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH, vals);
    xcb_flush(connection);
    values[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE; // NONIG IN Xconnection
    xcb_change_window_attributes_checked(connection, event->window,
        XCB_CW_EVENT_MASK, values);
    setFocus(event->window);
}

bool Modifiers[3] = {false, false, false}; // Shift[0], Ctrl[1], Alt[2]

void handleKeyPress(xcb_key_press_event_t *  event) {
    xcb_keysym_t Key = xcb_get_keysym(event->detail);
    window = event->child;
    /*
    char s[9];
    sprintf(s,"%08x", Key);
    print(s);
    */

    switch (Key) {
    case (xcb_keysym_t)'q':
        if(event->state == (XCB_MOD_MASK_4 | XCB_MOD_MASK_SHIFT) ) running = false;
        else xcb_kill_client(connection, window);
        break;
    case Key_Enter:
        static char* terminal[] = { "alacritty", NULL };
        spawn(terminal);
        break;
    case Key_Shift:
        Modifiers[0] = true;
        break;
    case Key_Ctrl:
        Modifiers[1] = true;
        break;
    case Key_Alt:
        Modifiers[2] = true;
        break;
    }
}

void handleKeyRelease(xcb_key_release_event_t *  event) {
    xcb_keysym_t Key = xcb_get_keysym(event->detail);

    switch (Key) {
    case Key_Shift:
        Modifiers[0] = false;
        break;
    case Key_Ctrl:
        Modifiers[1] = false;
        break;
    case Key_Alt:
        Modifiers[2] = false;
        break;
    }
}


void loop() {
    xcb_generic_event_t * event = xcb_wait_for_event(connection);
    int type = event->response_type;
    switch(type) {
        case XCB_KEY_PRESS:
            handleKeyPress(( xcb_key_press_event_t *) event);
            break;
        case XCB_KEY_RELEASE:
            handleKeyRelease((xcb_key_release_event_t *)event);
            break;
        case XCB_MOTION_NOTIFY:
            break;
        case XCB_ENTER_NOTIFY:
            handleEnterNotify(event);
            break;
        case XCB_DESTROY_NOTIFY:
            handleDestroyNotify(event);
            break;
        case XCB_BUTTON_PRESS:
            break;
        case XCB_BUTTON_RELEASE:
            break;
        case XCB_NONE:
            break;
        case XCB_MAP_REQUEST:
            handleMapRequest((xcb_map_request_event_t*) event);
            break;
        case XCB_FOCUS_IN:
            handleFocusIn(event);
            break;
        case XCB_FOCUS_OUT:
            handleFocusOut(event);
            break;
        case XCB_MAPPING_NOTIFY:
            break;
        case XCB_EXPOSE:
            handleExpose((xcb_expose_event_t*) event);
            //running = false;
            break;
    }
    xcb_flush(connection);
}

int main(int argc, char * argv[]) {

    display = XOpenDisplay(0);
    connection = XGetXCBConnection(display);

    //connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(connection) > 0) return xcb_connection_has_error(connection);
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

    values[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
        | XCB_EVENT_MASK_PROPERTY_CHANGE;
    xcb_change_window_attributes_checked(connection, screen->root,
        XCB_CW_EVENT_MASK, values);
    xcb_ungrab_key(connection, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);
    xcb_grab_key(connection, 1, screen->root, XCB_MOD_MASK_4, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    //xcb_grab_key(connection, 1, screen->root, XCB_MOD_MASK_4 | XCB_MOD_MASK_SHIFT, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    //xcb_grab_key(connection, 1, screen->root, XCB_MOD_MASK_4 | XCB_MOD_MASK_CONTROL, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    //xcb_grab_key(connection, 1, screen->root, XCB_MOD_MASK_4 | XCB_MOD_MASK_CONTROL | XCB_MOD_MASK_SHIFT , XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_flush(connection);

    createTitlebar(connection, screen, titlebar_height, display);

    while (running) {
        if (xcb_connection_has_error(connection) > 0) return xcb_connection_has_error(connection);
        loop();
    }     

    xcb_disconnect(connection);
    return 42069;
}