#include "X_Connection.h"
#include "Util.h"

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <array>

unsigned border_width = 2;

#define INVALID -1

void mapVector(Extends ext, std::vector<unsigned> &clients) {
    if(clients.size() == 0) throw std::runtime_error("no clients in vector");
    unsigned h = ext.height / clients.size();
    for(unsigned i = 0; i < clients.size()-1; i++) clientSetDimensions(clients[i], ext.x, ext.y + i*h, ext.width, h, border_width);
    clientSetDimensions(*(clients.end()-1), ext.x, ext.y + h*(clients.size()-1), ext.width, ext.height- h*(clients.size()-1), border_width);
}

struct MainWindowLayout {
    Extends* ext;
    unsigned main = INVALID;
    std::vector<unsigned> left;
    std::vector<unsigned> right;
    unsigned active = INVALID;
    unsigned expanded = INVALID;
};

void remap(MainWindowLayout &layout) {
    if(layout.left.size() == 0 && layout.right.size() == 0) {
        if(layout.main == INVALID) return;
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
    if(layout.main == INVALID) layout.main = client;
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

void umapAll(MainWindowLayout &layout) {
    umapAll(layout, INVALID);
}

void mapAll(MainWindowLayout &layout) {
    if(layout.expanded != INVALID) {
        clientMap(layout.expanded);
        return;
    }
    if(layout.main != INVALID) clientMap(layout.main);
    for(unsigned c : layout.left) clientMap(c);
    for(unsigned c : layout.right) clientMap(c);
}

void expand(MainWindowLayout &layout, unsigned client) {
    clientSetDimensions(client, layout.ext->x, layout.ext->y, layout.ext->width, layout.ext->height, border_width);
    umapAll(layout, client);
}

void uexpand(MainWindowLayout &layout) {
    layout.expanded = INVALID;
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

std::vector<LayoutMonitor> layoutMonitors;
static unsigned currentMonitor;

void wmSetup(std::vector<Extends> exts, unsigned borderWidth) {
    border_width = borderWidth;
    layoutMonitors.resize(exts.size());
    for(unsigned i = 0; i < exts.size(); i++) {
        layoutMonitors[i].ext = exts[i];
        for(unsigned j = 0; j < 9; j++) layoutMonitors[i].layouts[j].ext = &layoutMonitors[i].ext;
        layoutMonitors[i].current = 0;
    }
    currentMonitor = 0;
}

#define LAYOUT_MONITOR layoutMonitors[currentMonitor]
#define LAYOUT LAYOUT_MONITOR.layouts[LAYOUT_MONITOR.current]

void wmAddClient(unsigned client) {
    addClient(LAYOUT, client);
}

void wmRemoveClient(unsigned client) {
    if(client == LAYOUT.expanded) {
        uexpand(LAYOUT);
        LAYOUT.expanded = INVALID;
    }
    removeClient(LAYOUT, client);
}

void wmToggleExpand(unsigned client) {
    if(!clientIsValid(client)) return;
    if(LAYOUT.expanded == INVALID) {
        expand(LAYOUT, client);
        LAYOUT.expanded = client;
        return;
    }
    if(LAYOUT.expanded == client) {
        uexpand(LAYOUT);
        return;
    }
    uexpand(LAYOUT);
    expand(LAYOUT, client);
    LAYOUT.expanded = client;
}

void wmSetTag(unsigned tag) {
    if(tag == LAYOUT_MONITOR.current) return;
    unsigned old = LAYOUT_MONITOR.current;
    LAYOUT_MONITOR.current = tag;
    mapAll(LAYOUT_MONITOR.layouts[tag]);
    umapAll(LAYOUT_MONITOR.layouts[old]);
}

/*
    PROBLEM:
    executbale gaht selebr zue
*/
