#ifndef MAIN_H
#define MAIN_H
struct Screen;

typedef struct Connection {
	struct Screen* screen;
	int direction;
} Connection;

typedef struct Screen {
	Screenshot* screenshot;
#ifdef RW
	Screenshot* screenshot1;
	Screenshot* screenshot2;
	Screenshot* screenshot3;
	Screenshot* screenshot4;
	Screenshot* screenshot5;
#endif
	int x_scrot;
	int y_scrot;
	int connections_length;
	Connection* connections;
} Screen;
#endif
