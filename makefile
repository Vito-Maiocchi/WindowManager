TARGET_FILENAME = VitoWM
SRC_DIR = src
BUILD_DIR = build

FLAGS = -std=c++20 `pkg-config --cflags --libs xcb xcb-randr xcb-cursor xcb-keysyms xft x11 x11-xcb fontconfig` -I/usr/include/freetype2
# TODO: irgend wenn mal mit -Wall alles chli schönner go mache

#for a more orthodox location use:
#TARGET = $(TARGET_FILENAME)
TARGET = ~/tmp/$(TARGET_FILENAME)

SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))
H_FILES	  = $(wildcard $(SRC_DIR)/*.h)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(H_FILES) | $(BUILD_DIR)
	g++ -o $@ -c $< $(FLAGS)

$(TARGET): $(OBJ_FILES)
	g++ -o $(TARGET) $^ $(FLAGS)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

install: $(TARGET)
	sudo cp $(TARGET) /usr/bin/VitoWM
	sudo chmod +x /usr/bin/VitoWM

clean:
	rm -f TARGET
	rm -r $(BUILD_DIR)

.PHONY: clean install