#include "scene.h"
#include "ecs.h"
#include "game.h"
#include "util.h"
#include <stdint.h>

typedef struct {
  int left, right, up, down;
}
Input;

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
  float x, y, ox, oy;
}
C_Size;

typedef struct {
  float speed, acc, fric, gr_j, gr_f, jump, jumpi, tt_j, tt_jo, tt_ct;

  int can_jump;
  float jump_timer, coyote_timer;
}
C_Plat;

typedef struct {
  const char *spr;
  float sx, sy, rot;
} C_Spr;

typedef enum {
  CE_Tag,
  CE_Pos,
  CE_Vel,
  CE_Size,
  CE_Plat,
  CE_Spr,
  CE_Count
}
Component;

static size_t scene_create_entity(C_Tag *i_tag, C_Pos *i_pos, C_Vel *i_vel, C_Size *i_size, C_Spr *i_spr);

static Input in;
static int brick_ids[15][20];

void
scene_init()
{
  DEBUG_TRACE("Scene init begin");

  size_t cs[CE_Count] = {
    [CE_Tag]  = sizeof(C_Tag),
    [CE_Pos]  = sizeof(C_Pos),
    [CE_Vel]  = sizeof(C_Vel),
    [CE_Size] = sizeof(C_Size),
    [CE_Plat] = sizeof(C_Plat),
    [CE_Spr]  = sizeof(C_Spr),
  };
  ecs_init(CE_Count, cs);

  // Player
  for (size_t i = 0; i < 20000; i++)
  {
    C_Tag p_tag = {ETag_Player};
    C_Pos p_pos = {rand() % 256 + 32, 40}; // {80, 80};
    C_Vel p_vel = {0};
    C_Size p_size = {
      .x  = 10,
      .y  = 14,
      .ox = 0,
      .oy = 1
    };
    C_Spr p_sprite = {
      .spr = "plr_s",
      .sx  = 1,
      .sy  = 1,
      .rot = 0,
    };
    size_t p = scene_create_entity(&p_tag, &p_pos, &p_vel, &p_size, &p_sprite);
    C_Plat *pl = ecs_add_component(p, CE_Plat);
    *pl = (C_Plat){
      .speed = 100 + rand() % 200, // 160,
      .acc   = 100 + rand() % 900, // 700,
      .fric  = 100 + rand() % 900, // 900,
      .gr_j  = 600 + rand() % 100, // 650,
      .gr_f  = 800 + rand() % 200, // 980,
      .jump  = 200 + rand() % 100, // 240,
      .jumpi = 100 + rand() % 100, // 150,
      .tt_j  = 0.18f,
      .tt_jo = 0.05f,
      .tt_ct = 0.05f,
    };
  }

  // Bricks
  const char bricks[15][20] = {
    "CCCCCCCCCCCCCCCCCCCC",
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
    "R..............LR..L",
    "CCCCCCCCCCCCCCCCCCCC",
  };
  for (size_t i = 0; i < 15; i++)
  {
    for (size_t j = 0; j < 20; j++)
    {
      if (bricks[i][j] == '.')
      {
        brick_ids[i][j] = -1;
        continue;
      }

      C_Tag b_tag = {ETag_Wall};
      C_Pos b_pos = {j * 16 + 8, i * 16 + 8};
      C_Spr b_sprite = {NULL, 1, 1, 0};
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

      int b = scene_create_entity(&b_tag, &b_pos, NULL, NULL, &b_sprite);
      brick_ids[i][j] = b;
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

    C_Tag *et = ecs_get_component(e, CE_Tag);

    // Player
    if (et->tags & ETag_Player)
    {
      C_Pos *pp = ecs_get_component(e, CE_Pos);
      C_Vel *pv = ecs_get_component(e, CE_Vel);
      C_Size *ps = ecs_get_component(e, CE_Size);
      C_Plat *pl = ecs_get_component(e, CE_Plat);

      float h_dest = (in.right - in.left) * pl->speed;
      float h_step = h_dest ? pl->acc : pl->fric;
      pv->x = move_toward(pv->x, h_dest, h_step * dt);

      pv->y += pv->y > 0 ? pl->gr_f * dt : pl->gr_j * dt;
      if (in.up && pl->can_jump)
      {
        pv->y = pl->jump_timer <= pl->tt_jo ? -pl->jumpi : -pl->jump;
        if (pl->coyote_timer < pl->tt_ct)
        {
          pl->jump_timer = 0;
        }
        pl->coyote_timer = pl->tt_ct;
      }

      // Horzontal and vertical movement for collision has to be handled separately
      int xi_hl = (int)((pp->x - (ps->x - ps->ox + 0.5f) / 2 + pv->x * dt) / 16);
      int xi_hr = (int)((pp->x + (ps->x + ps->ox + 0.5f) / 2 + pv->x * dt) / 16);
      int yi_hu = (int)((pp->y - (ps->y - ps->oy + 0.5f) / 2) / 16);
      int yi_hd = (int)((pp->y + (ps->y + ps->oy + 0.5f) / 2) / 16);
      int xi_vl = (int)((pp->x - (ps->x - ps->ox + 0.5f) / 2) / 16);
      int xi_vr = (int)((pp->x + (ps->x + ps->ox + 0.5f) / 2) / 16);
      int yi_vu = (int)((pp->y - (ps->y - ps->oy + 0.5f) / 2 + pv->y * dt) / 16);
      int yi_vd = (int)((pp->y + (ps->y + ps->oy + 0.5f) / 2 + pv->y * dt) / 16);

      if (xi_hl < 0  || xi_hr < 0  || yi_hu < 0  || yi_hd < 0  ||
          xi_vl < 0  || xi_vr < 0  || yi_vu < 0  || yi_vd < 0  ||
          xi_hl > 19 || xi_hr > 19 || yi_hu > 14 || yi_hd > 14 ||
          xi_vl > 19 || xi_vr > 19 || yi_vu > 14 || yi_vd > 14)
      {
        DEBUG_WARNING("Player outside bounds");
        break;
      }

      int col_l = (brick_ids[yi_hu][xi_hl] != -1 || brick_ids[yi_hd][xi_hl] != -1);
      int col_r = (brick_ids[yi_hu][xi_hr] != -1 || brick_ids[yi_hd][xi_hr] != -1);
      int col_u = (brick_ids[yi_vu][xi_vl] != -1 || brick_ids[yi_vu][xi_vr] != -1);
      int col_d = (brick_ids[yi_vd][xi_vl] != -1 || brick_ids[yi_vd][xi_vr] != -1);

      if ((pv->x < 0 && col_l) || (pv->x > 0 && col_r))
      {
        pv->x = 0;
      }
      if ((pv->y < 0 && col_u) || col_d)
      {
        pp->y -= col_d * pv->y * dt;
        pv->y = 0;
      }

      // Update coyote and jump timers and update if player can jump
      pl->jump_timer += dt;
      pl->coyote_timer += dt;
      if (in.up == 0)
      {
        pl->jump_timer = pl->tt_j;
      }
      if (col_d)
      {
        pl->jump_timer = 0;
        pl->coyote_timer = 0;
      }
      pl->can_jump = col_d || pl->jump_timer < pl->tt_j || pl->coyote_timer < pl->tt_ct;
    }

    // Update position with velocity
    if (ecs_has_component(e, CE_Pos) && ecs_has_component(e, CE_Vel))
    {
      C_Vel *ep = ecs_get_component(e, CE_Pos);
      C_Vel *ev = ecs_get_component(e, CE_Vel);
      ep->x += ev->x * dt;
      ep->y += ev->y * dt;
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

    if (ecs_has_component(e, CE_Pos) == 0 || ecs_has_component(e, CE_Spr) == 0)
    {
      continue;
    }

    C_Pos *p = ecs_get_component(e, CE_Pos);
    C_Spr *s = ecs_get_component(e, CE_Spr);

    game_draw_sprite(s->spr, p->x, p->y, s->sx, s->sy, s->rot);
  }

  game_draw_text("font0",
                 "Press arrow keys to move around",
                 160, 32, 0.5f, 0.5f, 0.5f, 0.5f);
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

static size_t
scene_create_entity(C_Tag *i_tag, C_Pos *i_pos, C_Vel *i_vel, C_Size *i_size, C_Spr *i_spr)
{
  size_t e = ecs_create_entity();

  if (i_tag != NULL)
  {
    C_Tag *t = ecs_add_component(e, CE_Tag);
    t->tags |= i_tag->tags;
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
  if (i_size != NULL)
  {
    C_Size *s = ecs_add_component(e, CE_Size);
    s->x = i_size->x;
    s->y = i_size->y;
    s->ox = i_size->ox;
    s->oy = i_size->oy;
  }
  if (i_spr != NULL)
  {
    C_Spr *s = ecs_add_component(e, CE_Spr);
    s->spr = i_spr->spr;
    s->rot = i_spr->rot;
    s->sx = i_spr->sx;
    s->sy = i_spr->sy;
  }

  return e;
}

