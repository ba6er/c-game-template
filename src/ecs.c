#include "ecs.h"
#include "util.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t Entity;

static size_t num_components, num_entities, max_entities, next_index;

static const size_t init_entities = 32;

static size_t component_sizes[sizeof(Entity) * 8 - 1];
static void   *components[sizeof(Entity) * 8 - 1];
static Entity *entities;

void
ecs_init(size_t num_c, size_t *size_c)
{
  DEBUG_TRACE("ECS init begin");
  DEBUG_ASSERT(num_c < sizeof(Entity) * 8, "Too many components to init");

  num_components = num_c;

  max_entities = init_entities;
  num_entities = 0;
  next_index = 0;

  for (int i = 0; i < num_components; i++)
  {
    component_sizes[i] = size_c[i];

    components[i] = calloc(max_entities, component_sizes[i]);
    DEBUG_ASSERT(components[i], "Can't allocate space for components");
  }
  entities = calloc(max_entities, sizeof(Entity));
  DEBUG_ASSERT(entities, "Can't allocate space for entities");

  DEBUG_TRACE("ECS init end");
}

void
ecs_free()
{
  DEBUG_TRACE("ECS free");

  for (int i = 0; i < num_components; i++)
  {
    free(components[i]);
    component_sizes[i] = 0;
  }
  free(entities);

  num_components = 0;
  num_entities = 0;
  max_entities = 0;
  next_index = 0;
}

size_t
ecs_create_entity()
{
  if (num_entities >= max_entities)
  {
    for (int i = 0; i < num_components; i++)
    {
      void *new_component = calloc(max_entities * 2, component_sizes[i]);
      DEBUG_ASSERT(new_component, "Can't reallocate space for components");

      memcpy(new_component, components[i], max_entities * component_sizes[i]);
      free(components[i]);
      components[i] = new_component;
    }
    void *new_entities = calloc(max_entities * 2, sizeof(Entity));
    DEBUG_ASSERT(new_entities, "Can't reallocate space for entities");

    memcpy(new_entities, entities, max_entities * sizeof(Entity));
    free(entities);
    entities = new_entities;

    max_entities *= 2;
  }

  while (entities[next_index])
  {
    next_index = (next_index + 1) % max_entities;
  }
  num_entities++;

  entities[next_index] = 1;
  return next_index;
}

void
ecs_get_entities(size_t *num_e, size_t *max_e)
{
  *num_e = num_entities;
  *max_e = max_entities;
}

void
ecs_destroy_entity(size_t e)
{
  if (entities[e] == 0)
  {
    ERROR_RETURN(, "No entity to destroy at index %ld", e);
  }

  entities[e] = 0;
  num_entities--;
}

int
ecs_alive(size_t e)
{
  return entities[e] != 0;
}

int
ecs_has_component(size_t e, size_t c)
{
  return (entities[e] & (2 << c)) != 0;
}

void *
ecs_add_component(size_t e, size_t c)
{
  // First bit is for checking if alive
  // Maybe redundant, but technically an entity can have no components
  entities[e] |= (2 << c);
  return (int8_t *)components[c] + e * component_sizes[c];
}

void
ecs_remove_component(size_t e, size_t c)
{
  entities[e] &= ~(2 << c);
}

void *
ecs_get_component(size_t e, size_t c)
{
  return (int8_t *)components[c] + e * component_sizes[c];
}
