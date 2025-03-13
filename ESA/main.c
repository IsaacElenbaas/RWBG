#include <dirent.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "MagickWand/MagickWand.h"

/*{{{ structs and variables*/
struct Region;
typedef struct Region {
	int index;
	int x;
	int y;
	int w;
	int h;
	PixelWand* color;
	MagickWand* wand;
	struct Region* next;
} Region;

struct Screen;
typedef struct Connection {
	struct Screen* screen;
	int direction;
} Connection;

typedef struct Screen {
	int index;
	int x_scrot;
	int y_scrot;
	Region* region;
	short unsigned int connections;
} Screen;
int screens_length = 0;

int w = 32;
int h = 25;
int x_offset_map = 32;
int y_offset_map = 32;
int w_map_tile = 30;
int h_map_tile = 30;
int x_offset = 250;
int y_offset = 217;
int w_tile = 320;
int h_tile = 240;
int region_index = -1;
Region* region_first = NULL;
MagickWand* map_wand;
PixelWand* border;
PixelWand* blank;
PixelWand* color;
PixelWand* color2;
/*}}}*/

/*{{{ bool exists_override(int x, int y, Region* region)*/
bool exists_override(int x, int y, Region* region) {
	int tile_region_index = (region != NULL) ? region->index : region_index+1;
	if(tile_region_index == 5 && y >= 18) return false; // Research Outpost
	if(tile_region_index == 9 && x <= 10) return false; // hidden room with Repair Point at the left of the Depths and hidden rooms with Spirit Orb below the Sandrock Sector
	const int exists_overrides[][4] = {

/*{{{*/
		{21, 22, 0, 1}, // hidden rooms at upper left of The Temple
		{27, 27, 0, 0}, // hidden room (purple dot) at top of The Temple
		{8, 9, 6, 7}, // Land of Friendship
		{10, 10, 6, 6}, // hidden room next to Land of Friendship
		{4, 5, 8, 8}, // hidden rooms with teleporter at upper right of the Underwater Sector
		{0, 0, 19, 20}, // hidden rooms at bottom left of the Underwater Sector
		{29, 29, 8, 8}, // hidden room with Spirit Orb at upper right of the Volcanic Sector
		{29, 29, 8, 8}, // hidden room to the right of the Volcanic Sector
		{14, 14, 21, 21}, // hidden room (purple dot) at bottom-left-ish of the Depths
		{24, 24, 19, 19}, // hidden room at upper left of the Derelict Ship
		{0, 0, 0, 0}
/*}}}*/

	};
	for(int i = 0; exists_overrides[i][0] != 0 || exists_overrides[i][2] != 0; i++) {
		if(exists_overrides[i][0] <= x && x <= exists_overrides[i][1] &&
		   exists_overrides[i][2] <= y && y <= exists_overrides[i][3]) {
			return false;
		}
	}
	return true;
}
/*}}}*/

/*{{{ void region_override(int x, int y, Region** region, PixelWand* region_color)*/
void region_override(int x, int y, Region** region, PixelWand* region_color) {
	int region_override_index = -1;
	if((x == 22) &&
	   (y == 0)
	) {
		region_override_index = region_index+1;
		ClearPixelWand(region_color);
		MagickGetImagePixelColor(map_wand, x_offset_map+(x+1)*w_map_tile+1, y_offset_map+(y+1)*h_map_tile-3, region_color);
	}
	const int region_overrides[][5] = {

/*{{{*/
		{24, 24, 1,  1,  1}, // boss room that's hard to match region for
		{8,  8,  2,  2,  0}, // boss room that's hard to match region for
		{8,  10, 6,  7,  2}, // Land of Friendship and hidden room next to it
		{3,  3,  8,  8,  4}, // boss room that's hard to match region for
		{17, 19, 9,  9,  3}, // boss room that's a different color
		{28, 28, 10, 10, 5}, // boss room that's hard to match region for
		{30, 30, 14, 14, 5}, // room that's a different color
		{15, 15, 16, 16, 8}, // first room seen of the Depths, see region_override_post
		{25, 25, 16, 16, 5}, // boss room that's hard to match region for
		{25, 25, 24, 24, 5}, // room that's a different color
		{0, 0, 0, 0, 0}
/*}}}*/

	};
	for(int i = 0; region_overrides[i][0] != 0 || region_overrides[i][2] != 0; i++) {
		if(region_overrides[i][0] <= x && x <= region_overrides[i][1] &&
		   region_overrides[i][2] <= y && y <= region_overrides[i][3]) {
			region_override_index = region_overrides[i][4];
			break;
		}
	}
	if(region_override_index == -1) return;
	*region = region_first;
	for(int i = 0; i < region_override_index; i++) {
		*region = (*region)->next;
	}
}
/*}}}*/

