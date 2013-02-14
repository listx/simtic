Introduction
------------

Simtic allows HUMAN v. CPU play in a game of text-based tic-tac-toe on Linux.

The CPU has 3 difficulty levels, with the hardest difficulty resulting in a
full minimax search of all possible game moves. This is possible because the
game of tic-tac-toe is simple enough to do a brute-force search of all game
scenarios. Any more complicated game would require a search function that
continuosly searches for the best move while holding a "best move" candidate at
hand (this is how chess engines work).

Building
--------

Use the `make` command to create a binary using the included `Makefile`.
