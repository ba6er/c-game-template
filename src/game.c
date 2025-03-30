#include "game.h"
#include "scene.h"
#include "util.h"

#include <time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static SDL_Window   *window;
static SDL_Renderer *renderer;

static size_t      num_textures;
static const char  **tex_map;
static SDL_Texture **textures;

static size_t      num_sprites;
static const char  **spr_map;
static SDL_Rect    *sprites;
static size_t      *spr_ids;

static size_t      num_fonts;
static const char  **font_map;
static SDL_Texture **fonts;
static SDL_Rect    *font_rects;

static size_t     num_audio;
static const char **audio_map;
static Mix_Chunk  **audios;

void
game_init(int ww, int wh, int lw, int lh, const char *title)
{
  DEBUG_TRACE("System init start");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
  {
    DEBUG_ASSERT(0, "Can't init SDL! SDL_Error:\n%s", SDL_GetError());
  }
  window = SDL_CreateWindow(
    title,
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    ww,
    wh,
    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
  );
  if (window == NULL)
  {
    SDL_Quit();
    DEBUG_ASSERT(0, "Can't create window! SDL_Error:\n%s", SDL_GetError());
  }
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL)
  {
    DEBUG_WARNING("Switching to software renderer");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (renderer == NULL)
  {
    SDL_Quit();
    DEBUG_ASSERT(0, "Can't create renderer! SDL_Error:\n%s", SDL_GetError());
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
  SDL_RenderSetLogicalSize(renderer, lw, lh);

  if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
  {
    SDL_Quit();
    DEBUG_ASSERT(0, "Can't init SDL_image! IMG_Error:\n%s", IMG_GetError());
  }
  if (TTF_Init() == -1)
  {
    IMG_Quit();
    SDL_Quit();
    DEBUG_ASSERT(0, "Can't init SDL_ttf! TTF_Error:\n%s", TTF_GetError());
  }
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
  {
    DEBUG_ERROR("Can't init SDL_mixer! Mix_Error:\n%s", Mix_GetError());
  }

  srand(time(NULL));
  DEBUG_TRACE("System init end");
}

void
game_init_assets(TextureSource *t_src, size_t t_size,
                 SpriteSource  *s_src, size_t s_size,
                 FontSource    *f_src, size_t f_size,
                 AudioSource   *a_src, size_t a_size)
{
  DEBUG_TRACE("Asset init start");

  // Textures
  num_textures = t_size / sizeof(TextureSource);

  tex_map  = calloc(num_textures, sizeof(char *));
  textures = calloc(num_textures, sizeof(SDL_Texture *));

  if (tex_map == NULL || textures == NULL)
  {
    game_free();
    DEBUG_ASSERT(0, "Can't allocate space for textures!");
  }
  for (size_t i = 0; i < num_textures; i++)
  {
    tex_map[i] = t_src[i].key;
    textures[i] = IMG_LoadTexture(renderer, t_src[i].file);
    if (textures[i] == NULL)
    {
      DEBUG_ERROR("Can't load texture! SDL_Error:\n%s", SDL_GetError());
    }
  }

  // Sprites
  num_sprites = s_size / sizeof(SpriteSource);

  spr_map = calloc(num_sprites, sizeof(char *));
  sprites = calloc(num_sprites, sizeof(SDL_Rect));
  spr_ids = calloc(num_sprites, sizeof(size_t));

  if (spr_map == NULL || spr_ids == NULL || sprites == NULL)
  {
    game_free();
    DEBUG_ASSERT(0, "Can't allocate space for sprites!");
  }
  for (size_t i = 0; i < num_sprites; i++)
  {
    spr_map[i] = s_src[i].key;
    sprites[i] = s_src[i].rect;
    int tex_id = binary_search(tex_map, num_textures, s_src[i].tex);
    if (tex_id != -1)
    {
      spr_ids[i] = (size_t)tex_id;
    }
  }

  // Fonts
  num_fonts = f_size / sizeof(FontSource);

  font_map   = calloc(num_fonts, sizeof(char *));
  fonts      = calloc(num_fonts, sizeof(SDL_Texture *));
  font_rects = calloc(num_fonts * (127 - ' '), sizeof(SDL_Rect));

  if (font_map == NULL || fonts == NULL || font_rects == NULL)
  {
    game_free();
    DEBUG_ASSERT(0, "Can't allocate space for fonts!");
  }
  for (size_t i = 0; i < num_fonts; i++)
  {
    font_map[i] = f_src[i].key;
    TTF_Font *font = TTF_OpenFont(f_src[i].file, f_src[i].ptsize);
    if (font == NULL)
    {
      DEBUG_ERROR("Can't load font! TTF_Error:\n%s", TTF_GetError());
      continue;
    }

    int charset_width = 0, charset_height = 0;
    SDL_Surface *charset[127 - ' '];
    for (int ch = 0; ch < 127 - ' '; ch++)
    {
      if (f_src[i].smooth == 0)
      {
        charset[ch] = TTF_RenderGlyph_Solid(font, ch + ' ', (SDL_Color){255, 255, 255, 255});
      }
      else
      {
        charset[ch] = TTF_RenderGlyph_Blended(font, ch + ' ', (SDL_Color){255, 255, 255, 255});
      }
      font_rects[i * (127 - ' ') + ch] = (SDL_Rect){charset_width, 0, charset[ch]->w, charset[ch]->h};

      charset_width += charset[ch]->w;
    }
    charset_height = charset[0]->h;

    SDL_Surface *charset_full = SDL_CreateRGBSurface(
      0,
      charset_width,
      charset_height,
      32,
      0xff,
      0xff00,
      0xff0000,
      0xff000000
    );

    int char_offset = 0;
    for (int ch = 0; ch < 127 - ' '; ch++)
    {
      SDL_Rect dest_rect = {char_offset, 0, charset[ch]->w, charset_height};
      SDL_BlitSurface(charset[ch], NULL, charset_full, &dest_rect);

      SDL_FreeSurface(charset[ch]);
      char_offset += dest_rect.w;
    }
    fonts[i] = SDL_CreateTextureFromSurface(renderer, charset_full);
    if (fonts[i] == NULL)
    {
      DEBUG_ERROR("Can't load charset texture! SDL_Error:\n%s", SDL_GetError());
    }
    SDL_SetTextureBlendMode(fonts[i], SDL_BLENDMODE_ADD);
    TTF_CloseFont(font);
    SDL_FreeSurface(charset_full);
  }

  // Audio
  num_audio = a_size / sizeof(AudioSource);

  audio_map = calloc(num_audio, sizeof(char *));
  audios    = calloc(num_audio, sizeof(Mix_Chunk *));

  if (audio_map == NULL || audios == NULL)
  {
    game_free();
    DEBUG_ASSERT(0, "Can't allocate space for audio!");
  }
  for (size_t i = 0; i < num_audio; i++)
  {
    audio_map[i] = a_src[i].key;
    audios[i] = Mix_LoadWAV(a_src[i].file);
    if (audios[i] == NULL)
    {
      DEBUG_ERROR("Can't load audio! Mix_Error:\n%s", Mix_GetError());
    }
  }

  DEBUG_TRACE("Asset init end");

  scene_init();
}

void
game_run(int tick_rate)
{
  uint64_t tick_counter = 0;

  float tick_time     = 1.0f / tick_rate;
  float current_time  = 0.0f;
  float previous_time = 0.0f;
  float delta_time    = 0.0f;
  float lag_time      = 0.0f;

  int is_running = 1;
  while (is_running)
  {
    // Time
    current_time  = (float)SDL_GetTicks() / 1000.0f;
    delta_time    = current_time - previous_time;
    previous_time = current_time;
    lag_time     += delta_time;

    // Events and input
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        is_running = 0;
      }
      if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
      {
        if (event.key.repeat)
        {
          break;
        }
        scene_input_key(event.key.keysym.sym, event.key.state);
      }
    }

    // Update
    while (lag_time >= tick_time)
    {
      scene_update(tick_time, current_time);

      lag_time -= tick_time;
      tick_counter++;
    }

    // Render
    SDL_SetRenderDrawColor(renderer, 0x40, 0x80, 0xd0, 0xff);
    SDL_RenderClear(renderer);

    scene_render(delta_time, current_time);

    SDL_RenderPresent(renderer);
  }
}

