#include "Util.h"

    //DIMENSIONS

Dimensions getDimensions(int x, int y, int width, int height) {
    Dimensions d;
    d.x = x;
    d.y = y;
    d.width = width;
    d.height = height;
}

    //UINT LIST

uintList::~uintList() {
    delete[] elements;
}

bool uintList::append(unsigned int client) {
    int index = 0;
    while (index < size) {
        if(elements[index] == client) break;
        index++;
    }
    if(index != size) return false; //allready contains client

    unsigned int* new_elements = new unsigned int[size + 1];
    for(int i = 0; i < size; i++) new_elements[i] = elements[i];
    new_elements[size] = client;
    size++;
    delete[] elements;
    elements = new_elements;
    return true;
}

bool uintList::remove(unsigned int client) {
    int index = 0;
    while (index < size) {
        if(elements[index] == client) break;
        index++;
    }
    if(index == size) return false;

    size--;
    unsigned int* new_elements = new unsigned int[size];
    for(int i = 0; i < size; i++) {
        if(i == index) elements++;
        new_elements[i] = elements[i];
    }
    delete[] (elements - 1);
    elements = new_elements;
    return true;
}

void uintList::setArray(unsigned int array[]) {
    size = sizeof(array) / sizeof(unsigned int);
    delete[] elements;
    elements = new unsigned int[size];
    for(int i = 0; i < size; i++) elements[i] = array[i]; 
}

unsigned int* uintList::getArray() {
    return elements;
}

int uintList::getSize() {
    return size;
}

int uintList::operator[](int i) {
    return elements[i];
}
