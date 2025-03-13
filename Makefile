CC=clang
INC_PATH=-I. -I$(MAKECMDGOALS)
LIB_PATH=#-L/home/dev_tools/apr/lib
LIBS=#-lapr-1 -laprutil-1
CFLAGS=-Wall -Wextra -O3 -std=c++23 $(INC_PATH) -mcmodel=medium -Wl,--no-relax
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
ifneq ($(MAKECMDGOALS),RWBG-test)
$(info $(shell $(MAKE) -C $(MAKECMDGOALS) run))
$(info Done generating map)
SOURCES=$(wildcard ./$(MAKECMDGOALS)/Merged_Screenshots_C/*.cpp) $(wildcard ./$(MAKECMDGOALS)/Merged_Screenshots_C/*/*/*.cpp)
OBJECTS=$(patsubst %.cpp, %.o, $(SOURCES))
endif
endif

.PHONY: all
all: main

.PHONY: RW
RW: main
.PHONY: ESA
ESA: main

# won't build for different game without phony
.PHONY: main
main: main.cpp $(OBJECTS)
	$(info $(CC) $(CFLAGS) $(WORLD_DEF) $(MAGICKFLAGS) -lm -o $@ $@.c SCREENSHOTS $(LIB_PATH) $LIBS)
	@$(CC) $(CFLAGS) $(WORLD_DEF) $(MAGICKFLAGS) -lm -o $@ $^ $(LIB_PATH) $(LIBS)

RWBG-test: test.c
	$(info $(CC) $(CFLAGS) $(WORLD_DEF) $(MAGICKFLAGS) -lm -o $@ $@.c SCREENSHOTS $(LIB_PATH) $LIBS)
	@$(CC) $(CFLAGS) $(WORLD_DEF) $(MAGICKFLAGS) -lm -o $@ $< $(LIB_PATH) $(LIBS)

$(OBJECTS): %.o : %.cpp
	$(CC) $(CFLAGS) -c -o $@ $< $(LIB_PATH) $(LIBS)

.PHONY: clean
clean:
	rm -f main RWBG-test
	for game in RW ESA; do \
		$(MAKE) -C $$game $(MAKECMDGOALS); \
	done
