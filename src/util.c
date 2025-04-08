#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int
binary_search(const char **arr, size_t n, const char *target)
{
  size_t low = 0;
  size_t high = n;
  while (low < high)
  {
    size_t mid = (low + high) / 2;
    int comp = strcmp(arr[mid], target);
    if (comp == 0)
    {
      return mid;
    }
    if (comp > 0)
    {
      high = mid; // Look in 1st half
    }
    else
    {
      low = mid + 1; // Look in 2nd half
    }
  }
  ERROR_RETURN(-1, "Failed to find target %s", target);
}

float
lerp(float start, float dest, float step)
{
  if (start == dest)
  {
    return start;
  }

  if (start < dest)
  {
    if (dest - start <= step)
    {
      return dest;
    }
    return start + step;
  }
  else
  {
    if (start - dest <= step)
    {
      return dest;
    }
    return start - step;
  }
}
