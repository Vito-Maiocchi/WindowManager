#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <stdbool.h>

extern bool running;
extern int screen_width;
extern int screen_height;

// Base functions
bool connect();
void eventListen();
void disconnect();

//Titlebar
/**
 * @param color Hex code like this: "#FFFFFF"
 */
void titlebarInit(int height, char font[], char colors[][8]);
void titlebarDrawStart();
void titlebarDrawRectangle(int color, int x, int y, int width, int height);
void titlebarDrawText(int color, int x, int y, char text[]);
void titlebarDrawFinalize();

//Windows
void clientKill(unsigned int client);
void clientSpawn(char command[]);
void clientInputFocus(unsigned int client);
void clientSetDimensions(unsigned int client, unsigned int x, unsigned int y, unsigned int width, unsigned int height);
void clientSetBorderWidth(unsigned int client, unsigned int pixels);
/**
 * @param color Hex code like this: 0xFFFFFF
 */
void clientSetBorderColor(unsigned int client, unsigned int color);