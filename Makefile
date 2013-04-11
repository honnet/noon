PROJECT = noon
APP_DIR = sifteo
APP     = $(APP_DIR)/$(PROJECT).elf
TARGET  = $(PROJECT)
CFLAGS  = -Wall # -O2

USING   = # "C++"
ifeq ($(USING), "C++")
	EXE = ./$(TARGET)
else
	EXE = python rtmidi_test.py
	TARGET =
endif


BUILD_PLATFORM := $(shell uname)

ifeq ($(BUILD_PLATFORM), Linux)
	CC      = g++
	CFLAGS += -D__LINUX_ALSA__
	LFLAGS  = -lasound -lpthread -lrtmidi
else
	ifeq ($(BUILD_PLATFORM), Darwin) # TODO test it!
		CC      = clang++
		CFLAGS += -D__MACOSX_CORE__
		LFLAGS  = -framework CoreMIDI -framework CoreAudio -framework CoreFoundation
	else
	#ifeq ($(BUILD_PLATFORM), windows32) # TODO test it!
		CC      = g++
		CFLAGS +=
		LFLAGS  =
	endif
endif

.PHONY: all install test live clean xclean

all: test

$(TARGET): *.cpp
	$(CC) *.cpp $(CFLAGS) $(LFLAGS) -o $(TARGET)

$(APP):
	make -C $(APP_DIR)

install: $(APP)
	make -C $(APP_DIR) install

test: $(APP) $(TARGET)
	siftulator $(APP) --flush-logs | $(EXE)

live: $(TARGET)
	swiss listen $(APP) --flush-logs | $(EXE)

clean:
	rm -f $(TARGET) *.o

xclean: clean
	make -C $(APP_DIR) clean

