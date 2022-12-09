struct Dimensions {
    int x, y, width, height;
};

Dimensions getDimensions(int x, int y, int width, int height);

class uintList {
    int size = 0;
    unsigned int* elements = nullptr;
    public:
        ~uintList();
        bool append(unsigned int client);
        bool remove(unsigned int client);
        void setArray(unsigned int array[]);
        unsigned int* getArray();
        int getSize();
        int operator[](int i);
};