/*{{{ void region_override_post(int x, int y, Region** region)*/
void region_override_post(int x, int y, Region** region) {
	int region_override_index = -1;
	switch((*region)->index) {
		case 6: // stop non-heated rooms in the Volcanic Sector from becoming their own region
			region_override_index = 5;
			break;
		case 3: // stop the Depths from merging with Security
			if(y >= 16) region_override_index = 8;
			break;
	}
	if(region_override_index == -1) return;
	*region = region_first;
	for(int i = 0; i < region_override_index; i++) {
		*region = (*region)->next;
	}
}
/*}}}*/

/*{{{ unsigned short int get_connections(int x, int y)*/
unsigned short int get_connections(int x, int y) {
	unsigned short int connections = 0;
	// see get_region for border pixels
	MagickGetImagePixelColor(map_wand, x_offset_map, y_offset_map, color);
	// right
	MagickGetImagePixelColor(map_wand, x_offset_map+(x+1)*w_map_tile-2, y_offset_map+(y+0.5)*h_map_tile, color2);
	if(IsPixelWandSimilar(color, color2, 0) == MagickFalse) connections |= 1;
	ClearPixelWand(color2);
	// up
	MagickGetImagePixelColor(map_wand, x_offset_map+(x+0.5)*w_map_tile, y_offset_map+y*h_map_tile, color2);
	if(IsPixelWandSimilar(color, color2, 0) == MagickFalse) connections |= 1 << 1;
	ClearPixelWand(color2);
	// left
	MagickGetImagePixelColor(map_wand, x_offset_map+x*w_map_tile, y_offset_map+(y+0.5)*h_map_tile, color2);
	if(IsPixelWandSimilar(color, color2, 0) == MagickFalse) connections |= 1 << 2;
	ClearPixelWand(color2);
	// down
	MagickGetImagePixelColor(map_wand, x_offset_map+(x+0.5)*w_map_tile, y_offset_map+(y+1)*h_map_tile-2, color2);
	if(IsPixelWandSimilar(color, color2, 0) == MagickFalse) connections |= 1 << 3;
	ClearPixelWand(color2);
	ClearPixelWand(color);
	return connections;
}
/*}}}*/

/*{{{ Region* get_region(int x, int y, PixelWand* region_color)*/
Region* get_region(int x, int y, PixelWand* region_color) {
	// -1 and -2 are border (one border on top and left and two on right and bottom)
	MagickGetImagePixelColor(map_wand, x_offset_map+x*w_map_tile+1, y_offset_map+(y+1)*h_map_tile-3, color);
	MagickGetImagePixelColor(map_wand, x_offset_map+(x+1)*w_map_tile-3, y_offset_map+(y+1)*h_map_tile-3, color2);
	if(IsPixelWandSimilar(color, color2, 0) == MagickFalse) {
		ClearPixelWand(color);
		MagickGetImagePixelColor(map_wand, x_offset_map+x*w_map_tile+(int)(1/2*(w_map_tile-3))+1, y_offset_map+(y+1)*h_map_tile-3, color);
	}
	PixelSetColorFromWand(region_color, color);
	Region* region;
	for(region = region_first; region != NULL; region = region->next) {
		if(IsPixelWandSimilar(region_color, region->color, 0) == MagickTrue) break;
	}
	region_override(x, y, &region, region_color);
	ClearPixelWand(color);
	ClearPixelWand(color2);
	return region;
}
/*}}}*/

