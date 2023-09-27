#include <string>

#define KEY_ENTER 0xff0d
#define KEY_SPACE 0x0020
#define KEY_ESC 0xff1b

enum ModMask {
    SHIFT = 1,
    ALT   = 8,
    CTRL  = 4,
    WIN   = 64
};

extern int screen_width;
extern int screen_height;

void connect();
void disconnect();
void eventListen();

void clientSpawn(std::string command);
void clientSetBorderWidth(unsigned client, unsigned pixels);
void clientSetBorderColor(unsigned int client, std::string color);
void clientMap(unsigned int client);
void clientUnMap(unsigned int client);
void clientSetDimensions(unsigned int client, unsigned int x, unsigned int y, unsigned int width, unsigned int height);
void clientKill(unsigned int client);
void clientInputFocus(unsigned int client);
std::string clientGetTitle(unsigned client);


typedef void (*ClientCallback)(unsigned);
typedef void (*VoidCallback)();

void setExposeCallback(VoidCallback callback);
void setMapRequestCallback(ClientCallback callback);
void setTitleChangeCallback(ClientCallback callback);
void addKeyPressCallback(short modMask, short key, ClientCallback clientCallback);


void titlebarInit(unsigned height, double FontSize);
void titlebarDrawStart();
void titlebarDrawRectangle(int x, int y, int width, int height, std::string color);
void titlebarDrawText(int x, int y, int width, int height, std::string text, std::string color);
void titlebarDrawFinish();