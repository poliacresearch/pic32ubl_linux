LDFLAGS += -Wall -g
CFLAGS += -Wall -g
CXXFLAGS += -std=c++11 -Wall -g
CXXOBJS = main.o bootloader.o hexmanager.o
COBJS = hid.o
OBJS = $(COBJS) $(CXXOBJS)
INCLUDES += -I/usr/include/libusb-1.0
LIBS_UDEV = -lpthread -lusb-1.0 -ludev
BIN = blapp

all: $(BIN)

$(BIN): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LIBS_UDEV) -o $@

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@

$(CXXOBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(INCLUDES) $< -o $@

install:
	install -d $(PREFIX)$(DESTDIR)
	install -m 500 $(BIN) $(PREFIX)$(DESTDIR)

uninstall:
	rm -rf $(PREFIX)$(DESTDIR)

clean:
	rm -rf $(OBJS) $(BIN)

.PHONY: all clean install uninstall
