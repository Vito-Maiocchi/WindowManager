/**
 * Adds client id to be used as background Terminal.
 * Will return false if there is no new background Terminal needed.
 */
bool addBgTerminal(unsigned int id);

//Update Title of a Client
void updateTitle(unsigned int id);

//Get the Title of a Client
const char* getTitle(unsigned int id);

void setDisplay(int x, int y, int width, int height);