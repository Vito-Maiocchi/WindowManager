#include <string>
#include <functional>

struct Extends;

#define KEY_ENTER 0xff0d
#define KEY_SPACE 0x0020
#define KEY_ESC 0xff1b

enum ModMask {
    MOD_SHIFT = 1,
    MOD_ALT   = 8,
    MOD_CTRL  = 4,
    MOD_WIN   = 64
};

void connect();
void disconnect();
void eventListen();

extern int MONITOR_AMOUNT;
Extends monitorGetExtends(unsigned monitor_id);

void clientSpawn(std::string command);
void clientSetBorderWidth(unsigned client, unsigned pixels);
void clientSetBorderColor(unsigned int client, std::string color);
void clientMap(unsigned int client);
void clientUnMap(unsigned int client);
void clientSetDimensions(unsigned int client, unsigned int x, unsigned int y, unsigned int width, unsigned int height);
void clientSetDimensions(unsigned client, unsigned x, unsigned y, unsigned width, unsigned height, unsigned border_width);
void clientKill(unsigned int client);
void clientInputFocus(unsigned int client);
std::string clientGetTitle(unsigned client);
bool clientIsValid(unsigned client);

void titlebarInit(unsigned height, double font_size, unsigned monitor_id);
void titlebarDrawStart(int monitor_id);
void titlebarDrawRectangle(int x, int y, int width, int height, std::string color);
void titlebarDrawText(int x, int y, int width, int height, std::string text, std::string color);
void titlebarDrawFinish();

//typedef void (*ClientCallback)(unsigned);
typedef std::function<void(unsigned)> ClientCallback;

enum EventType {
    MAP_REQEST,
    TITLE_CHANGE
};

struct ShortCut {
    unsigned ModMask;
    unsigned KeyStroke;
    ClientCallback callback;
};

struct EventCallback {
    EventType eventType;
    ClientCallback callback;
};

void registerShortCuts(ShortCut shortCuts[], unsigned size);
void registerEventCallbacks(EventCallback eventCallbacks[], unsigned size);
