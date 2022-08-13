CC=gcc
INC_PATH=-I.
LIB_PATH=#-L/home/dev_tools/apr/lib
LIBS=#-lapr-1 -laprutil-1
CFLAGS=-Wall -Wextra -O3 $(INC_PATH)
MAGICKFLAGS=$(shell MagickWand-config --cflags --ldflags)

.PHONY: all
all: main

.PHONY: run
run: main
	./main

main: main.c
	$(info $(CC) $(CFLAGS) $(MAGICKFLAGS) -lm -o $@ $@.c SCREENSHOTS $(LIB_PATH) $LIBS)
	@$(CC) $(CFLAGS) $(MAGICKFLAGS) -lm -o $@ $@.c $(LIB_PATH) $(LIBS)

.PHONY: clean
clean:
	rm -f main map.h map.c
	rm -rf Merged\ Screenshots Merged_Screenshots_C
