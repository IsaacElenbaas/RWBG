#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "MagickWand/MagickWand.h"
#include "map.h"
#include "main.h"
#ifdef __linux__
#	include <signal.h>
#	include <unistd.h>
#endif

/*{{{ structs and variables*/
struct Node;
typedef struct Node {
	struct Node* next;
	struct Node* prev;
	int dx;
	int dy;
	int direction;
	Screen* screen;
} Node;

struct Solution;
typedef struct Solution {
	struct Solution* next;
	Screen** solution;
} Solution;

bool** monitors;
int*** monitors_data;
bool** visited;
int x_max, y_max;
int x_origin, y_origin;
int x, y;
Node origin;
Node* current;
Solution* solution;
bool backtracking = false;
int backtracking_to_complete = 0;
Screen* last = NULL;
/*}}}*/

/*{{{ tryMoveDirection*/
	/*{{{ bool tryMove(int x_new, int y_new, int direction)*/
bool tryMove(int x_new, int y_new, int direction) {
	if(x_new < 0 || y_new < 0) return false;
	if(x_new > x_max || y_new > y_max) return false;
	if(!monitors[x_new][y_new]) return false;
	for(int i = 0; i < current->screen->connections_length; i++) {
		if(current->direction == direction && current->screen->connections[i].screen == last) {
			last = NULL;
			continue;
		}
		// down here so that ^ can happen when backtracking_to_complete
		if(visited[x_new][y_new]) continue;
		if(!(current->screen->connections[i].direction & direction)) continue;
		// if backtracking, disallow any connections until we've passed the one we just came from
		if(last == NULL) {

		/*{{{ check if screen is already being used because ones that can be in multiple directions cause issues*/
			bool in_use = false;
			for(Node* current_t = current; current_t != NULL; current_t = current_t->prev) {
				if(current_t->screen == current->screen->connections[i].screen) {
					in_use = true;
					break;
				}
			}
			if(in_use) continue;
		/*}}}*/

			backtracking = false;
			if(backtracking_to_complete == 0) {
				current->next = malloc(sizeof(Node));
				current->next->prev = current;
				current->next->screen = current->screen->connections[i].screen;
				current = current->next;
				current->dx = 0; current->dy = 0;
				current->direction = 0;
				current->prev->direction = direction;
			}
			else {
				backtracking_to_complete = 0;
				current->screen = current->screen->connections[i].screen;
			}
			current->next = NULL;
			current->dx += x_new-x; current->dy += y_new-y;
			x = x_new; y = y_new;
			visited[x][y] = true;
			return true;
		}
	}
	return false;
}
	/*}}}*/

bool tryMoveRight() { return tryMove(x+1, y, 1); }
bool tryMoveUp() { return tryMove(x, y-1, 2); }
bool tryMoveLeft() { return tryMove(x-1, y, 4); }
bool tryMoveDown() { return tryMove(x, y+1, 8); }
/*}}}*/

