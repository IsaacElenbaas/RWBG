CC=gcc
INC_PATH=-I. -I$(MAKECMDGOALS)
LIB_PATH=#-L/home/dev_tools/apr/lib
LIBS=#-lapr-1 -laprutil-1
CFLAGS=-Wall -Wextra -O3 -mcmodel=medium -mlarge-data-threshold=1023 $(INC_PATH)
MAGICKFLAGS=$(shell MagickWand-config --cflags --ldflags)

ifneq ($(MAKECMDGOALS),clean)
ifeq ($(MAKECMDGOALS),)
MAKECMDGOALS=RW
endif
ifeq ($(MAKECMDGOALS),all)
MAKECMDGOALS=RW
endif
WORLD_DEF=
ifeq ($(MAKECMDGOALS),RW)
WORLD_DEF=-DRW
endif
ifeq ($(MAKECMDGOALS),ESA)
WORLD_DEF=-DESA
endif
$(info $(shell $(MAKE) -C $(MAKECMDGOALS) run))
$(info Done generating map)
SOURCES=$(wildcard ./$(MAKECMDGOALS)/Merged_Screenshots_C/*.c) $(wildcard ./$(MAKECMDGOALS)/Merged_Screenshots_C/*/*/*.c)
OBJECTS=$(patsubst %.c, %.o, $(SOURCES))
endif

.PHONY: all
all: main

.PHONY: RW
RW: main
.PHONY: ESA
ESA: main

# won't build for different game without phony
.PHONY: main
main: main.c $(OBJECTS)
	$(info $(CC) $(CFLAGS) $(WORLD_DEF) $(MAGICKFLAGS) -lm -o $@ $@.c SCREENSHOTS $(LIB_PATH) $LIBS)
	@$(CC) $(CFLAGS) $(WORLD_DEF) $(MAGICKFLAGS) -lm -o $@ $@.c $(OBJECTS) $(LIB_PATH) $(LIBS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LIB_PATH) $(LIBS)

.PHONY: clean
clean:
	rm -f main
	for game in RW ESA; do \
		$(MAKE) -C $$game $(MAKECMDGOALS); \
	done
