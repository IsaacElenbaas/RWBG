CC=gcc
INC_PATH=-I.
LIB_PATH=#-L/home/dev_tools/apr/lib
LIBS=#-lapr-1 -laprutil-1
CFLAGS=-Wall -Wextra -O3 $(INC_PATH)
MAGICKFLAGS=$(shell MagickWand-config --cflags --ldflags)

ifneq ($(MAKECMDGOALS),clean)
$(info $(shell python3 ./gen_map.py)Done generatng map)
SOURCES=$(wildcard Merged_Screenshots_C/*/*.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))
endif

all: main

main: main.c $(OBJECTS)
	$(info $(CC) $(CFLAGS) $(MAGICKFLAGS) -lm -o $@ $@.c SCREENSHOTS $(LIB_PATH) $LIBS)
	@$(CC) $(CFLAGS) $(MAGICKFLAGS) -lm -o $@ $@.c $(OBJECTS) $(LIB_PATH) $(LIBS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LIB_PATH) $(LIBS)

clean:
	rm -f main screenshots.h screenshots.c
	rm -rf Merged_Screenshots_C
