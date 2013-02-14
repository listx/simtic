/* Simtic -- a tic-tac-toe playing program that uses the minimax() search
 * algorithm.
 *
 * Copyright (C) 2010-2013 Linus Arver (linus /at/ ucla /dot/ edu)
 *
 * License (GPL v3 or later):
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>	/* printf(), etc. */
#include <stdbool.h>	/* use bool type instead of mocking it with int */
#include <assert.h>	/* preemptive debugging */
#include <termios.h>	/* set terminal to 1-character-at-a-time input */

#define MAX(X,Y) (X > Y ? X : Y)
#define MIN(X,Y) (X > Y ? Y : X)

/*
 * Total number of possible moves; highest is 9 (empty board); lowest is 0 (all
 * squares taken).
 */
#define MOVES_MAX 9

#define SQUARES_MAX 9 /* Total number of squares in the board. */

/*
 * Define a type for describing the state of an arbitrary square in the
 * board. The square can be occupied by X (White), O (Black), or just EMPTY (no
 * piece on it). We call X White and O Black because X moves first (like what
 * the White pieces do in Chess).
 */
enum {
	WHITE, BLACK, EMPTY
};

/* Board position information. */
struct board_pos {
	int sq[SQUARES_MAX]; /* There are 9 squares, indexed 0 - 8. */
	int color; /* The color that will make the next move. */
};

/* Move list used by AI. */
struct move_list {
	/*
	 * A move is represented by the square that the X or O will fill in ---
	 * the possible range is 0 - 8.
	 *
	 * Since tic-tac-toe is played by placing X or O on an empty square,
	 * move[] just holds all of the available empty squares. This info is
	 * used to generate a move tree for all possible, legal moves.
	 *
	 * The struct is called move_list because move[] will contain a list of
	 * empty squares, each of which is a valid move. Hence, move_list.
	 */
	int move[MOVES_MAX];
	/*
	 * Number of empty squares; this is just a sum of how many non-null
	 * values are inside move[].
	 */
	int moves;
};

/*
 * Fake NULL value for an uninitialized move variable; this is needed because of
 * the int 'move' array used by move_list.
 */
const int MOVE_NONE = -1;
/* The best possible score for a given position. */
const int INF = 100;
/* All winning three-in-a-row combinations in the game. */
const int winning_squares[8][3] = {
	/* rows */
	{0,1,2},
	{3,4,5},
	{6,7,8},
	/* columns */
	{0,3,6},
	{1,4,7},
	{2,5,8},
	/* diagonals */
	{0,4,8},
	{2,4,6}
};

/* Tell user how many times we called minimax(); this serves to verify the
 * difficulty levels. */
static unsigned int nodecount;

/* FUNCTION PROTOTYPES */

/* Game mechanics */
void game_loop();
void newgame(bool human, int depth);
void make_move(struct board_pos *pos, bool human, int depth);
int move_pick(struct board_pos *pos, int depth);
/* Evaluation */
int checkmate(struct board_pos *pos);
int eval(struct board_pos *pos);
/* Search */
int minimax(struct board_pos *pos, int depth);
/* Move handling */
void move_generate(struct board_pos *pos, struct move_list *mp);
void move_do(struct board_pos *pos, int move);
void move_undo(struct board_pos *pos, int move);
/* Misc board helper */
bool board_empty(struct board_pos *pos);
/* UI helpers */
void display_moves(struct move_list *mp);
void display_board(struct board_pos *pos);

int main()
{
	struct termios orig, rawmode;

	/* Disable buffering for easier debugging. */
        setvbuf(stdout, NULL, _IONBF ,0);

	/* Get current terminal settings. */
        tcgetattr(0, &orig);
	/*
	 * Modify settings so that we are in "raw" mode --- this effectively
	 * makes getchar() behave so that it reads in a single key press
	 * immediately (instead of following up with the ENTER key).
	 */
        rawmode=orig;
        rawmode.c_lflag &= ~(ISIG|ICANON);
        rawmode.c_cc[VMIN]=1;
        rawmode.c_cc[VTIME]=2;
	/* Apply these settings to the current terminal. */
        tcsetattr(0, TCSANOW, &rawmode);

	game_loop();

	/* restore terminal settings before exiting */
        tcsetattr(0, TCSANOW, &orig);

	return 0;
}

