#include "WindowManager.h"
#include "Xconnection.h"
//#include "Client.h"
#include "DisplayManager.h"

void draw() {
    titlebarDrawStart();
    titlebarDrawRectangle(4, 0, 0, screen_width, 30);
    titlebarDrawText(0, 10, 10, "LECK MICH DU HUERE NUTTE !!!");
    titlebarDrawFinalize();
}

//Display d;

int main() {
    if(!connect()) return -1;

    //d = Display(0, 0, screen_width, screen_height);
    setDisplay(0,0,screen_width,screen_height);

    while (running) {
        eventListen();
    }
    disconnect();
    return 1;
}

//Events
void handleKeyPress(unsigned int client, unsigned int key, unsigned int mod_mask) {
    if( (key == KEY_ENTER) && mod_mask == MODMASK_WIN) clientSpawn("alacritty");
    //if( (key == KEY_Q) && mod_mask == MODMASK_WIN) 
    //draw();
}

void handleEnterNotify(unsigned int client) {

}

void handleLeaveNotify(unsigned int client) {

}

void handleDestroyNotify(unsigned int client) {

}

void handleFocusIn(unsigned int client) {

}

void handleFocusOut(unsigned int client) {

}

void handleMapRequest(unsigned int client) {

    updateTitle(client);
    if(getTitle(client) == "Alacritty" && addBgTerminal(client)) return;

    clientSetBorderWidth(client, 2);
    clientSetBorderColor(client, 0xFFFFFF);
    clientSetDimensions(client, 0, 30, screen_width - 4, screen_height - 34);
    clientMap(client);

    titlebarDrawStart();
    titlebarDrawRectangle(4, 0, 0, screen_width, 30);
    titlebarDrawText(0, 10, 10, clientGetTitle(client));
    titlebarDrawFinalize();
}

void handleExpose() {
    //draw();
}

void handlePropertyChange(unsigned int client) {
    titlebarDrawStart();
    titlebarDrawRectangle(4, 0, 0, screen_width, 30);
    titlebarDrawText(0, 10, 10, clientGetTitle(client));
    titlebarDrawFinalize();
}