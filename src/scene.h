#pragma once

void scene_init();
void scene_free();
void scene_update(float dt, float ct);
void scene_render(float dt, float ct);
void scene_input_key(int key, int pressed);