void
game_free()
{
  scene_free();

  DEBUG_TRACE("Asset free");

  for (size_t i = 0; i < num_textures; i++)
  {
    SDL_DestroyTexture(textures[i]);
  }
  for (size_t i = 0; i < num_fonts; i++)
  {
    SDL_DestroyTexture(fonts[i]);
  }
  for (size_t i = 0; i < num_audio; i++)
  {
    Mix_FreeChunk(audios[i]);
  }

  free(tex_map);
  free(textures);

  free(spr_map);
  free(sprites);
  free(spr_ids);

  free(font_map);
  free(fonts);
  free(font_rects);

  free(audio_map);
  free(audios);

  DEBUG_TRACE("System free");

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  Mix_Quit();
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
}

void
game_draw_sprite(const char *sprite, float x, float y, float sx, float sy, float a)
{
  if (sprite == NULL)
  {
    ERROR_RETURN(, "No sprite provided!");
  }

  int si = binary_search(spr_map, num_sprites, sprite);
  if (si == -1)
  {
    ERROR_RETURN(, "Can't find sprite: %s", sprite);
  }

  SDL_FRect dest = {
    x - sprites[si].w / 2.0 * sx,
    y - sprites[si].h / 2.0 * sy,
    sprites[si].w * sx,
    sprites[si].h * sy,
  };
  SDL_RenderCopyExF(renderer, textures[spr_ids[si]], &sprites[si], &dest, a, NULL, SDL_FLIP_NONE);
}


