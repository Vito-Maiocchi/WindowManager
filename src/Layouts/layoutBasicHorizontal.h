#include "BasicLayout.h"

class layoutBasicHorizontal: public LayoutManager {
    void rebuild();
    uintList clients; 
};