#include "strategy.h"

#include <cassert>
#include <cstdint>

#include "SDL_stdinc.h"

static bool isInBound(Sint8 x, Sint8 y) {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

bool Strategy::isPositionValid(Sint8 x, Sint8 y) const {
    return isInBound(x, y) && _blobs.get(x, y) == -1 && !_holes.get(x, y);
}

void Strategy::increaseScore(Uint16 player) {
    _playerScore[player] += 1 - ((player ^ 1) << 1);
}

void Strategy::decreaseScore(Uint16 player) {
    _playerScore[player] -= 1 - ((player ^ 1) << 1);
}

// AI points are counted positively, while the player's points are counted
// negatively
void Strategy::initializeScores() {
    _playerScore[0] = 0;
    _playerScore[1] = 0;
    for (Uint8 x = 0; x < 8; ++x) {
        for (Uint8 y = 0; y < 8; ++y) {
            Sint16 cellValue = _blobs.get(x, y);
            if (cellValue != -1) {
                increaseScore(cellValue);
            }
        }
    }
}

void Strategy::applyMove(const movement& mv) {
    if (mv.distance() == 1) {
        _blobs.set(mv.nx, mv.ny, _current_player);
        increaseScore(_current_player);
    } else {
        _blobs.set(mv.ox, mv.oy, -1);
        _blobs.set(mv.nx, mv.ny, _current_player);
    }
    for (Sint8 dx = -1; dx <= 1; ++dx) {
        for (Sint8 dy = -1; dy <= 1; ++dy) {
            Sint8 neighbourX = mv.nx + dx;
            Sint8 neighbourY = mv.ny + dy;

            if (isInBound(neighbourX, neighbourY)) {
                Sint16 cellValue = _blobs.get(neighbourX, neighbourY);
                Sint16 opponent_player = _current_player ^ 1;

                if (cellValue == opponent_player) {
                    decreaseScore(opponent_player);
                    _blobs.set(neighbourX, neighbourY, _current_player);
                    increaseScore(_current_player);
                }
            }
        }
    }
}

Sint32 Strategy::estimateCurrentScore() const {
    return _playerScore[0] + _playerScore[1];
}

vector<movement>& Strategy::computeValidMoves(
    vector<movement>& validMoves) const {
    for (Sint8 x = 0; x < 8; ++x) {
        for (Sint8 y = 0; y < 8; ++y) {
            if (_blobs.get(x, y) == _current_player) {
                for (Sint8 dx = -2; dx <= 2; ++dx) {
                    for (Sint8 dy = -2; dy <= 2; ++dy) {
                        if (isPositionValid(x + dx, y + dy)) {
                            validMoves.push_back(
                                movement(x, y, x + dx, y + dy));
                        }
                    }
                }
            }
        }
    }
    return validMoves;
}

movement mov;

void Strategy::computeBestMove() {
    initializeScores();
#ifdef _GREEDY
    computeGreedyMove();
    _saveBestMove(mov);
#endif
#ifdef _MINMAX
    computeMinMaxMove(0);
#endif
}

Sint32 Strategy::computeGreedyMove() {
    vector<movement> validMoves;
    computeValidMoves(validMoves);

    movement bestMove;
    Sint32 bestScore;
    Sint32 factor;
    if (_current_player) {
        bestScore = INT32_MIN;
        factor = 1;
    } else {
        bestScore = INT32_MAX;
        factor = -1;
    }

    if (validMoves.size() == 0) {
        return bestScore;
    }

    for (auto mv : validMoves) {
        bidiarray<Sint8> temp_blobs = _blobs;
        Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

        applyMove(mv);
        Sint32 score = estimateCurrentScore();

        _blobs = temp_blobs;
        _playerScore[0] = prevScore[0];
        _playerScore[1] = prevScore[1];

        if (factor * score >
            factor * bestScore) {  // convert > into < for player
            bestScore = score;
            bestMove = mv;
        }
    }

    mov = bestMove;
    return bestScore;
}

#define MAX_DEPTH 4
Sint32 Strategy::computeMinMaxMove(Uint32 depth) {
    if (depth == MAX_DEPTH) {
        Sint32 score = computeGreedyMove();
        _current_player ^= 1;
        return score;
    } else {
        vector<movement> validMoves;
        computeValidMoves(validMoves);

        Sint32 bestScore;
        Sint32 factor;
        if (_current_player) {
            bestScore = INT32_MIN;
            factor = 1;
        } else {
            bestScore = INT32_MAX;
            factor = -1;
        }

        for (auto mv : validMoves) {
            bidiarray<Sint8> temp_blobs = _blobs;
            Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

            applyMove(mv);
            _current_player ^= 1;
            Sint32 score = computeMinMaxMove(depth + 1);
            if (factor * score > factor * bestScore) {
                bestScore = score;
                if (depth == 0) {
                    _saveBestMove(mv);
                }
            }

            _blobs = temp_blobs;
            _playerScore[0] = prevScore[0];
            _playerScore[1] = prevScore[1];
        }

        _current_player ^= 1;
        return bestScore;
    }
}
