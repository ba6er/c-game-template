#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

File
file_read(const char *filename)
{
  File file = {0};

  if (!filename)
  {
    ERROR_RETURN(file, "No filename provided");
  }

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL)
  {
    ERROR_RETURN(file, "Invalid filename %s", filename);
  }

  struct stat file_stat;
  stat(filename, &file_stat);
  if (file_stat.st_size < 0)
  {
    ERROR_RETURN(file, "File %s is too large", filename);
  }

  file.size = file_stat.st_size + 1;
  file.data = malloc(file.size);
  file.data[file.size - 1] = 0;
  fread(file.data, file.size - 1, 1, fp);

  fclose(fp);
  DEBUG_TRACE("Loaded %s, size: %d", filename, file.size);
  return file;
}

void
file_free(File file)
{
  free(file.data);
}

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
