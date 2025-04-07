#pragma once

#include "ecs.h"

#include <stddef.h>

typedef struct {
  int left, right, up, down;
}
Input;

typedef struct {
  size_t w, h;
  Input in;
  ECS *ecs;
  int *brick_ids;

  float plat_speed, plat_accel, plat_fric;
  float grav_jump, grav_fall, jump_force, init_jump_force;
  float timer_jump, timer_init_jump, timer_coyote;
}
Scene;

char *level_load(const char *file, size_t *w, size_t *h);

Scene *scene_init(const char *bricks, size_t w, size_t h);
void scene_free(Scene *s);
void scene_update(Scene *s, float dt, float ct);
void scene_render(Scene *s, float dt, float ct);
void scene_input_key(Scene *s, int key, int pressed);
