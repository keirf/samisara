OBJS += core.o

OBJS-$(at32f4) += hw_dwc_otg.o
OBJS-$(at32f4) += hw_usbd_at32f4.o
OBJS-$(at32f4) += hw_at32f4.o

SUBDIRS += hid

$(OBJS) $(OBJS-y): CFLAGS += -include $(SRCDIR)/defs.h
