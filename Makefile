TARGET  = noon
APP_DIR = sifteo
APP     = $(APP_DIR)/$(TARGET).elf
CFLAGS  = -Wall # -O2

BUILD_PLATFORM := $(shell uname)

ifeq ($(BUILD_PLATFORM), Linux)
	CC 		= g++
	CFLAGS += -D__LINUX_ALSA__
	LFLAGS  = -lasound -lpthread -lrtmidi
endif
ifeq ($(BUILD_PLATFORM), Darwin) # TODO test it!
	CC 		= clang++
	CFLAGS += -D__MACOSX_CORE__
	LFLAGS  = -framework CoreMIDI -framework CoreAudio -framework CoreFoundation
endif
ifeq ($(BUILD_PLATFORM), windows32) # TODO test it!
	CC 		= mingw32-gcc
	CFLAGS +=
	LFLAGS  =
endif

.PHONY: install test live clean xclean

$(TARGET): *.cpp
	$(CC) *.cpp $(CFLAGS) $(LFLAGS) -o $(TARGET)

$(APP):
	make -C $(APP_DIR)

install: $(APP)
	make -C $(APP_DIR) install

test: $(APP) $(TARGET)
	siftulator $(APP) --flush-logs | ./$(TARGET)

live: install $(TARGET)
	swiss listen $(APP) --flush-logs | ./$(TARGET)

clean:
	rm -f $(TARGET) *.o

xclean: clean
	make -C $(APP_DIR) clean

