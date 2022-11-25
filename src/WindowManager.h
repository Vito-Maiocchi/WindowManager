
#define MODMASK_WIN 64
#define MODMASK_SHIFT 1
#define MODMASK_CTRL 4

#define KEY_ENTER 0xff0d
#define KEY_SPACE 0x0020
#define KEY_ESC 0xff1b
#define KEY_Q 'q'

//Events
void handleKeyPress(unsigned int client, unsigned int key, unsigned int mod_mask);
void handleEnterNotify(unsigned int client);
void handleLeaveNotify(unsigned int client);
void handleDestroyNotify(unsigned int client);
void handleFocusIn(unsigned int client);
void handleFocusOut(unsigned int client);
void handleMapRequest(unsigned int client);
void handleExpose();
void handlePropertyChange(unsigned int client);