#ifndef VITO_WM_UTIL_H
#define VITO_WM_UTIL_H

struct Extends {
    int x;
    int y;
    int width;
    int height;
};

enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

Extends extendsModify(Extends ext, int pixels, Direction direction);

#endif