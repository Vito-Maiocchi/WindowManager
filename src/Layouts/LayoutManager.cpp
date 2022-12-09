#include "LayoutManager.h"
#include "layoutBasicHorizontal.h"

void LayoutManager::setDimensions(Dimensions dim) {
    LayoutManager::dim = dim;
}

Dimensions LayoutManager::getDimensions() {
    return LayoutManager::dim;
}

LayoutManager getLayoutManager(Dimensions dim, Layout layout) {
    LayoutManager lm;

    switch(layout) {
        case BASIC_HORIZONTAL:
            lm = layoutBasicHorizontal();
        break;
    }

    lm.setDimensions(dim);

}

void changeLayout(LayoutManager &layoutManager, Layout layout) {
    LayoutManager lm;

    switch(layout) {
        case BASIC_HORIZONTAL:
            lm = layoutBasicHorizontal();
        break;
    }

    lm.setDimensions(layoutManager.getDimensions());
    lm.setClients(layoutManager.getClients());
    
    layoutManager = lm;
}