void game_loop()
{
	bool human = false;
	int depth;

start_game_loop:
	printf("\nStarting new game...");

move_first_menu:
	printf("\nWould you like to move first? (y/n) ");
	switch (getchar()) {
	case 'y': human = true; break;
	case 'n': human = false; break;
	default: goto move_first_menu;
	}

difficulty_menu:
	printf("\nChoose difficulty ([h]ard/[m]edium/[e]asy): ");
	switch (getchar()) {
	case 'h': depth = 9; break;
	case 'm': depth = 3; break;
	case 'e': depth = 1; break;
	default: goto difficulty_menu;
	}

	newgame(human, depth);

newgame_menu:
	printf("\nPlay again? (y/n) ");
	switch (getchar()) {
	case 'y': goto start_game_loop;
	case 'n': goto exit_game_loop;
	default: goto newgame_menu;
	}

exit_game_loop:
	printf("\nGoodbye!\n");
}

void newgame(bool human, int depth)
{
	struct board_pos pos = {
		{
			EMPTY, EMPTY, EMPTY,
			EMPTY, EMPTY, EMPTY,
			EMPTY, EMPTY, EMPTY
		},
		WHITE
	};

	/*
	 * Keep making moves until the board is filled up, or if someone
	 * wins.
	 */
	while (board_empty(&pos)) {
		make_move(&pos, human, depth);
		/*
		 * Swap move order; otherwise, one side will make all the
		 * moves!
		 */
		human = human ? false : true;

		if (checkmate(&pos)) {
			printf("%s wins!\n", (pos.color == WHITE) ? "Black (O)" : "White (X)");
			printf("%s won the game!", human ? "AI" : "You");
			break;
		}
	}
	display_board(&pos);
	if (!checkmate(&pos))
		printf("Draw!\n");
}

/*
 * Either prompt the user to input a move, or make the AI decide a move, and
 * then execute that move.
 */
void make_move(struct board_pos *pos, bool human, int depth)
{
	int move, sq, sq_tentative, best_sq;
	if (human) {
		display_board(pos);
choose_square:
		printf("Enter square 0 - 8: ");
		sq_tentative = getchar();
		printf("\n");
		switch (sq_tentative) {
		case '0': sq = 0; break;
		case '1': sq = 1; break;
		case '2': sq = 2; break;
		case '3': sq = 3; break;
		case '4': sq = 4; break;
		case '5': sq = 5; break;
		case '6': sq = 6; break;
		case '7': sq = 7; break;
		case '8': sq = 8; break;
		default: goto choose_square;
		}

		if (pos->sq[sq] == EMPTY) {
			move = sq;
		} else {
			printf("That square is taken.\n");
			goto choose_square;
		}
	} else {
		printf("Deciding best move... ");
		best_sq = move_pick(pos, depth);
		printf("AI chose square %d\n", best_sq);
		move = best_sq;
	}

	move_do(pos, move);
}

/*
 * Select the best possible move for the given position. First generate all
 * legal moves with move_generate(), and get the score of each move. Play the
 * move with the best score for the current color.
 */
