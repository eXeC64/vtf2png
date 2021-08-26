.PHONY: clean

CFLAGS ?= -O2 -std=c99 -Wall -Wextra -Wstrict-prototypes -Wmissing-declarations -Wwrite-strings -Wshadow -Werror -Wpedantic -g
LDLIBS = -lpng

TARGET = vtf2png
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=%.o)

$(TARGET): $(OBJECTS)

clean:
	$(RM) $(TARGET) $(OBJECTS)
