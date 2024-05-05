#ifndef __MOVE_H
#define __MOVE_H
#include <SDL.h>

#include <algorithm>

#include "SDL_stdinc.h"

/** Move class
 */
struct movement {
    movement() { ox = oy = nx = ny = 0; }

    movement(const movement& mv)
        : ox(mv.ox),
          oy(mv.oy),
          nx(mv.nx),
          ny(mv.ny),
          distance(mv.distance),
          score(mv.score) {}

    movement(const Uint8 oldx,
             const Uint8 oldy,
             const Uint8 newx,
             const Uint8 newy)
        : ox(oldx), oy(oldy), nx(newx), ny(newy) {
        Uint8 dx = std::abs(nx - ox);
        Uint8 dy = std::abs(ny - oy);
        distance = std::max(dx, dy);
    }

    movement& operator=(const movement& mv) {
        ox = mv.ox;
        oy = mv.oy;
        nx = mv.nx;
        ny = mv.ny;
        return *this;
    }

    Uint8 ox;
    Uint8 oy;
    Uint8 nx;
    Uint8 ny;
    Uint8 distance;
    Uint8 score = 0;
};

#endif
