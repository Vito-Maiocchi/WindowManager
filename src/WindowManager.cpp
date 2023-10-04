#include "X_Connection.h"

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <iostream>

const unsigned border_width = 2;

#define INVALID -1

void mapVector(Extends ext, std::vector<unsigned> &clients) {
    if(clients.size() == 0) throw std::runtime_error("no clients in vector");
    unsigned h = ext.height / clients.size();
    for(unsigned i = 0; i < clients.size()-1; i++) clientSetDimensions(clients[i], ext.x, ext.y + i*h, ext.width, h, border_width);
    clientSetDimensions(*(clients.end()-1), ext.x, ext.y + h*(clients.size()-1), ext.width, ext.height- h*(clients.size()-1), border_width);
}

struct MainWindowLayout {
    Extends* ext;
    unsigned main = -1;
    std::vector<unsigned> left;
    std::vector<unsigned> right;
    unsigned active = INVALID;
    unsigned expanded = INVALID;
};

void remap(MainWindowLayout &layout) {
    if(layout.left.size() == 0 && layout.right.size() == 0) {
        if(layout.main == -1) return;
        clientSetDimensions(layout.main, layout.ext->x, layout.ext->y, layout.ext->width, layout.ext->height, border_width);
        return;
    }
    if(layout.right.size() == 0) {
        int split = layout.ext->width/layout.ext->height > 2 ? layout.ext->width / 4 : layout.ext->width /2;
        mapVector({layout.ext->x, layout.ext->y, split, layout.ext->height}, layout.left);
        clientSetDimensions(layout.main, layout.ext->x + split, layout.ext->y, layout.ext->width - split, layout.ext->height, border_width);
        return;
    }
    if(layout.left.size() == 0) {
        int split = layout.ext->width/layout.ext->height > 2 ? layout.ext->width / 4 : layout.ext->width /2;
        mapVector({layout.ext->x + layout.ext->width - split, layout.ext->y, split, layout.ext->height}, layout.right);
        clientSetDimensions(layout.main, layout.ext->x, layout.ext->y, layout.ext->width - split, layout.ext->height, border_width);
        return;
    }
    int split = layout.ext->width/4;
    mapVector({layout.ext->x, layout.ext->y, split, layout.ext->height}, layout.left);
    mapVector({layout.ext->x + layout.ext-> width - split, layout.ext->y, split, layout.ext->height}, layout.right);
    clientSetDimensions(layout.main, layout.ext->x + split, layout.ext->y, layout.ext->width - 2*split, layout.ext->height, border_width);
}

void addClient(MainWindowLayout &layout, unsigned client) {
    if(layout.main == -1) layout.main = client;
    else if(layout.right.size() < layout.left.size()) layout.right.push_back(client);
    else layout.left.push_back(client);
    remap(layout);
}

void removeClient(MainWindowLayout &layout, unsigned client) {
    if(layout.main == client) {
        if(layout.left.size() != 0) {
            layout.main = layout.left[0];
            layout.left.erase(layout.left.begin());
        } else if(layout.right.size() != 0) {
            layout.main = layout.right[0];
            layout.right.erase(layout.right.begin());
        }
        else layout.main = INVALID;
        remap(layout);
        return;
    }
    auto itl = std::find(layout.left.begin(), layout.left.end(), client);
    if(itl != layout.left.end()) {
        layout.left.erase(itl);
        remap(layout);
        return;
    }
    auto itr = std::find(layout.right.begin(), layout.right.end(), client);
    if(itr != layout.right.end()) {
        layout.right.erase(itr);
        remap(layout);
        return;
    }
    throw std::runtime_error("Client can not be removed: NOT FOUND");
}

void umapAll(MainWindowLayout &layout, unsigned client) {
    if( (layout.main != INVALID) && (layout.main != client) ) clientUnMap(layout.main);
    for(unsigned c : layout.left) if(c != client) clientUnMap(c);
    for(unsigned c : layout.right) if(c != client) clientUnMap(c);
}

void mapAll(MainWindowLayout &layout) {
    if(layout.main != INVALID) clientMap(layout.main);
    for(unsigned c : layout.left) clientMap(c);
    for(unsigned c : layout.right) clientMap(c);
}

void expand(MainWindowLayout &layout, unsigned client) {
    clientSetDimensions(client, layout.ext->x, layout.ext->y, layout.ext->width, layout.ext->height, border_width);
    umapAll(layout, client);
}

void uexpand(MainWindowLayout &layout) {
    mapAll(layout);
    remap(layout);
}


/*
        Layouts und monitors
*/

struct LayoutMonitor {
    Extends ext;
    MainWindowLayout layouts[9];
    unsigned current;
};

LayoutMonitor layoutMonitor;

void WM_setup(Extends extends) {
    layoutMonitor.ext = extends;
    for(unsigned i = 0; i < 9; i++) layoutMonitor.layouts[i].ext = &layoutMonitor.ext;
    layoutMonitor.current = 0;
}

#define LAYOUT (layoutMonitor.layouts[layoutMonitor.current])

void addClient(unsigned client) {
    //std::cout << "add begin: LAYOUT -  main: " << layout.main << ";  left size: " << layout.left.size() << "  right size: "  << layout.left.size() << std::endl;
    addClient(LAYOUT, client);
    //std::cout << "add end:   LAYOUT -  main: " << layout.main << ";  left size: " << layout.left.size() << "  right size: "  << layout.left.size() << std::endl;
}

void removeClient(unsigned client) {
    //std::cout << "remove begin:  LAYOUT -  main: " << layout.main << ";  left size: " << layout.left.size() << "  right size: "  << layout.left.size() << std::endl;
    removeClient(LAYOUT, client);
    //std::cout << "remove end:    LAYOUT -  main: " << layout.main << ";  left size: " << layout.left.size() << "  right size: "  << layout.left.size() << std::endl;
}

void toggle_expand(unsigned client) {
    if(LAYOUT.expanded == INVALID) {
        expand(LAYOUT, client);
        LAYOUT.expanded = client;
        return;
    }
    if(LAYOUT.expanded == client) {
        uexpand(LAYOUT);
        LAYOUT.expanded = INVALID;
        return;
    }
    uexpand(LAYOUT);
    expand(LAYOUT, client);
    LAYOUT.expanded = client;
}