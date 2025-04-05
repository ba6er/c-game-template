#include "game.h"

int
main(int argc, char *argv[])
{
  const char *title = "Luk Adventures";
  int ww = 640, wh = 480;
  int lw = 320, lh = 240;

  int tick_rate = 300;

  // Sources MUST be in alphabetical order because of binary search

  // Textures
  TextureSource t_src[] = {
    {"ingame", "gfx/ingame.png"},
  };
  // Sprites
  SpriteSource s_src[] = {
    {"brick_c", "ingame", {16, 16, 16, 16}},
    {"brick_l", "ingame", {0,  16, 16, 16}},
    {"brick_r", "ingame", {32, 16, 16, 16}},
    {"plr_s",   "ingame", {0,  0,  16, 16}},
    {"plr_w0",  "ingame", {16, 0,  16, 16}},
    {"plr_w1",  "ingame", {32, 0,  16, 16}},
  };
  // Fonts
  FontSource f_src[] = {
    {"font0", "font/noto_serif.ttf", 28, 1},
  };
  // Audio
  AudioSource a_src[] = {
    {"explosion", "sfx/explosion.wav"},
  };

  game_init_system(ww, wh, lw, lh, title);
  game_init_assets(t_src, sizeof(t_src),
                   s_src, sizeof(s_src),
                   f_src, sizeof(f_src),
                   a_src, sizeof(a_src));
  game_init_scene();
  game_run(tick_rate);
  game_free();
  return 0;
}