int main() {
	Screen* screens[w][h];
	for(int x = 0; x < w; x++) {
		for(int y = 0; y < h; y++) {
			screens[x][y] = NULL;
		}
	}
	int screen_index = 0;
	MagickWandGenesis();
	PixelWand* letterbox_wand = NewPixelWand();
	PixelSetColor(letterbox_wand, "#ff0000");
	map_wand = NewMagickWand();
	MagickReadImage(map_wand, "map.png");
	MagickWand* world_wand = NewMagickWand();
	MagickReadImage(world_wand, "world.png");
	MagickWand* tile_wand = NewMagickWand();
	border = NewPixelWand();
	MagickGetImagePixelColor(map_wand, x_offset_map, y_offset_map, border);
	blank = NewPixelWand();
	MagickGetImagePixelColor(map_wand, x_offset_map+1, y_offset_map+1, blank);
	color = NewPixelWand();
	color2 = NewPixelWand();
	PixelWand* region_color = NewPixelWand();
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			MagickGetImagePixelColor(map_wand, x_offset_map+x*w_map_tile+1, y_offset_map+y*h_map_tile+1, color);
			MagickGetImagePixelColor(map_wand, x_offset_map+x*w_map_tile+1, y_offset_map+(y+1)*h_map_tile-3, color2);
			if(IsPixelWandSimilar(color, blank, 0) == MagickTrue && IsPixelWandSimilar(color2, blank, 0) == MagickTrue) {
				ClearPixelWand(color);
				continue;
			}
			ClearPixelWand(color);
			Region* region = get_region(x, y, region_color);
			if(!exists_override(x, y, region)) {
				ClearPixelWand(region_color);
				continue;
			}
			if(region == NULL) {

/*{{{ create new region*/
				int index = 0;
				if(region_first == NULL) {
					region_first = malloc(sizeof(Region));
					region = region_first;
				}
				else {
					index++;
					for(region = region_first; region->next != NULL; region = region->next) {
						index++;
					}
					region->next = malloc(sizeof(Region));
					region = region->next;
				}
				region->index = index;
				region->x = x;
				region->y = y;
				region->w = 0;
				region->h = 0;
				region->color = region_color;
				region_color = NewPixelWand();
				region->wand = NewMagickWand();
				MagickNewImage(region->wand, 1, 1, letterbox_wand);
				MagickSetImageBackgroundColor(region->wand, letterbox_wand);
				region->next = NULL;
				region_index++;
/*}}}*/

			}
			region_override_post(x, y, &region);
			screens_length++;
			screens[x][y] = malloc(sizeof(Screen));
			screens[x][y]->index = screen_index;
			screens[x][y]->region = region;
			screens[x][y]->connections = get_connections(x, y);
			screen_index++;
			if(x < region->x || y < region->y ||
			   region->x+region->w < x+1 || region->y+region->h < y+1
			) {

/*{{{ expand region*/
				region->w =
					(x < region->x)
						? region->x+region->w-x
						: (region->x+region->w < x+1)
							? x+1-region->x
							: region->w
				;
				region->h =
					(y < region->y) // shouldn't happen but no harm
						? region->y+region->h-y
						: (region->y+region->h < y+1)
							? y+1-region->y
							: region->h
				;
				MagickExtentImage(region->wand,
					region->w*w_tile, region->h*h_tile,
					(x < region->x) ? (x-region->x)*w_tile : 0, (y < region->y) ? (y-region->y)*h_tile : 0
				);
				if(x < region->x) region->x = x;
				if(y < region->y) region->y = y;
/*}}}*/

			}
			MagickNewImage(tile_wand, w_tile, h_tile, letterbox_wand);
			MagickCompositeImage(tile_wand, world_wand, OverCompositeOp, MagickTrue, -(x_offset+x*w_tile), -(y_offset+y*h_tile));
			MagickCompositeImage(region->wand, tile_wand, OverCompositeOp, MagickTrue, (x-region->x)*w_tile, (y-region->y)*h_tile);
			MagickRemoveImage(tile_wand);
			ClearMagickWand(tile_wand);
		}
	}
	DestroyPixelWand(color);
	DestroyPixelWand(blank);
	DestroyMagickWand(tile_wand);
	DestroyMagickWand(world_wand);
	DestroyMagickWand(map_wand);
	DestroyPixelWand(letterbox_wand);

