//#include "X_Connection.h"
#include "WindowManager.h"

#include <iostream>

#define SIZE(n) (sizeof(n) / sizeof(n[0]))

void mapRequestCallback(unsigned client) {
    Extends ext = monitorGetExtends(0);

    clientSetBorderWidth(client, 2);
    clientSetBorderColor(client, "#D8F032");
    //clientSetDimensions(client, 0, 30, ext.width-4, ext.height-4-30);
    addClient(client);
    clientMap(client);
}

void goofyAhhCallback(unsigned client) {
    std::cout << "goofy ahh: " << client << std::endl;
}
void goofyBhhCallback(unsigned client) {
    std::cout << "goofy bhh: " << client << std::endl;
}

void spawnTerminal(unsigned client) {
    clientSpawn("alacritty");
}

void spawnRofi(unsigned client) {
    clientSpawn("rofi -show drun");
}

void closeClient(unsigned client) {
    if(!clientIsValid(client)) return;
    clientKill(client);
    removeClient(client); //es tuet au root remove
}

bool running;
void quitCallback(unsigned client) {
    running = false;
}

EventCallback eventCallbacks[] = {
    {MAP_REQEST, mapRequestCallback}
};

ShortCut shortCuts[] = {
    {MOD_WIN | MOD_SHIFT,       'a',        goofyAhhCallback},
    {MOD_WIN | MOD_ALT,         'b',        goofyBhhCallback},
    {MOD_WIN | MOD_SHIFT,       'q',        quitCallback},
    {MOD_WIN,                   KEY_SPACE,  spawnRofi},
    {MOD_WIN,                   KEY_ENTER,  spawnTerminal},
    {MOD_WIN,                   'q',        closeClient},
    {MOD_WIN,                   'w',        toggle_expand}
};

int main() {
    connect();

    registerEventCallbacks(eventCallbacks, SIZE(eventCallbacks));
    registerShortCuts(shortCuts, SIZE(shortCuts));

    for(int i = 0; i < MONITOR_AMOUNT; i++) {
        titlebarInit(30, 15, i);
        Extends ext = monitorGetExtends(i);
        titlebarDrawStart(i);
        titlebarDrawRectangle(0, 0, ext.width, 30, "#AA55FF");
	    std::string str = "MONITOR: " + std::to_string(i+1) + " OF " + std::to_string(MONITOR_AMOUNT) + " aaahadsdas";
        titlebarDrawText(0, 30, ext.width, 30, str, "#000000");
        titlebarDrawFinish();
    }

    Extends ext = monitorGetExtends(0);
    WM_setup({ext.x, ext.y + 30, ext.width, ext.height - 30});
    //titlebarInit(30, "Open Sans");
    //setExposeCallback(exposeCallback);

    //clientSpawn("alacritty");

    running = true;
    while(running) eventListen();
}
