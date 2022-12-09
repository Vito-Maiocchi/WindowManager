#include "Util.h"


enum Layout {BASIC_HORIZONTAL};

class LayoutManager {
    private:
        Dimensions dim;
    public:
        void setDimensions(Dimensions dim);
        Dimensions getDimensions();
        virtual void addClient(unsigned int client);
        virtual void removeClient(unsigned int client);
        virtual void setClients(unsigned int clients[]);
        virtual unsigned int* getClients();
};

LayoutManager getLayoutManager(Dimensions dim, Layout layout);
void changeLayout(LayoutManager &layoutManager, Layout layout);