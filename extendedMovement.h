#include <algorithm>

#include "move.h"

class extendedMovement : public movement {
   public:
    Uint8 distance;
    Uint8 score;

    extendedMovement(movement& mv) : movement(mv) {
        distance = std::max(abs(mv.nx - mv.ox), abs(mv.ny - mv.oy));
        score = 0;
    }
    extendedMovement(const Uint8 oldx,
                     const Uint8 oldy,
                     const Uint8 newx,
                     const Uint8 newy)
        : movement(oldx, oldy, newx, newy) {
        distance = std::max(abs(newx - oldx), abs(newy - oldy));
        score = 0;
    }
};
