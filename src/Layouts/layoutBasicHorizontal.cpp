#include "layoutBasicHorizontal.h"
#include "Xconnection.h"

void layoutBasicHorizontal::rebuild() {
    int size = clients.getSize();
    if(size == 0) return;
    const Dimensions dim = getDimensions();
    int width = dim.width / size;
    clientSetDimensions(clients[0], dim.x, dim.y, width + dim.width % size, dim.height);
    for(int i = 1; i < size; i++) clientSetDimensions(clients[i], dim.x + width*i, dim.y, width, dim.height);
}