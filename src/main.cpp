#include "X_Connection.h"

#include <iostream>

void mapRequestCallback(unsigned client) {
    clientSetBorderWidth(client, 2);
    clientSetBorderColor(client, "#D8F032");
    clientSetDimensions(client, 0, 30, screen_width-4, screen_height-4-30);
    clientMap(client);
}

void goofyAhhCallback(unsigned client) {
    std::cout << "goofy ahh: " << client << std::endl;
}
void goofyBhhCallback(unsigned client) {
    std::cout << "goofy bhh: " << client << std::endl;
}


void exposeCallback() {
    std::cout << "AHHHHH" << std::endl;
    //titlebarDrawStart();
    //titlebarDrawRectangle("#D80032", 0, 0, screen_width, 30);
    //titlebarDrawText("#FFFFFF", 0, 0, "leckmich");
    //titlebarDrawFinalize();
    titlebarDrawStart();
    //drawRect(0, 0, screen_width, 30);
    //drawText(0, 0, screen_width, 30, "LECK MICH");
    titlebarDrawFinish();
}

int main() {
    connect();
    setMapRequestCallback(mapRequestCallback);
    addKeyPressCallback(WIN | SHIFT, 'a', goofyAhhCallback);
    addKeyPressCallback(WIN | ALT, 'b', goofyBhhCallback);

    titlebarInit(30, 15);

    titlebarDrawStart();
    titlebarDrawRectangle(0, 0, screen_width, 30, "#AA55FF");
    titlebarDrawText(0, 30, screen_width, 30, "LECK MICH", "#000000");
    titlebarDrawFinish();

    //titlebarInit(30, "Open Sans");
    setExposeCallback(exposeCallback);

    //clientSpawn("alacritty");


    while(true) eventListen();
}