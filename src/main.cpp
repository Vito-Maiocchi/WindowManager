#include "X_Connection.h"

#include <iostream>

void mapRequestCallback(unsigned client) {
    Extends ext = monitorGetExtends(0);

    clientSetBorderWidth(client, 2);
    clientSetBorderColor(client, "#D8F032");
    clientSetDimensions(client, 0, 30, ext.width-4, ext.height-4-30);
    clientMap(client);
}

void goofyAhhCallback(unsigned client) {
    std::cout << "goofy ahh: " << client << std::endl;
}
void goofyBhhCallback(unsigned client) {
    std::cout << "goofy bhh: " << client << std::endl;
}

bool running;

void quitCallback(unsigned client) {
    running = false;
}


void exposeCallback() {
    std::cout << "AHHHHH" << std::endl;
    //titlebarDrawStart();
    //titlebarDrawRectangle("#D80032", 0, 0, screen_width, 30);
    //titlebarDrawText("#FFFFFF", 0, 0, "leckmich");
    //titlebarDrawFinalize();
    titlebarDrawStart(0);
    //drawRect(0, 0, screen_width, 30);
    //drawText(0, 0, screen_width, 30, "LECK MICH");
    titlebarDrawFinish();
}

int main() {
    connect();
    setMapRequestCallback(mapRequestCallback);
    addKeyPressCallback(WIN | SHIFT, 'a', goofyAhhCallback);
    addKeyPressCallback(WIN | ALT, 'b', goofyBhhCallback);
    addKeyPressCallback(WIN | SHIFT, 'q', quitCallback);

    for(int i = 0; i < MONITOR_AMOUNT; i++) {
        titlebarInit(30, 15, i);
        Extends ext = monitorGetExtends(i);
        titlebarDrawStart(i);
        titlebarDrawRectangle(0, 0, ext.width, 30, "#AA55FF");
	std::string str = "aahh " + std::to_string(MONITOR_AMOUNT) + " aaahadsdas";
        titlebarDrawText(0, 30, ext.width, 30, str, "#000000");
        titlebarDrawFinish();
    }

    //titlebarInit(30, "Open Sans");
    //setExposeCallback(exposeCallback);

    //clientSpawn("alacritty");

    running = true;
    while(running) eventListen();
}
