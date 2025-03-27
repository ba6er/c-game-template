#include "scene.h"
#include "ecs.h"
#include "game.h"
#include "util.h"

#include <stdlib.h>

typedef struct {
  float x, y;
}
Pos, Vel;

void
scene_init()
{
  DEBUG_TRACE("Scene init begin");

  size_t cs[2] = { sizeof(Pos), sizeof(Vel) };
  ecs_init(2, cs);

  for (size_t i = 0; i < 200; i++)
  {
    size_t e = ecs_create_entity();

    Pos *p = ecs_add_component(e, 0);
    p->x = rand() % 300 + 10;
    p->y = rand() % 220 + 10;

    Vel *v = ecs_add_component(e, 1);
    v->y = rand() % 40 - 20;
    v->x = rand() % 40 - 20;
  }

  DEBUG_TRACE("Scene init end");
}

void
scene_free()
{
  DEBUG_TRACE("Scene free");

  ecs_free();
}

void
scene_update(float dt, float ct)
{
  // Temporary update in render tick
}

void
scene_render(float dt, float ct)
{
  size_t num_e = 0, max_e = 0, num_iter = 0;
  ecs_get_entities(&num_e, &max_e);

  for (size_t e = 0; e < max_e && num_iter < num_e; e++)
  {
    if (!ecs_alive(e))
    {
      continue;
    }
    num_iter++;

    Pos *p = ecs_get_component(e, 0);
    Vel *v = ecs_get_component(e, 1);
    p->x += v->x * dt;
    p->y += v->y * dt;
    if (p->x > 320 || p->x < 0)
    {
      v->x *= -1;
    }
    if (p->y > 240 || p->y < 0)
    {
      v->y *= -1;
    }

    game_draw_sprite("player", p->x, p->y, e * e + ct * 90);
  }
}

