#pragma once

#include <stddef.h>
#include <stdint.h>

void ecs_init(size_t num_c, size_t *size_c);
void ecs_free();

size_t ecs_create_entity();
void   ecs_destroy_entity(size_t e);

void ecs_get_entities(size_t *num_e, size_t *max_e);

int  ecs_alive(size_t e);
int  ecs_has_component(size_t e, size_t c);
void *ecs_add_component(size_t e, size_t c);
void ecs_remove_component(size_t e, size_t c);
void *ecs_get_component(size_t e, size_t c);

