#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint8_t Entity;

typedef struct {
  size_t num_components, num_entities, max_entities, next_index;
  size_t component_sizes[sizeof(Entity) * 8 - 1];
  void   *components[sizeof(Entity) * 8 - 1];
  Entity *entities;
}
ECS;

ECS  *ecs_init(size_t num_c, size_t *size_c);
void ecs_free(ECS *ecs);

size_t ecs_create_entity(ECS *ecs);
void   ecs_destroy_entity(ECS *ecs, size_t e);

void ecs_get_entities(ECS *ecs, size_t *num_e, size_t *max_e);

int  ecs_alive(ECS *ecs, size_t e);
int  ecs_has_component(ECS *ecs, size_t e, size_t c);
void *ecs_add_component(ECS *ecs, size_t e, size_t c);
void ecs_remove_component(ECS *ecs, size_t e, size_t c);
void *ecs_get_component(ECS *ecs, size_t e, size_t c);

