#include "strategy.h"

#include <cstdint>

#include "SDL_stdinc.h"

static bool isInBound(Sint8 x, Sint8 y) {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

bool Strategy::isPositionValid(Sint8 x, Sint8 y) const {
    return isInBound(x, y) && _blobs.get(x, y) == -1 && !_holes.get(x, y);
}

// The score of a player is its number of blobs
void Strategy::initializeScores() {
    _playerScore[0] = 0;
    _playerScore[1] = 0;
    for (Uint8 x = 0; x < 8; ++x) {
        for (Uint8 y = 0; y < 8; ++y) {
            Sint16 cellValue = _blobs.get(x, y);
            if (cellValue != -1) {
                ++_playerScore[cellValue];
            }
        }
    }
}

void Strategy::applyMove(const movement& mv) {
    if (mv.distance() == 1) {
        ++_playerScore[_current_player];
    } else {
        _blobs.set(mv.ox, mv.oy, -1);
    }
    _blobs.set(mv.nx, mv.ny, _current_player);

    for (Sint8 dx = -1; dx <= 1; ++dx) {
        for (Sint8 dy = -1; dy <= 1; ++dy) {
            Sint8 neighbourX = mv.nx + dx;
            Sint8 neighbourY = mv.ny + dy;

            if (isInBound(neighbourX, neighbourY)) {
                Sint16 cellValue = _blobs.get(neighbourX, neighbourY);
                Sint16 _opponent_player = _current_player ^ 1;

                if (cellValue == _opponent_player) {
                    --_playerScore[_opponent_player];
                    _blobs.set(neighbourX, neighbourY, _current_player);
                    ++_playerScore[_current_player];
                }
            }
        }
    }
}

Sint32 Strategy::estimateCurrentScore() const {
    return _playerScore[_current_player] - _playerScore[_current_player ^ 1];
    // TODO: take into account infinite loops
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
Uint32 minMaxDepth = 1;

void Strategy::computeBestMove() {
    initializeScores();
#ifdef _GREEDY
    computeGreedyMove();
    _saveBestMove(mov);
#endif
#ifdef _MINMAX
    computeMinMaxMove(minMaxDepth);
#endif
}

Sint32 Strategy::computeGreedyMove() {
    vector<movement> validMoves;
    computeValidMoves(validMoves);

    movement bestMove;
    Sint32 bestScore = INT32_MIN;

    if (validMoves.size() == 0) {  // TODO: find value
        return bestScore;
    }

    for (auto mv : validMoves) {
        // cout << "mv: " << (int)mv.ox << ' ' << (int)mv.oy << ' ' <<
        // (int)mv.nx
        //<< ' ' << (int)mv.ny << '\n';
        bidiarray<Sint8> temp_blobs = _blobs;
        Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

        applyMove(mv);
        Sint32 score = estimateCurrentScore();
        // cout << "score: " << score << '\n';

        _blobs = temp_blobs;
        _playerScore[0] = prevScore[0];
        _playerScore[1] = prevScore[1];

        if (score > bestScore) {
            bestScore = score;
            bestMove = mv;
        }
    }

    mov = bestMove;
    return bestScore;
}

Sint32 Strategy::computeMinMaxMove(Uint32 depth) {
    if (depth == 0) {
        Sint32 score = computeGreedyMove();
        _current_player ^= 1;
        return score;
    } else {
        vector<movement> validMoves;
        computeValidMoves(validMoves);

        Sint32 bestScore = INT32_MIN;

        for (auto mv : validMoves) {
            bidiarray<Sint8> temp_blobs = _blobs;
            Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

            applyMove(mv);
            _current_player ^= 1;
            Sint32 score = -computeMinMaxMove(depth - 1);
            if (score > bestScore) {
                bestScore = score;
                if (depth == minMaxDepth) {
                    _saveBestMove(mv);
                }
            }

            // if (depth == minMaxDepth) {
            //   cout << "mv: " << (int)mv.ox << ' ' << (int)mv.oy << ' '
            //<< (int)mv.nx << ' ' << (int)mv.ny << '\n';
            // cout << "score: " << score << '\n';
            //}

            _blobs = temp_blobs;
            _playerScore[0] = prevScore[0];
            _playerScore[1] = prevScore[1];
        }

        _current_player ^= 1;
        return bestScore;
    }
}
