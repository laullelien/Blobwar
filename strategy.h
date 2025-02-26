#ifndef __STRATEGY_H
#define __STRATEGY_H

#include "SDL_stdinc.h"
#include "bidiarray.h"
#include "extendedMovement.h"
#include "move.h"

class Strategy {
   private:
    //! array containing all blobs on the board
    bidiarray<Sint8> _blobs;
    //! an array of booleans indicating for each cell whether it is a hole or
    //! not.
    const bidiarray<bool>& _holes;
    //! Current player
    Uint16 _current_player;

    //! Call this function to save your best move.
    //! Multiple call can be done each turn,
    //! Only the last move saved will be used.
    void (*_saveBestMove)(movement&);

    // Array containing the score of both players
    Sint32 _playerScore[2] = {0, 0};

   public:
    // Constructor from a current situation
    Strategy(bidiarray<Sint16>& blobs,
             const bidiarray<bool>& holes,
             const Uint16 current_player,
             void (*saveBestMove)(movement&))
        : _holes(holes),
          _current_player(current_player),
          _saveBestMove(saveBestMove) {
        for (Sint8 i = 0; i < 8; ++i) {
            for (Sint8 j = 0; j < 8; ++j) {
                _blobs.set(i, j, blobs.get(i, j));
            }
        }
    }

    // Copy constructor
    Strategy(const Strategy& St)
        : _blobs(St._blobs),
          _holes(St._holes),
          _current_player(St._current_player) {
        _playerScore[0] = St._playerScore[0];
        _playerScore[1] = St._playerScore[1];
    }

    // Destructor
    ~Strategy() {}

    /**
     * Initializes the score of both players in score array.
     * The score of a player is the number of blobs he has.
     */
    void initializeScores();

    /**
     * Set _current_player to the opponent player of the current player
     */
    void switchPlayer();

    /**
     * Apply a move to the current state of blobs
     * Assumes that the move is valid
     */
    void applyMove(const movement& mv);

    /**
     * Returns a boolean that indicates whether the player
     * can play in position (x, y)
     */
    bool isPositionValid(Sint8 x, Sint8 y) const;

    /**
     * Compute the vector containing every possible moves
     */
    vector<movement>& computeValidMoves(vector<movement>& valid_moves) const;

    /**
     * Compute the number of valid moves that both player can do. The value is
     * stored using the parameters
     */
    void numberOfMoves(Sint32& firstPlayerMoves,
                       Sint32& secondPlayerMoves) const;

    /**
     * Returns the score associated to a move.
     */
    Uint8 computeScore(extendedMovement& mv) const;

    /**
     * Estimate the score of the current state of the game
     */
    Sint32 estimateCurrentScore() const;

    /**
     * Estimates the maximal depth p so that the number of boards of depth p is less than limit.
     */
    Uint32 estimateMaxDepth(Sint64 limit, Uint32& depth) const;

    /**
     * Find the best move.
     */
    void computeBestMove();

    /**
     * Finds a move using a greedy strategy
     */
    Sint32 computeGreedyMove();

    /**
     * Finds a move using the minmax algorithm
     */
    Sint32 computeMinMaxMove(Uint32 depth);

    /**
     * Finds a move using the minmax algorithm
     */
    Sint32 computeMinMaxAlphaBetaMove(Uint32 depth, Sint32 alpha, Sint32 beta);

    /**
     * Finds a move using the minmax algorithm and parallelism
     */
    Sint32 computeMinMaxAlphaBetaParallelMove(Uint32 depth,
                                              Sint32 alpha,
                                              Sint32 beta);
};

#endif
