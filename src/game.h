#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

typedef struct {
  const char *key;
  const char *file;
}
TextureSource;

typedef struct {
  const char *key;
  const char *tex;
  SDL_Rect rect;
}
SpriteSource;

typedef struct {
  const char *key;
  const char *file;
  int ptsize, smooth;
}
FontSource;

typedef struct {
  const char *key;
  const char *file;
}
AudioSource;

void game_init_system(int ww, int wh, int lw, int lh, const char *title);
void game_init_assets(TextureSource *t_src, size_t t_size,
                      SpriteSource  *s_src, size_t s_size,
                      FontSource    *f_src, size_t f_size,
                      AudioSource   *a_src, size_t a_size);
void game_init_scene();
void game_run(int tick_rate);
void game_free();

void game_draw_sprite(const char *sprite, float x, float y, float sx, float sy, float a);
void game_draw_text(const char *font, const char *text, float x, float y, float sx, float sy, float ox, float oy);
void game_play_audio(const char *aud, int loops);
