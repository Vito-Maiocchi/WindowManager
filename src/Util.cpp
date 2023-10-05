#include "Util.h"

Extends extendsModify(Extends ext, int pixels, Direction direction) {
    switch(direction) {
        case UP:
        return {
            ext.x,
            ext.y - pixels,
            ext.width,
            ext.height + pixels
        };
        case DOWN:
        return {
            ext.x,
            ext.y,
            ext.width,
            ext.height + pixels
        };
        case LEFT:
        return {
            ext.x - pixels,
            ext.y,
            ext.width + pixels,
            ext.height
        };
        case RIGHT:
        return {
            ext.x,
            ext.y,
            ext.width + pixels,
            ext.height
        };
    }
    return ext;
}