g++ src/*.cpp -o ~/tmp/WindowManager `pkg-config --cflags --libs xcb xcb-randr xcb-cursor xcb-keysyms xft x11 x11-xcb fontconfig` -I/usr/include/freetype2
sudo cp ~/tmp/WindowManager /usr/bin/VitoWM
sudo chmod +x /usr/bin/VitoWM