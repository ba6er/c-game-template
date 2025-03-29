#include "scene.h"
#include "ecs.h"
#include "game.h"
#include "util.h"
#include <stdint.h>

typedef struct {
  int left, right, up, down;
}
Input;

static Input in;

typedef enum {
  ETag_Player = 1 >> 0,
  ETag_Wall   = 1 >> 1,
}
ETag;

typedef struct {
  uint64_t tags;
}
C_Tag;

typedef struct {
  float x, y;
}
C_Pos, C_Vel;

typedef struct {
  const char *spr;
  float sx, sy, rot;
} C_Sprite;

typedef enum {
  CE_Tag,
  CE_Pos,
  CE_Vel,
  CE_Sprite,
  CE_Count
}
Component;

static void scene_create_entity(uint64_t *i_tags, C_Pos *i_pos, C_Vel *i_vel, C_Sprite *i_spr);

void
scene_init()
{
  DEBUG_TRACE("Scene init begin");

  size_t cs[CE_Count] = {
    [CE_Tag]    = sizeof(C_Tag),
    [CE_Pos]    = sizeof(C_Pos),
    [CE_Vel]    = sizeof(C_Vel),
    [CE_Sprite] = sizeof(C_Sprite),
  };
  ecs_init(CE_Count, cs);

  // Player
  {
    size_t p_tags = ETag_Player;
    C_Pos p_pos  = {80, 160};
    C_Sprite p_sprite = {"plr_s", 1, 1, 0};
    scene_create_entity(&p_tags, &p_pos, NULL, &p_sprite);
  }

  // Bricks
  const char bricks[15][20] = {
    "R..................L",
    "R..................L",
    "R..................L",
    "R..................L",
    "R........LR........L",
    "R........LR........L",
    "R........LCCCR.....L",
    "R..................L",
    "R..................L",
    "CCCR...............L",
    "R..............LR..L",
    "R..................L",
    "R.....LCCCR........L",
    "R..................L",
    "CCCCCCCCCCCCCCCCCCCC",
  };
  for (size_t i = 0; i < 15; i++)
  {
    for (size_t j = 0; j < 20; j++)
    {
      if (bricks[i][j] == '.')
      {
        continue;
      }

      size_t b_tags = ETag_Wall;
      C_Pos b_pos = {j * 16 + 8, i * 16 + 8};
      C_Sprite b_sprite = {NULL, 1, 1, 0};
      switch (bricks[i][j])
      {
      case 'C':
        b_sprite.spr = "brick_c";
        break;
      case 'L':
        b_sprite.spr = "brick_l";
        break;
      case 'R':
        b_sprite.spr = "brick_r";
        break;
      }

      scene_create_entity(&b_tags, &b_pos, NULL, &b_sprite);
    }
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
  size_t num_e = 0, max_e = 0, num_iter = 0;
  ecs_get_entities(&num_e, &max_e);

  for (size_t e = 0; e < max_e && num_iter < num_e; e++)
  {
    if (ecs_alive(e) == 0)
    {
      continue;
    }
    num_iter++;

    if (ecs_has_component(e, CE_Tag) == 0)
    {
      DEBUG_ERROR("Entity %ld has no tags!", e);
      continue;
    }

    
  }
}

void
scene_render(float dt, float ct)
{
  size_t num_e = 0, max_e = 0, num_iter = 0;
  ecs_get_entities(&num_e, &max_e);

  for (size_t e = 0; e < max_e && num_iter < num_e; e++)
  {
    if (ecs_alive(e) == 0)
    {
      continue;
    }
    num_iter++;

    C_Pos *p = ecs_get_component(e, CE_Pos);
    C_Sprite *s = ecs_get_component(e, CE_Sprite);

    game_draw_sprite(s->spr, p->x, p->y, s->sx, s->sy, s->rot);
  }

  game_draw_text("font0",
                 "Press arrow keys to move around",
                 160, 40, 0.5f, 0.5f, FontDraw_Center, FontDraw_Center);
}

void
scene_input_key(int key, int pressed)
{
  switch (key)
  {
  case SDLK_LEFT:
    in.left = pressed;
    break;
  case SDLK_RIGHT:
    in.right = pressed;
    break;
  case SDLK_UP:
    in.up = pressed;
    break;
  case SDLK_DOWN:
    in.down = pressed;
    break;
  }
}

static void
scene_create_entity(uint64_t *i_tags, C_Pos *i_pos, C_Vel *i_vel, C_Sprite *i_spr)
{
  size_t e = ecs_create_entity();

  if (i_tags != NULL)
  {
    C_Tag *t = ecs_add_component(e, CE_Tag);
    t->tags |= *i_tags;
  }
  if (i_pos != NULL)
  {
    C_Pos *p = ecs_add_component(e, CE_Pos);
    p->x = i_pos->x;
    p->y = i_pos->y;
  }
  if (i_vel != NULL)
  {
    C_Vel *v = ecs_add_component(e, CE_Vel);
    v->x = i_vel->x;
    v->y = i_vel->y;
  }
  if (i_spr != NULL)
  {
    C_Sprite *s = ecs_add_component(e, CE_Sprite);
    s->spr = i_spr->spr;
    s->rot = i_spr->rot;
    s->sx = i_spr->sx;
    s->sy = i_spr->sy;
  }
}

