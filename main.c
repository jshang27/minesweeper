#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void usage(FILE *file, const char *progname) {
	fprintf(file, "USAGE: %s [cols rows] [mines]\n\tcols: positive integer up to 30 (default 30)\n\trows: positive integer up to 24 (default 24)\n\tmines: positive integer up to cols*rows (default 10, will need to specify if cols*rows < 10)\n", progname);
}

typedef struct {
	char number;
	bool mine;
	bool revealed;
	bool flagged;
} Tile;

typedef struct {
	Tile *tiles;
	int cols;
	int rows;
	int mines_remaining;
} Board;

Tile default_tile = {.number=0, .mine=false, .revealed=true, .flagged=false};

bool is_between(int a, int lower, int upper) {
	return a >= lower && a < upper;
}

Tile *board_get(const Board *board, int idx, int idy) {
	if (!is_between(idx, 0, board->cols)) {
		return &default_tile;
	}
	if (!is_between(idy, 0, board->rows)) {
		return &default_tile;
	}
	return board->tiles + idy * board->cols + idx;
}

void destroy_board(Board *board) {
	free(board->tiles);
	free(board);
}

Board *create_board(int cols, int rows, int mines) {
	int size = cols * rows;
	Tile *tiles = calloc(size, sizeof(Tile));
	Board *board = malloc(sizeof(Board));
	board->cols = cols;
	board->rows = rows;
	board->tiles = tiles;
	board->mines_remaining = mines;

	for (int i = 0; i < mines; i++) {
		int idx = rand() % size;
		if (tiles[idx].mine) {
			i--;
			continue;
		}
		tiles[idx].mine = true;
	}

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			Tile *tile = board_get(board, x, y);
			for (int dy = -1; dy < 2; dy++) {
				for (int dx = -1; dx < 2; dx++) {
					tile->number += board_get(board, x + dx, y + dy)->mine;
				}
			}
			tile->number -= tile->mine;
		}
	}

	return board;
}

void add_number(char num) {
	if (num == 0) {
		addch(' ');
		return;
	}
	attron(COLOR_PAIR(num));
	addch(num + '0');
	attroff(COLOR_PAIR(num));
}

void add_tile(void) {
	attron(COLOR_PAIR(8));
	addch(' ');
	attroff(COLOR_PAIR(8));
}

bool reveal_tile(Board *board, int x, int y) {	
	Tile *tile = board_get(board, x,y);
	if (tile->flagged) {
		tile->flagged = false;
		board->mines_remaining++;
	}
	if (tile->revealed) {
		return true;
	}

	tile->revealed = true;
	if (tile->mine) {
		return false;
	}

	bool successful = true;
	if (tile->number == 0) {
		for (int dy = -1; dy < 2; dy++) {
			for (int dx = -1; dx < 2; dx++) {
				successful &= reveal_tile(board, x + dx, y + dy);
			}
		}
	}

	return successful;	
}

void add_flag(void) {
	attron(COLOR_PAIR(9));
	addch('F');
	attroff(COLOR_PAIR(9));
}	

void add_board(Board *board) {
	for (int y = 0; y < board->rows; y++) {
		for (int x = 0; x < board->cols; x++) {
			Tile *tile = board_get(board, x, y);
			if (tile->flagged) {
				add_flag();
			} else if (tile->revealed) {
				add_number(board_get(board, x, y)->number);
			} else {
				add_tile();
			}
		}
		addstr("\n");
	}
}

int main(int argc, char **argv) {
	if (argc > 4) {
		fprintf(stderr, "\033[031merror:\033[0m too many arguments\n"); 
		usage(stderr, argv[0]);
		return 1;
	}

	int cols = 30;
	int rows = 24;
	int mines = 10;

	if (argc > 2) {
		char *end;
		cols = strtol(argv[1], &end, 10);
		if (*end || cols < 1 || cols > 30) {
			fprintf(stderr, "\033[31merror:\033[0m expected a cols value between 1 and 30 (inclusive)\n");
			usage(stderr, argv[0]);
			return 1;
		}
		rows = strtol(argv[2], &end, 10);
		if (*end || rows < 1 || rows > 24) {
			fprintf(stderr, "\033[31merror:\033[0m expected a rows value between 1 and 24 (inclusive)\n");
			usage(stderr, argv[0]);
			return 1;
		}
	}

	if (argc == 2 || argc == 4) {
		char *end;
		mines = strtol(argv[argc - 1], &end, 10);
		if (*end || mines < 1) {
			fprintf(stderr, "\033[31merror:\033[0m expected a mines value above 0\n");
			usage(stderr, argv[0]);
		       	return 1;
		}
	}

	if (mines > rows * cols) {
		fprintf(stderr, "\033[31merror:\033[0m expected a mines value less than or equal to rows*cols (%d)\n", rows * cols);
		usage(stderr, argv[0]);
		return 1;
	}	

	initscr();
	keypad(stdscr, TRUE);
	noecho();
	ESCDELAY = 0;

	srand(time(NULL));
	Board *board = create_board(cols, rows, mines);

	int return_code = 0;
	char *errmsg;

	if (has_colors() == FALSE) {
		errmsg = "\033[31merror:\033[0m terminal does not support color\n";
		return_code = 1;
		goto end;
	}
	start_color();

	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_CYAN, COLOR_BLACK);
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(6, COLOR_YELLOW, COLOR_BLACK);
	init_pair(7, COLOR_WHITE, COLOR_BLACK);
	init_pair(8, COLOR_BLACK, COLOR_WHITE);
	init_pair(9, COLOR_RED, COLOR_WHITE);

	int curs_x = 0;
	int curs_y = 0;

	while (true) {
		erase();
		add_board(board);
		refresh();

		move(curs_y, curs_x);

		switch (getch()) {
		case 'q':; __attribute__((fallthrough));
		case 27: {
			goto end;
		}
		case KEY_UP:; __attribute__((fallthrough));
		case 'k':; __attribute__((fallthrough));
		case 'w': {
			if (curs_y > 0) {
				curs_y--;
			}
		} break;
		case KEY_DOWN:; __attribute__((fallthrough));
		case 'j':; __attribute__((fallthrough));
		case 's': {
			if (curs_y < board->rows - 1) {
				curs_y++;
			}
		} break;
		case KEY_LEFT:; __attribute__((fallthrough));
		case 'h':; __attribute__((fallthrough));
		case 'a': {
			if (curs_x > 0) {
				curs_x--;
			}
		} break;
		case KEY_RIGHT:; __attribute__((fallthrough));
		case 'l':; __attribute__((fallthrough));
		case 'd': {
			if (curs_x < board->cols - 1) {
				curs_x++;
			}
		} break;
		case ' ':; __attribute__((fallthrough));
		case 'f':; __attribute__((fallthrough));
		case 'o': {
			Tile *tile = board_get(board, curs_x, curs_y);
			if (!tile->revealed) {
				tile->flagged ^= true;
				board->mines_remaining--;
			}
		} break;
		case 10:; __attribute__((fallthrough));
		case 'e':; __attribute__((fallthrough));
		case 'i': {
			if (!reveal_tile(board, curs_x, curs_y)) {
				// lose sequence	
			}
		}
		}
	}

end:
	destroy_board(board);
	endwin();
	
	if (return_code != 0) {
		fprintf(stderr, "%s", errmsg);
	}

	return return_code;
}
