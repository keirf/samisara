OBJS += board.o
OBJS += build_info.o
OBJS += vectors.o
OBJS += string.o
OBJS += cortex.o
OBJS += time.o
OBJS += timer.o
OBJS += util.o
OBJS-$(debug) += console.o

OBJS += main.o
OBJS += keyboard.o

SUBDIRS += mcu usb

.PHONY: $(SRCDIR)/build_info.c
build_info.o: CFLAGS += -DFW_MAJOR=$(FW_MAJOR) -DFW_MINOR=$(FW_MINOR)

# Avoid infinite loops due to GCC noticing code that can be replaced by a call
# to a standard library function... within our implementation of that function.
util.o: CFLAGS += -fno-tree-loop-distribute-patterns
