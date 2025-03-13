#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "MagickWand/MagickWand.h"
#ifdef __linux__
#	include <signal.h>
#	include <unistd.h>
#endif

/*{{{ structs and variables*/
bool** monitors;
int*** monitors_data;
int x_max, y_max;
/*}}}*/

int main(int argc, char* argv[]) {
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
	//sscanf(argv[2], "%d,%d", &x_origin, &y_origin);
	monitors = malloc((x_max+1)*sizeof(bool*));
	monitors_data = malloc((x_max+1)*sizeof(bool*));
	//visited = malloc((x_max+1)*sizeof(bool*));
	for(int i = 0; i < x_max+1; i++) {
		monitors[i] = malloc((y_max+1)*sizeof(bool));
		monitors_data[i] = malloc((y_max+1)*sizeof(bool*));
		//visited[i] = malloc((y_max+1)*sizeof(bool));
		for(int j = 0; j < y_max+1; j++) {
			monitors[i][j] = 0;
			//visited[i][j] = 0;
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

	int w_scrot = 1024;
	int w_scrot_extended = 1366;
	int h_scrot = 768;

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

			MagickWand* screen_wand = NewMagickWand();
			PixelWand* layout_wand = NewPixelWand();
			PixelSetColor(layout_wand, "#00FF00");
			MagickSetImageBackgroundColor(screen_wand, layout_wand);
			MagickNewImage(screen_wand, w_scrot_extended, h_scrot, layout_wand);
			int w = w_scrot;
			MagickSetImageBackgroundColor(screen_wand, letterbox_wand);
			int h = h_scrot;
			int w_letterboxed = w_scrot_extended-w;
			int x_scrot = w_letterboxed/2;
			int y_scrot = 0;

	/*{{{ if the monitor is widescreen and not using a multiscreen room, widescreen*/
			if(w_letterboxed > w/2) w_letterboxed = w_scrot_extended-w_scrot;
			if(((double)monitors_data[i][j][2])/monitors_data[i][j][3] > ((double)w+w_letterboxed/2)/h) {
				// the cropping and extending is to make rooms that are letterboxed on one side not centered (end of long rooms)
				if(i-1 < 0 || !monitors[i-1][j] || true) {
					w += w_letterboxed/2;
					x_scrot -= w_letterboxed/2;
				}
				if(i+1 > x_max || !monitors[i+1][j] || true) {
					w += w_letterboxed/2;
					MagickCropImage(screen_wand, w, h, x_scrot, y_scrot);
					if(w != w_scrot_extended) {
						MagickExtentImage(screen_wand, w_scrot_extended, h, -w_letterboxed/2, 0);
						w = w_scrot_extended;
					}
				}
				else {
					MagickCropImage(screen_wand, w, h, x_scrot, y_scrot);
					if(w == w_scrot+w_letterboxed/2) {
						MagickExtentImage(screen_wand, w_scrot_extended, h, 0, 0);
						w = w_scrot_extended;
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
			MagickResizeImage(screen_wand, monitors_data[i][j][2], monitors_data[i][j][3],
				PointFilter
			);
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
		int output_fd;
		if(kill(output_PID, 0) != -1 && (output_fd = open(output_stdout, O_WRONLY | O_APPEND)) != -1) {
			while(background_blob_length > 0) {
				ssize_t res = write(output_fd, background_blob, background_blob_length);
				if(res < 0) {
					if(errno == EINTR) continue;
					break;
				}
				background_blob = &background_blob[res];
				// save CPU just in case, though server should prevent this now
				if((background_blob_length -= res) != 0) sleep(0.5);
			}
			close(output_fd);
			// with my server the reader gets EOF (I think? Doesn't seem to be a signal) too early
			// not a great solution but I tried everything I could think of
			sleep(1);
			kill(output_PID, SIGINT);
		}
	}
#endif
	DestroyMagickWand(background_wand);
	DestroyPixelWand(letterbox_wand);
	MagickWandTerminus();
/*}}}*/

	return 0;
}
