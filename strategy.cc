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
            Sint8 cellValue = _blobs.get(x, y);
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
                Sint8 cellValue = _blobs.get(neighbourX, neighbourY);
                Sint8 _opponent_player = _current_player ^ 1;

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

bool Strategy::canMove(Uint16 player) const {
    for (Sint8 x = 0; x < 8; ++x) {
        for (Sint8 y = 0; y < 8; ++y) {
            if (_blobs.get(x, y) == player) {
                for (Sint8 dx = -2; dx <= 2; ++dx) {
                    for (Sint8 dy = -2; dy <= 2; ++dy) {
                        if (isPositionValid(x + dx, y + dy)) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

movement mov;
Uint32 minMaxDepth = 3;
Uint32 minMaxAlphaBetaDepth = 3;
Sint32 inf = 1000000;
int calculatedMoves = 0;

void Strategy::computeBestMove() {
    initializeScores();
#ifdef _GREEDY
    computeGreedyMove();
    _saveBestMove(mov);
#endif
#ifdef _MINMAX
    computeMinMaxMove(minMaxDepth);
#endif
#ifdef _MINMAXALPHABETA
    computeMinMaxAlphaBetaMove(minMaxAlphaBetaDepth, -inf, inf);
#endif
    cout << "Numbers of move calculated: " << calculatedMoves << '\n';
}

Sint32 Strategy::computeGreedyMove() {
    ++calculatedMoves;
    vector<movement> validMoves;
    computeValidMoves(validMoves);

    movement bestMove;
    Sint32 bestScore = -inf;

    if (validMoves.size() == 0) {
        return estimateCurrentScore();
    }

    for (auto mv : validMoves) {
        bidiarray<Sint8> temp_blobs = _blobs;
        Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

        applyMove(mv);
        Sint32 score = estimateCurrentScore();

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
        Sint32 bestScore = -inf;

        if (validMoves.size() == 0) {
            bidiarray<Sint8> temp_blobs = _blobs;
            Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

            _current_player ^= 1;
            Sint32 score = -computeMinMaxMove(depth - 1);

            if (score > bestScore) {
                bestScore = score;
            }

            _blobs = temp_blobs;
            _playerScore[0] = prevScore[0];
            _playerScore[1] = prevScore[1];
        }

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

            _blobs = temp_blobs;
            _playerScore[0] = prevScore[0];
            _playerScore[1] = prevScore[1];
        }

        _current_player ^= 1;
        return bestScore;
    }
}

Sint32 Strategy::computeMinMaxAlphaBetaMove(Uint32 depth,
                                            Sint32 alpha,
                                            Sint32 beta) {
    if (depth == 0) {
        Sint32 score = computeGreedyMove();
        _current_player ^= 1;
        return score;
    } else {
        vector<movement> validMoves;
        computeValidMoves(validMoves);

        if (validMoves.size() == 0) {
            bidiarray<Sint8> temp_blobs = _blobs;
            Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

            _current_player ^= 1;
            Sint32 score =
                -computeMinMaxAlphaBetaMove(depth - 1, -beta, -alpha);

            _blobs = temp_blobs;
            _playerScore[0] = prevScore[0];
            _playerScore[1] = prevScore[1];

            if (score > alpha) {
                alpha = score;
            }

            if (score >= beta) {
                _current_player ^= 1;
                return beta;
            }
        }

        for (auto mv : validMoves) {
            bidiarray<Sint8> temp_blobs = _blobs;
            Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

            applyMove(mv);
            _current_player ^= 1;
            Sint32 score =
                -computeMinMaxAlphaBetaMove(depth - 1, -beta, -alpha);

            _blobs = temp_blobs;
            _playerScore[0] = prevScore[0];
            _playerScore[1] = prevScore[1];

            if (score > alpha) {
                alpha = score;
                if (depth == minMaxAlphaBetaDepth) {
                    _saveBestMove(mv);
                }
            }

            if (score >= beta) {
                _current_player ^= 1;
                return beta;
            }
        }

        _current_player ^= 1;
        return alpha;
    }
}
