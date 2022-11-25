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
void titlebarInit(int height, const char font[], const char colors[][8]);
void titlebarDrawStart();
void titlebarDrawRectangle(int color, int x, int y, int width, int height);
void titlebarDrawText(int color, int x, int y, const char text[]);
void titlebarDrawFinalize();

//Windows
void clientKill(unsigned int client);
void clientSpawn(const char command[]);
void clientInputFocus(unsigned int client);
void clientSetDimensions(unsigned int client, unsigned int x, unsigned int y, unsigned int width, unsigned int height);
void clientSetBorderWidth(unsigned int client, unsigned int pixels);
/**
 * @param color Hex code like this: 0xFFFFFF
 */
void clientSetBorderColor(unsigned int client, unsigned int color);
void clientMap(unsigned int client);
void clientUnMap(unsigned int client);
char* clientGetTitle(unsigned int client);