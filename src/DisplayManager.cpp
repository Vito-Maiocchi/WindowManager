#include "DisplayManager.h"
#include "Xconnection.h"
#include <map>
#include <vector>

class Desktop;
void requestBgTerminal(Desktop* desktop);

        //CLIENT TITLES

std::map<unsigned int, char*> titles;

void updateTitle(unsigned int id) {
    titles.insert({id, clientGetTitle(id)});
}

const char* getTitle(unsigned int id) {
    std::map<unsigned int, char*>::iterator i = titles.find(id);
    if(i != titles.end()) return i-> second;
    return "NO TITLE FOUND!";
}

        //DESKTOP CLASS

class Desktop {
    private: 
        int x, y, width, height;
        bool hasBG = false;
        unsigned int bgTerminal = 0;
        std::vector<unsigned int> clients = {};

    public:
        Desktop() = default;
        Desktop(int x, int y, int width, int height) {
            Desktop::x = x;
            Desktop::y = y;
            Desktop::width = width;
            Desktop::height = height;
            requestBgTerminal(this);
        }
        
        void setBgTerminal(unsigned int id) {
            hasBG = true;
            bgTerminal = id;
            clientSetBorderWidth(id, 0);
            clientSetDimensions(id, x, y, width, height);
            clientMap(id);
        }
};

        //BACKGROUND TERMINAL

std::vector<Desktop*> bg_desktops;

void requestBgTerminal(Desktop* desktop) {
    bg_desktops.emplace_back(desktop);
    clientSpawn("alacritty");
}

bool addBgTerminal(unsigned int id) {
    if (bg_desktops.size() == 0) return false;
    bg_desktops[0]->setBgTerminal(id);
    return true;
}

        //DISPLAY CLASS

class Display {
    private:
        int x, y, width, height;
        Desktop desktop;
    public:
        Display() = default;
        Display(int x, int y, int width, int height) {
                Display::x = x;
                Display::y = y;
                Display::width = width;
                Display::height = height;

                // [TITLE BAR PLACEHOLDER]
                const int titlebar_height = 30;

                char Colors[][8] = {
                    "#FF0000",
                    "#00FF00",
                    "#0000FF",
                    "#777777",
                    "#777777",
                    "#777777",
                    "#777777",
                };
                titlebarInit(titlebar_height, "Open Sans", Colors);
                titlebarDrawStart();
                titlebarDrawRectangle(4, 0, 0, screen_width, titlebar_height);
                titlebarDrawText(0, 10, 10, "LECK MICH DU HUERE NUTTE !!!");
                titlebarDrawFinalize();
                // [\TITLE BAR PLACEHOLDER]

                desktop = Desktop(x, y+titlebar_height, width, height-titlebar_height);
        }
};

        //DISPLAY MANAGER

Display display;

void setDisplay(int x, int y, int width, int height) {
    display = Display(x, y, width, height);
}