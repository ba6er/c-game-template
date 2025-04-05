#include "ecs.h"
#include "util.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static const size_t init_entities = 32;

ECS *
ecs_init(size_t num_c, size_t *size_c)
{
  ECS *ecs = malloc(sizeof(ECS));

  DEBUG_TRACE("ECS init begin");
  DEBUG_ASSERT(num_c < sizeof(Entity) * 8, "Too many components to init");

  ecs->num_components = num_c;

  ecs->max_entities = init_entities;
  ecs->num_entities = 0;
  ecs->next_index = 0;

  for (int i = 0; i < ecs->num_components; i++)
  {
    ecs->component_sizes[i] = size_c[i];

    ecs->components[i] = calloc(ecs->max_entities, ecs->component_sizes[i]);
    DEBUG_ASSERT(ecs->components[i], "Can't allocate space for components");
  }
  ecs->entities = calloc(ecs->max_entities, sizeof(Entity));
  DEBUG_ASSERT(ecs->entities, "Can't allocate space for entities");

  DEBUG_TRACE("ECS init end");

  return ecs;
}

void
ecs_free(ECS *ecs)
{
  DEBUG_TRACE("ECS free");

  for (int i = 0; i < ecs->num_components; i++)
  {
    free(ecs->components[i]);
    ecs->component_sizes[i] = 0;
  }
  free(ecs->entities);

  free(ecs);
}

size_t
ecs_create_entity(ECS *ecs)
{
  if (ecs->num_entities >= ecs->max_entities)
  {
    for (int i = 0; i < ecs->num_components; i++)
    {
      void *new_component = calloc(ecs->max_entities * 2, ecs->component_sizes[i]);
      DEBUG_ASSERT(new_component, "Can't reallocate space for components");

      memcpy(new_component, ecs->components[i], ecs->max_entities * ecs->component_sizes[i]);
      free(ecs->components[i]);
      ecs->components[i] = new_component;
    }
    void *new_entities = calloc(ecs->max_entities * 2, sizeof(Entity));
    DEBUG_ASSERT(new_entities, "Can't reallocate space for entities");

    memcpy(new_entities, ecs->entities, ecs->max_entities * sizeof(Entity));
    free(ecs->entities);
    ecs->entities = new_entities;

    ecs->max_entities *= 2;
  }

  while (ecs->entities[ecs->next_index])
  {
    ecs->next_index = (ecs->next_index + 1) % ecs->max_entities;
  }
  ecs->num_entities++;

  ecs->entities[ecs->next_index] = 1;
  return ecs->next_index;
}

void
ecs_get_entities(ECS *ecs, size_t *num_e, size_t *max_e)
{
  *num_e = ecs->num_entities;
  *max_e = ecs->max_entities;
}

void
ecs_destroy_entity(ECS *ecs, size_t e)
{
  if (ecs->entities[e] == 0)
  {
    ERROR_RETURN(, "No entity to destroy at index %ld", e);
  }

  ecs->entities[e] = 0;
  ecs->num_entities--;
}

int
ecs_alive(ECS *ecs, size_t e)
{
  return ecs->entities[e] != 0;
}

int
ecs_has_component(ECS *ecs, size_t e, size_t c)
{
  return (ecs->entities[e] & (2 << c)) != 0;
}

void *
ecs_add_component(ECS *ecs, size_t e, size_t c)
{
  // First bit is for checking if alive
  // Maybe redundant, but technically an entity can have no components
  ecs->entities[e] |= (2 << c);
  return (int8_t *)ecs->components[c] + e * ecs->component_sizes[c];
}

void
ecs_remove_component(ECS *ecs, size_t e, size_t c)
{
  ecs->entities[e] &= ~(2 << c);
}

void *
ecs_get_component(ECS *ecs, size_t e, size_t c)
{
  return (int8_t *)ecs->components[c] + e * ecs->component_sizes[c];
}
