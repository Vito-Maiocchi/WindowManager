#include "X_Connection.h"
#include "WindowManager.h"
#include "Util.h"

#include <iostream>
#include <functional>

#define SIZE(n) (sizeof(n) / sizeof(n[0]))

void mapRequestCallback(unsigned client) {
    Extends ext = monitorGetExtends(0);

    clientSetBorderWidth(client, 2);
    clientSetBorderColor(client, "#D8F032");
    //clientSetDimensions(client, 0, 30, ext.width-4, ext.height-4-30);
    wmAddClient(client);
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
    wmRemoveClient(client);
}

bool running;
void quitCallback(unsigned client) {
    running = false;
}

EventCallback eventCallbacks[] = {
    {MAP_REQEST, mapRequestCallback}
};

std::vector<ShortCut> shortCuts = {
    {MOD_WIN | MOD_SHIFT,       'a',        goofyAhhCallback},
    {MOD_WIN | MOD_ALT,         'b',        goofyBhhCallback},
    {MOD_WIN | MOD_SHIFT,       'q',        quitCallback},
    {MOD_WIN,                   KEY_SPACE,  spawnRofi},
    {MOD_WIN,                   KEY_ENTER,  spawnTerminal},
    {MOD_WIN,                   'q',        closeClient},
    {MOD_WIN,                   'w',        wmToggleExpand}
};

ClientCallback tagCallbacks[9];

int main() {
    connect();

    for(unsigned i = 0; i < 9; i++) {
        tagCallbacks[i] = [i](unsigned client) { wmSetTag(i); };
        ShortCut sc = { MOD_WIN, (char) i+49U, tagCallbacks[i] };
        shortCuts.push_back(sc);
    }

    registerEventCallbacks(eventCallbacks, SIZE(eventCallbacks));
    registerShortCuts(shortCuts.data(), shortCuts.size());

    for(int i = 0; i < MONITOR_AMOUNT; i++) {
        titlebarInit(30, 15, i);
        Extends ext = monitorGetExtends(i);
        titlebarDrawStart(i);
        titlebarDrawRectangle(0, 0, ext.width, 30, "#AA55FF");
	    std::string str = "MONITOR: " + std::to_string(i+1) + " OF " + std::to_string(MONITOR_AMOUNT) + " aaahadsdas";
        titlebarDrawText(0, 30, ext.width, 30, str, "#000000");
        titlebarDrawFinish();
    }


    //WM_setup(extendsModify(monitorGetExtends(0), -30, UP));
    std::vector<Extends> wmExtends(MONITOR_AMOUNT);
    for(unsigned i = 0; i < MONITOR_AMOUNT; i++) wmExtends[i] = extendsModify(monitorGetExtends(i), -30, UP);
    wmSetup(wmExtends, 2);

    //clientSpawn("alacritty");

    running = true;
    while(running) eventListen();
}