int move_pick(struct board_pos *pos, int depth)
{
	struct move_list mlist;
	int move_picked, score_current, score_of_candidate_move, i;

	nodecount = 0;

	/* Generate all possible moves. */
	move_generate(pos, &mlist);

	/*
	 * Pick default move, so that if we don't find a move that improves our
	 * position, we can at least fall back to this move.
	 */
	move_picked = mlist.move[0];

	printf("Possible moves: ");
	display_moves(&mlist);

	/*
	 * Assume that the current position is very bad, and that we need to
	 * improve our position with the next move. If it's WHITE to move, we
	 * start out with -INF, and work our way up. If it's BLACK to move, we
	 * start with INF and try to get the smallest score (most negative
	 * score).
	 */
	score_current = (pos->color == WHITE) ? -INF : INF;

	for (i = 0; i < mlist.moves; i++) {
		move_do(pos, mlist.move[i]);
		score_of_candidate_move = minimax(pos, depth);
		move_undo(pos, mlist.move[i]);
		if (pos->color == WHITE) {
			if (score_of_candidate_move > score_current) {
				move_picked = mlist.move[i];
				score_current = score_of_candidate_move;
			}
		} else {
			if (score_of_candidate_move < score_current) {
				move_picked = mlist.move[i];
				score_current = score_of_candidate_move;
			}
		}
	}

	assert(move_picked < SQUARES_MAX);
	assert(pos->sq[move_picked] == EMPTY);
	printf("After examining %u nodes, best move is: %d\n", nodecount, move_picked + 1);
	return move_picked;
}

/*
 * Return an evaluation of the position. We return 1 if the color that
 * just moved has won the game. We return 0 otherwise. The chess equivalent is
 * determining if it is checkmate for a given side. We always call this function
 * first, because if there is a won condition, it does not make any sense to
 * keep evaluating past that point.
 */
int checkmate(struct board_pos *pos)
{
	int i, j, player_color, ours;
	/*
	 * Get the color that just played the last move (the opposite of the
	 * current color).
	 */
	player_color = (pos->color == WHITE) ? BLACK : WHITE;
	/* Check each won condition. */
	for (i = 0; i < 8; i++) {
		ours = 0;
		for (j = 0; j < 3; j++) {
			if (pos->sq[winning_squares[i][j]] == player_color)
				ours++;
			else
				break;
		}
		/* If we have a 3 in a row, then yes, somebody won the game! */
		if (ours == 3)
			return 1;
	}

	/* Nobody won the game so far. */
	return 0;
}

/*
 * Examine the position, and return a score based on how many possible
 * three-in-a-row opportunities there are. The higher the score, the more
 * opportunities. This function is very similar to checkmate(), because of the
 * simplicity of the game.
 */
int eval(struct board_pos *pos)
{
	int i, j, enemy_color, ours, points;
	points = 0;
	enemy_color = (pos->color == WHITE) ? BLACK : WHITE;
	/* Check each row, column, and diagonal for winning chances. */
	for (i = 0; i < 8; i++) {
		ours = 0;
		for (j = 0; j < 3; j++) {
			if (pos->sq[winning_squares[i][j]] == pos->color)
				ours++;
		}
		/*
		 * If we have 2 of our own lined up, check if there is an
		 * un-occupied empty square that we can play to make a
		 * three-in-a-row in the next turn.
		 */
		if (ours > 1) {
			for (j = 0; j < 3; j++) {
				if (pos->sq[winning_squares[i][j]] == EMPTY)
					points++;
				if (pos->sq[winning_squares[i][j]] == enemy_color)
					points--;
			}
		}
	}

	return points;
}

/*
 * minimax() is really an evaluation function; it merely looks at the root node
 * (the given position), and evaluates it by looking at variations that result
 * from playing different moves. The only real difference versus eval() and
 * checkmate() is that minimax() calls itself recursively to find the
 * evaluation.
 */
