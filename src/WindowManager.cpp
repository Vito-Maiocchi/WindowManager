#include "WindowManager.h"
#include "Xconnection.h"

void draw() {
    titlebarDrawStart();
    titlebarDrawRectangle(4, 0, 0, screen_width, 30);
    titlebarDrawText(0, 10, 10, "LECK MICH DU HUERE NUTTE !!!");
    titlebarDrawFinalize();
}

int main() {
    if(!connect()) return -1;

    char Colors[][8] = {
        "#FF0000",
        "#00FF00",
        "#0000FF",
        "#777777",
        "#777777",
        "#777777",
        "#777777",
    };
    titlebarInit(30, "Open Sans", Colors);
    titlebarDrawStart();
    titlebarDrawRectangle(4, 0, 0, screen_width, 30);
    titlebarDrawText(0, 10, 10, "LECK MICH DU HUERE NUTTE !!!");
    titlebarDrawFinalize();

    while (running) {
        eventListen();
    }
    disconnect();
    return 1;
}

//Events
void handleKeyPress(unsigned int client, unsigned int key, unsigned int mod_mask) {
    if( (key == KEY_ENTER) && mod_mask == MODMASK_WIN) clientSpawn("alacritty");
    //draw();
}

void handleEnterNotify(unsigned int client) {

}

void handleDestroyNotify(unsigned int client) {

}

void handleFocusIn(unsigned int client) {

}

void handleFocusOut(unsigned int client) {

}

void handleMapRequest(unsigned int client) {
    clientSetBorderWidth(client, 2);
    clientSetBorderColor(client, 0xFFFFFF);
    clientSetDimensions(client, 2, 32, screen_width - 4, screen_height - 34);
}

void handleExpose() {
    //draw();
}