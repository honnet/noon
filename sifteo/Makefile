APP = noon

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += *.png $(ASSETS).lua

include $(SDK_DIR)/Makefile.rules

.PHONY: test live

test: all
	siftulator --flush-logs $(APP).elf | python ../rtmidi_test.py

live: install
	swiss listen $(APP).elf --flush-logs | python ../rtmidi_test.py