int minimax(struct board_pos *pos, int depth)
{
	struct move_list mlist;
	int score, score_best, won, i;
	nodecount++;

	/* Check if position is already won */
	won = checkmate(pos);
	/*
	 * If the game is WON and it is WHITE to move, that means BLACK made the
	 * last move, and thus, BLACK won the game (-INF achieved!)
	 */
	if (won)
		return ((pos->color == WHITE) ? -INF : INF);

	/* No one has won yet, but the board is full; this is a draw. */
	if (!board_empty(pos))
		return 0;

	score = eval(pos);
	/*
	 * If we are at the end of our search "horizon," but no one won, just
	 * return whatever eval() says it is.
	 */
	if (depth == 0) {
		/* if it is WHITE to move, that means BLACK made the last move,
		 * so we have to return a negative value in that case (remember,
		 * BLACK is trying to find the most negative value)
		 */
		return ((pos->color == WHITE) ? -score : score);
	}

	/*
	 * If we are here, it means that the game has not ended yet, so we do
	 * our usual evaluation of it. First, we generate all possible moves.
	 * Then, we try out each move on the board, and then call our evaluation
	 * function. If the level of difficulty is high, we search all possible
	 * variations (depth 9). On medium, we only search down to depth 3. On
	 * easy, we only search down to depth 1.
	 */
	move_generate(pos, &mlist);

	/*
	 * White starts out with -INF and tries to maximize it; Black starts
	 * with INF and tries to minimize it.
	 */
	score = (pos->color == WHITE) ? -INF : INF;
	for (i = 0; i < mlist.moves; i++) {
		move_do(pos, mlist.move[i]);
		score_best = minimax(pos, depth - 1);
		move_undo(pos, mlist.move[i]);
		if (pos->color == WHITE)
			score = MAX(score, score_best);
		else
			score = MIN(score, score_best);
	}

	return score;
}

/*
 * Generate all possible moves from the given position. This is tic-tac-toe, so
 * it's very simple: we just return all the squares that are empty; nodes is the
 * number of possible moves in this position (the total number of empty
 * squares).
 */
void move_generate(struct board_pos *pos, struct move_list *mp)
{
	int i;
	/* Reset the move list. */
	for (i = 0; i < MOVES_MAX; i++) {
		mp->move[i] = MOVE_NONE;
	}
	mp->moves = 0;

	/* Find all empty squares, and place the moves into mp. */
	for (i = 0; i < SQUARES_MAX; i++) {
		if (pos->sq[i] == EMPTY) {
			mp->move[mp->moves] = i;
			mp->moves++;
		}
	}
}

/* Execute the move on the board. */
void move_do(struct board_pos *pos, int move)
{
	pos->sq[move] = pos->color;
	pos->color = (pos->color == WHITE) ? BLACK : WHITE;
}

/* Undo a move on the board. */
void move_undo(struct board_pos *pos, int move)
{
	pos->sq[move] = EMPTY;
	pos->color = (pos->color == WHITE) ? BLACK : WHITE;
}

bool board_empty(struct board_pos *pos)
{
	int i;
	for (i = 0; i < SQUARES_MAX; i++) {
		if (pos->sq[i] == EMPTY)
			return true;
	}
	return false;
}

/* Print a list of available moves that can be played. */
void display_moves(struct move_list *mp)
{
	int i;
	for (i = 0; i < mp->moves; i++) {
		switch (mp->move[i]) {
		case 0: printf("0"); break;
		case 1: printf("1"); break;
		case 2: printf("2"); break;
		case 3: printf("3"); break;
		case 4: printf("4"); break;
		case 5: printf("5"); break;
		case 6: printf("6"); break;
		case 7: printf("7"); break;
		case 8: printf("8"); break;
		default: assert(0);
		}
		printf(" ");
	}
	printf("\n");
}

void display_board(struct board_pos *pos)
{
	int i;
	printf("\n+---+---+---+\n");
	for (i = 0; i < SQUARES_MAX; i++) {
		printf("| ");
		switch (pos->sq[i]) {
		case WHITE: printf("X"); break;
		case BLACK: printf("O"); break;
		case EMPTY: printf(" "); break;
		default: break;
		}
		printf(" ");
		if (i == 2 || i == 5 || i == 8) {
			printf("|\n");
			printf("+---+---+---+\n");
		}
	}
	printf("\n");
}