int main(int argc, char* argv[]) {
	// TODO: check argc and write help

#ifdef __linux__
	int output_PID = 0;
	if(strcmp(argv[1], "--daemon") == 0) {
		signal(SIGCHLD, SIG_IGN);
		while(true) {
			char* line = NULL;
			size_t line_size = 0;
			size_t line_length;
			line_length = getline(&line, &line_size, stdin);
			if(line_length <= 0) return 0;
			if(fork() == 0) {
				argv = malloc((2+9*9)*sizeof(char*));
				argv[1] = malloc((3+1)*sizeof(char));
				argv[2] = malloc((3+1)*sizeof(char));
				for(int i = 3; i < 2+9*9; i++) {
					argv[i] = malloc((5+3+2*5+2*4+1)*sizeof(char));
				}
				sscanf(line, "%d %s %s", &output_PID, argv[1], argv[2]);
				argc = 3;
				// +1s are for spaces this time instead of NULL terminators
				for(size_t i = (((int)log10(output_PID)+1)+1+2*(3+1))*sizeof(char); i < line_length; i += (strlen(argv[argc++])+1)*sizeof(char)) {
					sscanf(line+i, "%s", argv[argc]);
				}
				break;
			}
			free(line);
		}
	}
#endif

/*{{{ load monitors*/
	sscanf(argv[1], "%dx%d", &x_max, &y_max);
	x_max--; y_max--;
	sscanf(argv[2], "%d,%d", &x_origin, &y_origin);
	x = x_origin; y = y_origin;
	monitors = malloc((x_max+1)*sizeof(bool*));
	monitors_data = malloc((x_max+1)*sizeof(bool*));
	visited = malloc((x_max+1)*sizeof(bool*));
	for(int i = 0; i < x_max+1; i++) {
		monitors[i] = malloc((y_max+1)*sizeof(bool));
		monitors_data[i] = malloc((y_max+1)*sizeof(bool*));
		visited[i] = malloc((y_max+1)*sizeof(bool));
		for(int j = 0; j < y_max+1; j++) {
			monitors[i][j] = 0;
			visited[i][j] = 0;
		}
	}
	for(int i = 3; i < argc; i++) {
		int x_monitor, y_monitor;
		sscanf(argv[i], "%d,%d@%*d,%*d:%*dx%*d", &x_monitor, &y_monitor);
		monitors[x_monitor][y_monitor] = true;
		monitors_data[x_monitor][y_monitor] = malloc(4*sizeof(int));
		sscanf(argv[i], "%*d,%*d@%d,%d:%dx%d",
			&monitors_data[x_monitor][y_monitor][0],
			&monitors_data[x_monitor][y_monitor][1],
			&monitors_data[x_monitor][y_monitor][2],
			&monitors_data[x_monitor][y_monitor][3]
		);
	}
/*}}}*/

	#include "map.c"
	origin.next = NULL;
	origin.prev = NULL;
	current = &origin;
	visited[x_origin][y_origin] = true;
	srand(time(0));
	bool tried[screens_length];
	for(int i = 0; i < screens_length; i++) tried[i] = false;
	for(int attempt = 0; attempt < screens_length; attempt++) {
		int screen;
		for(screen = rand()%screens_length; tried[screen]; screen = (screen+1)%screens_length);
		tried[screen] = true;
		origin.screen = &screens[screen];

/*{{{ generate solution list starting at screen*/
		while(true) {
			if(!(tryMoveRight() || tryMoveUp() || tryMoveLeft() || tryMoveDown())) {
				if(current->screen == origin.screen) {
					// end condition, backtracked from final solution to origin
					// (or didn't have anywhere to go in the first place)
					if(current->prev == NULL) {
						// if there's only one monitor anything is a solution
						if(x_max == 0 && y_max == 0) {
							solution = malloc(sizeof(Solution));
							solution->next = NULL;
							solution->solution = malloc(sizeof(Screen*));
							solution->solution[0] = current->screen;
						}
						// this isn't necessary because it happens in tryMove but _to_complete is
						//backtracking = false;
						backtracking_to_complete = 0;
						break;
					}

	/*{{{ filled all possible, check for solution and start backtracking*/
					bool is_solution = true;
					for(int i = 0; i <= x_max; i++) {
						for(int j = 0; j <= y_max; j++) {
							if(monitors[i][j] && !visited[i][j]) is_solution = false;
						}
					}
					if(is_solution) {

		/*{{{ store solution*/
						Solution* solution_t = malloc(sizeof(Solution));
						solution_t->next = NULL;
						if(solution == NULL) solution = solution_t;
						else {
							Solution* i;
							for(i = solution; i->next != NULL; i = solution->next);
							i->next = solution_t;
						}
						solution_t->solution = malloc((x_max+1)*(y_max+1)*sizeof(Screen*));
						int x_t = x; int y_t = y;
						for(Node* current_t = current; current_t != NULL; current_t = current_t->prev) {
							solution_t->solution[x_t*(x_max+1)+y_t] = current_t->screen;
							x_t -= current_t->dx; y_t -= current_t->dy;
						}
		/*}}}*/

					}
	/*}}}*/

					backtracking = true;
					backtracking_to_complete = 0;
				}
				// nothing else down this branch, go to head to try to fill in the rest of the monitors
				else if(!backtracking) {
					last = current->screen;
					if(backtracking_to_complete == 0) {
						backtracking_to_complete = 1;
						current->next = malloc(sizeof(Node));
						current->next->prev = current;
						current->next->screen = current->prev->screen;
						current = current->next;
						current->next = NULL;
						current->dx = -current->prev->dx; current->dy = -current->prev->dy;
						current->prev->direction = 0;
						current->direction = current->prev->prev->direction;
						x -= current->prev->dx; y -= current->prev->dy;
						continue;
					}
					backtracking_to_complete++;
					Node* current_t = current->prev;
					for(int i = 0; i < backtracking_to_complete; i++) current_t = current_t->prev;
					current->screen = current_t->screen;
					current->dx -= current_t->next->dx; current->dy -= current_t->next->dy;
					current->direction = current_t->direction;
					x -= current_t->next->dx; y -= current_t->next->dy;
					continue;
				}
				// backtrack one
				last = current->screen;
				visited[x][y] = false;
				visited[x_origin][y_origin] = true;
				x = x-current->dx; y = y-current->dy;
				current = current->prev;
				free(current->next);
				current->next = NULL;
			}
		}
/*}}}*/

		if(solution != NULL) break;
	}
	if(solution == NULL) {
#ifdef __linux__
		if(output_PID == 0)
#endif
			printf("There aren't any map sections that match this monitor configuration.\n");
#ifdef __linux__
		else kill(output_PID, SIGINT);
#endif
		return 0;
	}
	int solution_num = 1;
	for(Solution* solution_t = solution; solution_t->next != NULL; solution_t = solution_t->next) {
		solution_num++;
	}
	solution_num = rand()%solution_num;
	for(int i = 0; i < solution_num; i++) {
		solution = solution->next;
	}

/*{{{ write background*/
	MagickWandGenesis();
	MagickWand* background_wand = NewMagickWand();
	PixelWand* letterbox_wand = NewPixelWand();
	PixelSetColor(letterbox_wand, "#000000");
	MagickNewImage(background_wand, 1, 1, letterbox_wand);
	MagickSetImageBackgroundColor(background_wand, letterbox_wand);
	for(int i = 0; i <= x_max; i++) {
		for(int j = 0; j <= y_max; j++) {
			if(!monitors[i][j]) continue;

	/*{{{ expand background to fit screen*/
			if(MagickGetImageWidth(background_wand) < (unsigned int)(monitors_data[i][j][0]+monitors_data[i][j][2])) {
				MagickExtentImage(background_wand, monitors_data[i][j][0]+monitors_data[i][j][2], MagickGetImageHeight(background_wand), 0, 0);
			}
			if(MagickGetImageHeight(background_wand) < (unsigned int)(monitors_data[i][j][1]+monitors_data[i][j][3])) {
				MagickExtentImage(background_wand, MagickGetImageWidth(background_wand), monitors_data[i][j][1]+monitors_data[i][j][3], 0, 0);
			}
	/*}}}*/

			Screen* screen = solution->solution[i*(x_max+1)+j];
			MagickWand* screen_wand = NewMagickWand();
			MagickReadImageBlob(screen_wand, screen->screenshot->blob, screen->screenshot->length);
			MagickSetImageBackgroundColor(screen_wand, letterbox_wand);
			int w = 1024;
			int h = 768;
			int x_scrot = screen->x_scrot;
			int y_scrot = screen->y_scrot;

	/*{{{ if the monitor is widescreen and not using a multiscreen room, widescreen*/
			int w_letterboxed = 1366-w;
			if(w_letterboxed > w/2) w_letterboxed = 1366-1024;
			if(((double)monitors_data[i][j][2])/monitors_data[i][j][3] > ((double)w+w_letterboxed/2)/h) {
				// the cropping and extending is to make rooms that are letterboxed on one side not centered (end of long rooms)
				if(i-1 < 0 || !monitors[i-1][j] || screen->screenshot->blob != solution->solution[(i-1)*(x_max+1)+j]->screenshot->blob) {
					w += w_letterboxed/2;
					x_scrot -= w_letterboxed/2;
				}
				if(i+1 > x_max || !monitors[i+1][j] || screen->screenshot->blob != solution->solution[(i+1)*(x_max+1)+j]->screenshot->blob) {
					w += w_letterboxed/2;
					MagickCropImage(screen_wand, w, h, x_scrot, y_scrot);
					if(w != 1366) {
						MagickExtentImage(screen_wand, 1366, h, -w_letterboxed/2, 0);
						w = 1366;
					}
				}
				else {
					MagickCropImage(screen_wand, w, h, x_scrot, y_scrot);
					if(w == 1024+w_letterboxed/2) {
						MagickExtentImage(screen_wand, 1366, h, 0, 0);
						w = 1366;
					}
				}
			}
	/*}}}*/

			else MagickCropImage(screen_wand, w, h, x_scrot, y_scrot);
			// make canvas match monitor ratio so resize goes well
			if(((double)monitors_data[i][j][2])/w > ((double)monitors_data[i][j][3])/h) {
				int w_new = (h*monitors_data[i][j][2])/monitors_data[i][j][3];
				MagickExtentImage(screen_wand, w_new, h, -(w_new-w)/2, 0);
			}
			else {
				int h_new = (w*monitors_data[i][j][3])/monitors_data[i][j][2];
				MagickExtentImage(screen_wand, w, h_new, 0, -(h_new-h)/2);
			}
			// Lanczos is good as well
			MagickResizeImage(screen_wand, monitors_data[i][j][2], monitors_data[i][j][3], BlackmanFilter);
			MagickCompositeImage(background_wand, screen_wand, OverCompositeOp, MagickTrue, monitors_data[i][j][0], monitors_data[i][j][1]);
			DestroyMagickWand(screen_wand);
		}
	}
	MagickSetImageCompressionQuality(background_wand, 100);
#ifdef __linux__
	if(output_PID == 0)
#endif
		MagickWriteImage(background_wand, "background.png");
#ifdef __linux__
	else {
		MagickSetImageFormat(background_wand, "PNG");
		size_t background_blob_length;
		unsigned char* background_blob = MagickGetImageBlob(background_wand, &background_blob_length);
		char* output_stdout = malloc((strlen("/proc//fd/1")+((int)log10(output_PID)+1)+1)*sizeof(char));
		sprintf(output_stdout, "/proc/%d/fd/1", output_PID);
		FILE* output_file = fopen(output_stdout, "a");
		fwrite(background_blob, sizeof(char), background_blob_length, output_file);
		fclose(output_file);
		// with my server the reader gets EOF (I think? Doesn't seem to be a signal) too early
		// not a great solution but I tried everything I could think of
		sleep(1);
		kill(output_PID, SIGINT);
	}
#endif
	DestroyMagickWand(background_wand);
	DestroyPixelWand(letterbox_wand);
	MagickWandTerminus();
/*}}}*/

	return 0;
}
