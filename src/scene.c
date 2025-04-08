#include "scene.h"
#include "game.h"
#include "util.h"

#include <stdint.h>

typedef enum {
  ETag_Player = 1 >> 0,
  ETag_Wall   = 1 >> 1,
  ETag_Coin   = 1 >> 2,
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
  int can_jump;
  float timer_jump, timer_coyote;
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

static size_t scene_create_entity(Scene *s, C_Tag *i_tag, C_Pos *i_pos, C_Vel *i_vel, C_Size *i_size, C_Spr *i_spr);
static void scene_update_player(Scene *s, size_t e, float dt);

uint8_t *
level_load(const char *file, size_t *width, size_t *height)
{
  size_t w, h;

  File f = file_read(file);
  sscanf(f.data, "%lu %lu", &w, &h);

  size_t data_offset = 0;
  for (; f.data[data_offset - 1] != '\n'; data_offset++);

  uint8_t *brick_data = malloc(w * h * sizeof(uint8_t));
  for (int i = 0; i < (w + 1) * h; i++)
  {
    if (i % (w + 1) == w)
    {
      continue;
    }
    brick_data[i - (size_t)(i / (w + 1))] = f.data[data_offset + i];
  }

  *width = w;
  *height = h;

  file_free(f);
  return brick_data;
}

void
level_write(const char *file, size_t w, size_t h, uint8_t *data)
{
  FILE *f = fopen(file, "wb");
  if (f == NULL)
  {
    ERROR_RETURN(, "Can't open file %s", file);
  }

  fwrite(&w, sizeof(size_t), 1, f);
  fwrite(&h, sizeof(size_t), 1, f);
  fwrite(data, sizeof(uint8_t), w * h, f);

  fclose(f);
}

Scene *
scene_init(uint8_t *bricks, size_t w, size_t h)
{
  Scene *scene = calloc(sizeof(Scene), 1);
  scene->w = w;
  scene->h = h;

  scene->plat_speed   = 160.0f;
  scene->plat_accel   = 700.0f;
  scene->plat_fric    = 900.0f;
  scene->grav_jump    = 650.0f;
  scene->grav_fall    = 980.0f;
  scene->jump_bottom  = 80.0f;
  scene->jump_top     = 250.0f;
  scene->timer_jump   = 0.18f;
  scene->timer_coyote = 0.05f;

  DEBUG_TRACE("Scene init begin");

  size_t cs[CE_Count] = {
    [CE_Tag]  = sizeof(C_Tag),
    [CE_Pos]  = sizeof(C_Pos),
    [CE_Vel]  = sizeof(C_Vel),
    [CE_Size] = sizeof(C_Size),
    [CE_Plat] = sizeof(C_Plat),
    [CE_Spr]  = sizeof(C_Spr),
  };
  scene->ecs = ecs_init(CE_Count, cs);

  // Player
  {
    C_Tag p_tag = {ETag_Player};
    C_Pos p_pos = {80, 80};
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
    size_t p = scene_create_entity(scene, &p_tag, &p_pos, &p_vel, &p_size, &p_sprite);
    ecs_add_component(scene->ecs, p, CE_Plat);
  }

  int *brick_ids = calloc(w * h, sizeof(int));

  // Bricks
  for (size_t y = 0; y < h; y++)
  {
    for (size_t x = 0; x < w; x++)
    {
      if (bricks[y * w + x] == '.')
      {
        brick_ids[y * w + x] = -1;
        continue;
      }

      C_Tag b_tag = {ETag_Wall};
      C_Pos b_pos = {x * 16 + 8, y * 16 + 8};
      C_Spr b_sprite = {NULL, 1, 1, 0};
      switch (bricks[y * w + x])
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

      int b = scene_create_entity(scene, &b_tag, &b_pos, NULL, NULL, &b_sprite);
      brick_ids[y * w + x] = b;
    }
  }
  scene->brick_ids = brick_ids;

  DEBUG_TRACE("Scene init end");

  return scene;
}

void
scene_free(Scene *s)
{
  DEBUG_TRACE("Scene free");

  ecs_free(s->ecs);
  free(s->brick_ids);
  free(s);
}

void
scene_update(Scene *s, float dt, float ct)
{
  size_t num_e = 0, max_e = 0, num_iter = 0;
  ecs_get_entities(s->ecs, &num_e, &max_e);

  for (size_t e = 0; e < max_e && num_iter < num_e; e++)
  {
    if (ecs_alive(s->ecs, e) == 0)
    {
      continue;
    }
    num_iter++;

    if (ecs_has_component(s->ecs, e, CE_Tag) == 0)
    {
      DEBUG_ERROR("Entity %ld has no tags!", e);
      continue;
    }

    C_Tag *et = ecs_get_component(s->ecs, e, CE_Tag);

    // Player
    if (et->tags & ETag_Player)
    {
      scene_update_player(s, e, dt);
    }

    // Update position with velocity
    if (ecs_has_component(s->ecs, e, CE_Pos) && ecs_has_component(s->ecs, e, CE_Vel))
    {
      C_Vel *ep = ecs_get_component(s->ecs, e, CE_Pos);
      C_Vel *ev = ecs_get_component(s->ecs, e, CE_Vel);
      ep->x += ev->x * dt;
      ep->y += ev->y * dt;
    }
  }
}

void
scene_render(Scene *s, float dt, float ct)
{
  size_t num_e = 0, max_e = 0, num_iter = 0;
  ecs_get_entities(s->ecs, &num_e, &max_e);

  for (size_t e = 0; e < max_e && num_iter < num_e; e++)
  {
    if (ecs_alive(s->ecs, e) == 0)
    {
      continue;
    }
    num_iter++;

    if (ecs_has_component(s->ecs, e, CE_Pos) == 0 || ecs_has_component(s->ecs, e, CE_Spr) == 0)
    {
      continue;
    }

    C_Pos *ep = ecs_get_component(s->ecs, e, CE_Pos);
    C_Spr *es = ecs_get_component(s->ecs, e, CE_Spr);

    game_draw_sprite(es->spr, ep->x, ep->y, es->sx, es->sy, es->rot);
  }

  game_draw_text("font0",
                 "Press arrow keys to move around",
                 160, 32, 0.5f, 0.5f, 0.5f, 0.5f);
}

void
scene_input_key(Scene *s, int key, int pressed)
{
  switch (key)
  {
  case SDLK_LEFT:
    s->in.left = pressed;
    break;
  case SDLK_RIGHT:
    s->in.right = pressed;
    break;
  case SDLK_UP:
    s->in.up = pressed;
    break;
  case SDLK_DOWN:
    s->in.down = pressed;
    break;
  }
}

static size_t
scene_create_entity(Scene *s, C_Tag *i_tag, C_Pos *i_pos, C_Vel *i_vel, C_Size *i_size, C_Spr *i_spr)
{
  size_t e = ecs_create_entity(s->ecs);

  if (i_tag != NULL)
  {
    C_Tag *t = ecs_add_component(s->ecs, e, CE_Tag);
    t->tags |= i_tag->tags;
  }
  if (i_pos != NULL)
  {
    C_Pos *p = ecs_add_component(s->ecs, e, CE_Pos);
    p->x = i_pos->x;
    p->y = i_pos->y;
  }
  if (i_vel != NULL)
  {
    C_Vel *v = ecs_add_component(s->ecs, e, CE_Vel);
    v->x = i_vel->x;
    v->y = i_vel->y;
  }
  if (i_size != NULL)
  {
    C_Size *sz = ecs_add_component(s->ecs, e, CE_Size);
    sz->x = i_size->x;
    sz->y = i_size->y;
    sz->ox = i_size->ox;
    sz->oy = i_size->oy;
  }
  if (i_spr != NULL)
  {
    C_Spr *sp = ecs_add_component(s->ecs, e, CE_Spr);
    sp->spr = i_spr->spr;
    sp->rot = i_spr->rot;
    sp->sx = i_spr->sx;
    sp->sy = i_spr->sy;
  }

  return e;
}

static void
scene_update_player(Scene *s, size_t plr, float dt)
{
  C_Pos *pp = ecs_get_component(s->ecs, plr, CE_Pos);
  C_Vel *pv = ecs_get_component(s->ecs, plr, CE_Vel);
  C_Size *ps = ecs_get_component(s->ecs, plr, CE_Size);
  C_Plat *pl = ecs_get_component(s->ecs, plr, CE_Plat);

  float h_dest = (s->in.right - s->in.left) * s->plat_speed;
  float h_step = h_dest ? s->plat_accel : s->plat_fric;
  pv->x = lerp(pv->x, h_dest, h_step * dt);

  pv->y += (pv->y > 0 ? s->grav_fall : s->grav_jump) * dt ;
  if (s->in.up && pl->can_jump)
  {
    float jump_linear = sqrtf(pl->timer_jump / s->timer_jump);
    pv->y = -(s->jump_top * jump_linear + s->jump_bottom * (1 - jump_linear));
    if (pl->timer_coyote < s->timer_coyote)
    {
      pl->timer_jump = 0;
    }
    pl->timer_coyote = s->timer_coyote;
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

  if (xi_hl < 0    || xi_hr < 0    || yi_hu < 0    || yi_hd < 0    ||
      xi_vl < 0    || xi_vr < 0    || yi_vu < 0    || yi_vd < 0    ||
      xi_hl > s->w || xi_hr > s->w || yi_hu > s->h || yi_hd > s->h ||
      xi_vl > s->w || xi_vr > s->w || yi_vu > s->h || yi_vd > s->h)
  {
    DEBUG_WARNING("Player outside bounds");
    return;
  }

  int col_l = (s->brick_ids[yi_hu * s->w + xi_hl] != -1 ||
               s->brick_ids[yi_hd * s->w + xi_hl] != -1);
  int col_r = (s->brick_ids[yi_hu * s->w + xi_hr] != -1 ||
               s->brick_ids[yi_hd * s->w + xi_hr] != -1);
  int col_u = (s->brick_ids[yi_vu * s->w + xi_vl] != -1 ||
               s->brick_ids[yi_vu * s->w + xi_vr] != -1);
  int col_d = (s->brick_ids[yi_vd * s->w + xi_vl] != -1 ||
               s->brick_ids[yi_vd * s->w + xi_vr] != -1);

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
  pl->timer_jump += dt;
  pl->timer_coyote += dt;

  if (s->in.up == 0)
  {
    pl->timer_jump = s->timer_jump;
  }
  if (col_d)
  {
    pl->timer_jump = 0;
    pl->timer_coyote = 0;
  }
  pl->can_jump = col_d || (pl->timer_jump < s->timer_jump) || (pl->timer_coyote < s->timer_coyote);
}
