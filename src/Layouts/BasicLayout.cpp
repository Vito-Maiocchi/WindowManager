#include "BasicLayout.h"
#include <algorithm>

void BasicLayout::addClient(unsigned int client) {
    if(clients.append(client)) rebuild();
}

void BasicLayout::removeClient(unsigned int client) {
    if(clients.remove(client)) rebuild();
}

void BasicLayout::setClients(unsigned int clients[]) {
    BasicLayout::clients.setArray(clients);
}

unsigned int* BasicLayout::getClients() {
    return clients.getArray();
}