#ifndef MAIN_H
#define MAIN_H
struct Screen;

typedef struct Connection {
	struct Screen* screen;
	int direction;
} Connection;

typedef struct Screen {
	Screenshot* screenshot;
	int x_scrot;
	int y_scrot;
	int connections_length;
	Connection* connections;
} Screen;
#endif
