#include <vector>

//Client
void updateTitle(unsigned int id);
const char* getTitle(unsigned int id);
//unsigned int focused_client = -1;

//Desktop
class Desktop {
    private: 
        int x, y, width, height;
        bool hasBG = false;
        unsigned int bgTerminal = 0;
        std::vector<unsigned int> clients = {};

    public:
        Desktop(int x, int y, int width, int height);
        void setBgTerminal(unsigned int id); 
        Desktop() {}
};

/**
 * @brief 
 * Adds client id to be used as background Terminal.
 * Will return false if there is no new background Terminal needed.
 */
bool addBgTerminal(unsigned int id);


//Display
class Display {
    private:
        int x, y, width, height;
        Desktop desktop;
    public:
        Display(int x, int y, int width, int height);
        Display() {}
        void destroyClient(unsigned int id);
        void mapClient(unsigned int id);
        void enterClient(unsigned int id);
        void leaveClient(unsigned int id);
};

class DisplayManager {
    private:
        Display display;
    public:
        void setDisplay(int x, int y, int width, int height);
        void destroyClient(unsigned int id);
        void mapClient(unsigned int id);
        void enterClient(unsigned int id);
        void leaveClient(unsigned int id);
};

