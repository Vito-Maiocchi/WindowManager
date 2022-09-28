#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

static xcb_connection_t * connection;
static xcb_screen_t     * screen;

void print(const char* out);