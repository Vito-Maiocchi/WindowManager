#g++ src/main.c src/titlebar.c -o WindowManager `pkg-config --cflags --libs xcb xcb-randr xcb-keysyms xft x11 x11-xcb` -I/usr/include/freetype2
g++ src/*.c src/*.cpp -o WindowManager `pkg-config --cflags --libs xcb xcb-randr xcb-keysyms xft x11 x11-xcb` -I/usr/include/freetype2
# g++ src/*.c src/WindowManager/*.cpp -o WindowManager `pkg-config --cflags --libs xcb xcb-randr xcb-keysyms xft x11 x11-xcb` -I/usr/include/freetype2
 # cp WindowManager /home/vito/VM-HomeDirectory/
DISPLAY=:1 ./WindowManager