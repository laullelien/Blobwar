#ifndef __COMMON_H
#define __COMMON_H

#include <SDL.h>
#include <SDL_timer.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#ifdef SOUND
#include <SDL_mixer.h>
#endif
#include <SDL_image.h>

#include <list>
#include <utility>
#include <vector>

#include "move.h"

using namespace std;

// LIGHT should be defined in case the game is too heavy for your machine
#define LIGHT

// 1 animated frame every three frames
#define ANIMATIONSPEED 3

#ifndef LIGHT
// blobs are animated
#define ANIMATION
// mouse handled by SDL
#define SOFT_MOUSE
#endif

#endif