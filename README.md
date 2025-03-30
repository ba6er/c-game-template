# C game template

A simple template for a game made with C.  
Uses SDL2 for windowing/rendering/sound/text.

## How to use

This template is dead-simple.
Things related to input and other system stuff can be modified in `game.c`.

We also provide a basic ECS which you can see and modify in `ecs.c`.
The template is scene-based, with each "scene" having it's own ECS.

## How to build

Build the project by running `make` or `run.sh` which also starts the game.  
Clean the build files wih `make clean`.  
If you want to disable debug info or optimize compiling, just modify the `Makefile`.
