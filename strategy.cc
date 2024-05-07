#include "strategy.h"

#include <future>
#include <random>

#include "SDL_stdinc.h"
#include "move.h"

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

void Strategy::switchPlayer() { _current_player ^= 1; }

void Strategy::applyMove(const movement& mv) {
    if (mv.distance() == 1) {
        ++_playerScore[_current_player];
    } else {
        _blobs.set(mv.ox, mv.oy, -1);
    }
    _blobs.set(mv.nx, mv.ny, _current_player);

    Sint8 _opponent_player = _current_player ^ 1;
    for (Sint8 dx = -1; dx <= 1; ++dx) {
        for (Sint8 dy = -1; dy <= 1; ++dy) {
            Sint8 neighbourX = mv.nx + dx;
            Sint8 neighbourY = mv.ny + dy;

            if (isInBound(neighbourX, neighbourY)) {
                Sint8 cellValue = _blobs.get(neighbourX, neighbourY);

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

Uint8 Strategy::computeScore(extendedMovement& mv) const {
    Uint16 score = mv.distance == 1;

    Sint8 _opponent_player = _current_player ^ 1;
    for (Sint8 dx = -1; dx <= 1; ++dx) {
        for (Sint8 dy = -1; dy <= 1; ++dy) {
            Sint8 neighbourX = mv.nx + dx;
            Sint8 neighbourY = mv.ny + dy;

            if (isInBound(neighbourX, neighbourY)) {
                Sint8 cellValue = _blobs.get(neighbourX, neighbourY);
                score += (cellValue == _opponent_player) << 1;
            }
        }
    }

    return score;
}

bool compareMove(const extendedMovement& a, const extendedMovement& b) {
    return a.score > b.score;
}

vector<movement>& Strategy::computeValidMoves(
    vector<movement>& validMoves) const {
    for (Sint8 x = 0; x < 8; ++x) {
        for (Sint8 y = 0; y < 8; ++y) {
            if (_blobs.get(x, y) == _current_player) {
                for (Sint8 dx = -2; dx <= 2; ++dx) {
                    for (Sint8 dy = -2; dy <= 2; ++dy) {
                        if (isPositionValid(x + dx, y + dy)) {
                            auto mv = extendedMovement(x, y, x + dx, y + dy);
                            mv.score = computeScore(mv);
                            validMoves.push_back(mv);
                        }
                    }
                }
            }
        }
    }

    shuffle(validMoves.begin(),
            validMoves.end(),
            std::default_random_engine(
                std::chrono::system_clock::now().time_since_epoch().count()));
    sort(validMoves.begin(), validMoves.end(), compareMove);

    return validMoves;
}

void Strategy::numberOfMoves(Sint32& firstPlayerMoves,
                             Sint32& secondPlayerMoves) const {
    firstPlayerMoves = 0, secondPlayerMoves = 0;
    for (Sint8 x = 0; x < 8; ++x) {
        for (Sint8 y = 0; y < 8; ++y) {
            if (_blobs.get(x, y) == 0) {
                for (Sint8 dx = -2; dx <= 2; ++dx) {
                    for (Sint8 dy = -2; dy <= 2; ++dy) {
                        if (isPositionValid(x + dx, y + dy)) {
                            ++firstPlayerMoves;
                        }
                    }
                }
            } else if (_blobs.get(x, y) == 1) {
                for (Sint8 dx = -2; dx <= 2; ++dx) {
                    for (Sint8 dy = -2; dy <= 2; ++dy) {
                        if (isPositionValid(x + dx, y + dy)) {
                            ++secondPlayerMoves;
                        }
                    }
                }
            }
        }
    }
}

Sint32 inf = 1000000;

// Algorithm settings

Uint32 minMaxDepth;

Uint32 minMaxAlphaBetaDepth;

Uint32 minMaxAlphaBetaParallelDepth;

Uint32 Strategy::estimateMaxDepth(Sint64 limit, Uint32& depth) const {
    Uint32 d = 0;
    Sint32 moveNb[2];
    numberOfMoves(moveNb[0], moveNb[1]);
    if (moveNb[0] * moveNb[1] < 2) {
        depth = 4;
        return 0;
    }
    Sint64 plays = moveNb[0];
    while (plays * moveNb[(minMaxDepth & 1) ^ 1] <= limit) {
        ++d;
        plays *= moveNb[minMaxDepth & 1];
    }
    depth = min(d, 6U);
    return plays;
}

#ifdef _STAT
atomic<Sint32> calculatedMoves;
atomic<Sint32> moves;
atomic<Sint32> players;
#endif

void Strategy::computeBestMove() {
#ifdef _STAT
    calculatedMoves = 0;
    moves = 0;
    players = 0;
#endif
    initializeScores();
#ifdef _GREEDY
    computeGreedyMove();
#endif
#ifdef _MINMAX
    // Determine depth by estimating number of calculations
    Sint64 maxBoards = 4000000;
    Uint32 plays = estimateMaxDepth(maxBoards, minMaxDepth);

#ifdef _STAT
    cout << "depth: " << minMaxDepth << endl;
    cout << "estimation of the number of moves: " << plays << endl;
#endif
    computeMinMaxMove(minMaxDepth);
#endif
#ifdef _MINMAXALPHABETA
    // Determine depth by estimating number of calculations
    Sint64 maxBoards = 8000000000;
    Uint32 plays = estimateMaxDepth(maxBoards, minMaxAlphaBetaDepth);

#ifdef _STAT
    cout << "depth: " << minMaxAlphaBetaDepth << endl;
    cout << "estimation of the number of moves: " << plays << endl;
#endif

    computeMinMaxAlphaBetaMove(minMaxAlphaBetaDepth, -inf, inf);
#endif
#ifdef _MINMAXALPHABETAPARALLEL
    // Determine depth by estimating number of calculations
    Sint64 maxBoards = 8000000000;
    minMaxAlphaBetaParallelDepth = Uint32 plays =
        estimateMaxDepth(maxBoards, minMaxAlphaBetaParallelDepth);

#ifdef _STAT
    cout << "depth: " << minMaxAlphaBetaParallelDepth << endl;
    cout << "estimation of the number of moves: " << plays << endl;
#endif

    computeMinMaxAlphaBetaParallelMove(minMaxAlphaBetaParallelDepth, -inf, inf);
#endif
#ifdef _STAT
    cout << "numbers of calculated move : " << calculatedMoves << '\n';
    cout << "numver of moves: " << moves << '\n';
    cout << "numver of players: " << players << '\n';
    cout << "average number of move per blob " << moves / players << '\n';
#endif
}

Sint32 Strategy::computeGreedyMove() {
    vector<movement> validMoves;
    computeValidMoves(validMoves);

#ifdef _STAT
    calculatedMoves++;
    moves += validMoves.size();
    players += _playerScore[_current_player];
#endif

    movement bestMove;

    if (validMoves.size() == 0) {
        return estimateCurrentScore();
    }

#ifdef _GREEDY
    _saveBestMove(validMoves[0]);
#endif
    return estimateCurrentScore() + ((extendedMovement)(validMoves[0])).score;
}

Sint32 Strategy::computeMinMaxMove(Uint32 depth) {
    if (depth == 0) {
        Sint32 score = computeGreedyMove();
        _current_player ^= 1;
        return score;
    }

    vector<movement> validMoves;
    computeValidMoves(validMoves);
    Sint32 bestScore = -inf;

#ifdef _STAT
    moves += validMoves.size();
    players += _playerScore[_current_player];
#endif

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

Sint32 Strategy::computeMinMaxAlphaBetaMove(Uint32 depth,
                                            Sint32 alpha,
                                            Sint32 beta) {
    if (depth == 0) {
        Sint32 score = computeGreedyMove();
        _current_player ^= 1;
        return score;
    }
    vector<movement> validMoves;
    computeValidMoves(validMoves);

#ifdef _STAT
    moves += validMoves.size();
    players += _playerScore[_current_player];
#endif

    if (validMoves.size() == 0) {
        bidiarray<Sint8> temp_blobs = _blobs;
        Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

        _current_player ^= 1;
        Sint32 score = -computeMinMaxAlphaBetaMove(depth - 1, -beta, -alpha);

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
        Sint32 score = -computeMinMaxAlphaBetaMove(depth - 1, -beta, -alpha);

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

Sint32 launchThread(Strategy* current_strategy,
                    movement* mv,
                    Uint32 depth,
                    Sint32 alpha,
                    Sint32 beta) {
    Strategy s(*current_strategy);
    s.applyMove(*mv);
    s.switchPlayer();
    return -s.computeMinMaxAlphaBetaMove(depth - 1, -beta, -alpha);
}

Sint32 Strategy::computeMinMaxAlphaBetaParallelMove(Uint32 depth,
                                                    Sint32 alpha,
                                                    Sint32 beta) {
    // don't save moves
    minMaxAlphaBetaDepth = inf;

    vector<movement> validMoves;
    computeValidMoves(validMoves);
    cout << "size: " << validMoves.size() << '\n';

#ifdef _STAT
    moves += validMoves.size();
    players += _playerScore[_current_player];
#endif

    Sint16 iterativeBranches = validMoves.size() / 4;

    // Compute some branches to try to get good alpha values
    for (Sint16 i = 0; i < iterativeBranches; ++i) {
        auto mv = validMoves[i];
        if (i == 0) {
            _saveBestMove(mv);
        }
        bidiarray<Sint8> temp_blobs = _blobs;
        Sint32 prevScore[2] = {_playerScore[0], _playerScore[1]};

        applyMove(mv);
        _current_player ^= 1;

        Sint32 score = -computeMinMaxAlphaBetaMove(depth - 1, -beta, -alpha);

        _blobs = temp_blobs;
        _playerScore[0] = prevScore[0];
        _playerScore[1] = prevScore[1];

        if (score > alpha) {
            alpha = score;
            _saveBestMove(mv);
        }
    }

    vector<future<Sint32>> scoreFuture(validMoves.size() - iterativeBranches);
    for (size_t i = 0; i < validMoves.size() - iterativeBranches; ++i) {
        scoreFuture[i] = async(launch::async,
                               launchThread,
                               this,
                               &validMoves[i + iterativeBranches],
                               depth,
                               alpha,
                               beta);
    }

    for (size_t i = 0; i < validMoves.size() - iterativeBranches; ++i) {
        Sint32 score = scoreFuture[i].get();
        if (score > alpha) {
            alpha = score;
            _saveBestMove(validMoves[i + iterativeBranches]);
        }
    }

    return alpha;
}
