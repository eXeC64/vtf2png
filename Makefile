.PHONY: clean

CFLAGS ?= -O2 -Wall -Werror -Wpedantic
LDLIBS = -lpng

TARGET = vtf2png
SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.c,%.o,$(SOURCES))

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(TARGET) $(OBJECTS)
