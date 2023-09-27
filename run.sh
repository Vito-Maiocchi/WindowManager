#g++ src/main.cpp src/X_Connection.cpp src/util.c src/drw.c -o WindowManager `pkg-config --cflags --libs xcb xcb-randr xcb-keysyms xft x11 x11-xcb fontconfig` -I/usr/include/freetype2
#g++ src/*.c src/*.cpp src/Layouts/*.cpp -o WindowManager `pkg-config --cflags --libs xcb xcb-randr xcb-keysyms xft x11 x11-xcb` -I/usr/include/freetype2 -Isrc
#g++ src/*.c src/WindowManager/*.cpp -o WindowManager `pkg-config --cflags --libs xcb xcb-randr xcb-keysyms xft x11 x11-xcb` -I/usr/include/freetype2
# cp WindowManager /home/vito/VM-HomeDirectory/
DISPLAY=:1 ./WindowManager
