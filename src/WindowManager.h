#ifndef VITO_WM_WINDOWMANAGER_H
#define VITO_WM_WINDOWMANAGER_H

#include <vector>

struct Extends;

/*
void WM_setup(Extends extends);
void addClient(unsigned client);
void removeClient(unsigned client);
void toggle_expand(unsigned client);
void setTag(unsigned tag); */

void wmSetup(std::vector<Extends> exts, unsigned borderWidth);
void wmAddClient(unsigned client);
void wmRemoveClient(unsigned client);
void wmToggleExpand(unsigned client);
void wmSetTag(unsigned tag);

#endif