void
game_draw_text(const char *font, const char *text, float x, float y, float sx, float sy, FontDraw h, FontDraw v)
{
  if (font == NULL)
  {
    ERROR_RETURN(, "No font provided!");
  }

  int fi = binary_search(font_map, num_fonts, font);
  if (fi == -1)
  {
    ERROR_RETURN(, "Can't find font: %s", font);
  }

  int offset_x = 0, offset_y = 0;

  if (h == FontDraw_Right || h == FontDraw_Center)
  {
    for (size_t i = 0; text[i]; i++)
    {
      size_t ri = fi * (127 - ' ') + text[i] - ' ';
      offset_x -= font_rects[ri].w;
    }
  }
  if (h == FontDraw_Center)
  {
    offset_x /= 2;
  }

  if (v == FontDraw_Bottom || h == FontDraw_Center)
  {
    SDL_QueryTexture(fonts[fi], NULL, NULL, NULL, &offset_y);
    offset_y *= -1;
  }
  if (v == FontDraw_Center)
  {
    offset_y /= 2;
  }

  offset_x *= sx;
  offset_y *= sy;

  for (size_t i = 0; text[i]; i++)
  {
    size_t ri = fi * (127 - ' ') + text[i] - ' ';
    SDL_FRect dest = {
      x + offset_x,
      y + offset_y,
      font_rects[ri].w * sx,
      font_rects[ri].h * sy,
    };
    SDL_RenderCopyF(renderer, fonts[fi], &font_rects[ri], &dest);

    offset_x += dest.w;
  }
}

void
game_play_audio(const char *aud, int loops)
{
  if (aud == NULL)
  {
    ERROR_RETURN(, "No audio provided!");
  }

  int ai = binary_search(audio_map, num_audio, aud);
  if (ai == -1)
  {
    ERROR_RETURN(, "Can't find audio: %s", aud);
  }

  Mix_PlayChannel(-1, audios[ai], loops);
}
