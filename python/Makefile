INCLUDES := -I.. -I../lib -I/usr/include/python2.3
CPPFLAGS = $(DEFS) $(INCLUDES)
CFLAGS = $(CPPFLAGS) -g -Wall
COMPILE = $(CC) $(CFLAGS)
LIBS = -L../lib -lvserver
LINK = $(CC) $(LDFLAGS)

NATIVE_MODS := vserverimpl.so

all: $(NATIVE_MODS)

$(NATIVE_MODS): %.so: %.o
	$(LINK) -shared -o $@ $^ $(LIBS)

%.o: %.c
	$(COMPILE) -c $<
