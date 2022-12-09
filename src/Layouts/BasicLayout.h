#include "LayoutManager.h"

class BasicLayout: public LayoutManager {
    protected:
        uintList clients; 
        virtual void rebuild();
    public:
        void addClient(unsigned int client);
        void removeClient(unsigned int client);
        void setClients(unsigned int clients[]);
        unsigned int* getClients();
};