/*{{{ write images and generate c files*/
	mkdir("Merged Screenshots", 0777);
	mkdir("Merged_Screenshots_C", 0777);
	for(Region* region = region_first; region != NULL; region = region->next) {
		DestroyPixelWand(region->color);
		if(region->w != 0 && region->h != 0) {
			char out[strlen("Merged Screenshots/.png")+((int)log10(region->index|1)+1)+1];
			sprintf(out, "Merged Screenshots/%d.png", region->index);
			MagickWriteImage(region->wand, out);
			FILE* region_image = fopen(out, "r");
			fseek(region_image, 0L, SEEK_END);
			size_t region_image_length = ftell(region_image);
			fclose(region_image);
			sprintf(out, "Merged_Screenshots_C/%d.cpp", region->index);
			FILE* scrotc = fopen(out, "w");
			fprintf(scrotc, "#include \"map.h\"\n");
			fprintf(scrotc, "Screenshot r_%d;\n", region->index);
			fprintf(scrotc, "static char s[] = {\n");
			fprintf(scrotc, "#embed \"../Merged Screenshots/%d.png\"\n", region->index);
			fprintf(scrotc, "};\n");
			fprintf(scrotc, "void init_r_%d() {;\n", region->index);
			fprintf(scrotc, "\tr_%d.length = %zu;\n", region->index, region_image_length);
			// can't make unsigned, dumb #embed error due to in-range "negatives"
			fprintf(scrotc, "\tr_%d.blob = (unsigned char*)&s[0];\n", region->index);
			fprintf(scrotc, "}\n");
			fclose(scrotc);
		}
		DestroyMagickWand(region->wand);
	}
	MagickWandTerminus();
	FILE* mapc = fopen("map.c", "w");
	FILE* maph = fopen("map.h", "w");
	fprintf(maph, "#ifndef map_H\n");
	fprintf(maph, "#define map_H\n");
	fprintf(maph, "#include <stdlib.h>\n#include <string.h>\n");
	fprintf(maph, "\n");
	fprintf(maph, "typedef struct Screenshot {\n");
	fprintf(maph, "\tunsigned char* blob;\n");
	fprintf(maph, "\tint length;\n");
	fprintf(maph, "} Screenshot;\n");
	fprintf(maph, "\n");
	fprintf(mapc, "int w_scrot = %d;\n", w_tile);
	fprintf(mapc, "int w_scrot_extended = %d;\n", w_tile);
	fprintf(mapc, "int h_scrot = %d;\n", h_tile);
	fprintf(mapc, "\n");
	for(Region* region = region_first; region != NULL; region = region->next) {
		if(region->w != 0 && region->h != 0) {
			fprintf(maph, "extern Screenshot r_%d;\n", region->index);
			fprintf(maph, "void init_r_%d();\n", region->index);
			fprintf(mapc, "init_r_%d();\n", region->index);
		}
	}
	fprintf(maph, "#endif\n");
	fclose(maph);
	fprintf(mapc, "\n");
	fprintf(mapc, "const int screens_length = %d;\n", screens_length);
	fprintf(mapc, "Screen screens[screens_length];\n");
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			if(screens[x][y] != NULL) {
				fprintf(mapc, "\n");
				fprintf(mapc, "screens[%d].screenshot = &r_%d;\n", screens[x][y]->index, screens[x][y]->region->index);
				fprintf(mapc, "screens[%d].x_scrot = %d;\n", screens[x][y]->index, (x-screens[x][y]->region->x)*w_tile);
				fprintf(mapc, "screens[%d].y_scrot = %d;\n", screens[x][y]->index, (y-screens[x][y]->region->y)*h_tile);
				short unsigned int connections = screens[x][y]->connections;
				fprintf(mapc, "screens[%d].connections_length = %d;\n", screens[x][y]->index, (connections & 1)+((connections & (1 << 1)) >> 1)+((connections & (1 << 2)) >> 2)+((connections & (1 << 3)) >> 3));
				fprintf(mapc, "screens[%d].connections = (Connection*)malloc(screens[%d].connections_length*sizeof(Connection));\n", screens[x][y]->index, screens[x][y]->index);
				int i = 0;
				if((connections & 1)        != 0 && x+1  < w && screens[x+1][y] != NULL) {
					fprintf(mapc, "screens[%d].connections[%d].screen = &screens[%d];\n", screens[x][y]->index, i, screens[x+1][y]->index);
					fprintf(mapc, "screens[%d].connections[%d].direction = %d;\n", screens[x][y]->index, i, 1);
				i++; }
				if((connections & (1 << 1)) != 0 && y-1 >= 0 && screens[x][y-1] != NULL) {
					fprintf(mapc, "screens[%d].connections[%d].screen = &screens[%d];\n", screens[x][y]->index, i, screens[x][y-1]->index);
					fprintf(mapc, "screens[%d].connections[%d].direction = %d;\n", screens[x][y]->index, i, 1 << 1);
				i++; }
				if((connections & (1 << 2)) != 0 && x-1 >= 0 && screens[x-1][y] != NULL) {
					fprintf(mapc, "screens[%d].connections[%d].screen = &screens[%d];\n", screens[x][y]->index, i, screens[x-1][y]->index);
					fprintf(mapc, "screens[%d].connections[%d].direction = %d;\n", screens[x][y]->index, i, 1 << 2);
				i++; }
				if((connections & (1 << 3)) != 0 && y+1 < h && screens[x][y+1] != NULL) {
					fprintf(mapc, "screens[%d].connections[%d].screen = &screens[%d];\n", screens[x][y]->index, i, screens[x][y+1]->index);
					fprintf(mapc, "screens[%d].connections[%d].direction = %d;\n", screens[x][y]->index, i, 1 << 3);
				i++; }
			}
		}
	}
	fclose(mapc);
/*}}}*/

	return 0;
}
