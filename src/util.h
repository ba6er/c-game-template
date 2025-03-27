#pragma once

// Debug

#ifdef _NO_DEBUG // Debug disabled

#define DEBUG_TRACE(...)
#define DEBUG_ERROR(...)
#define DEBUG_ASSERT(x, ...)

#define ERROR_RETURN(R, ...) return R

#else // Debug enabled

#ifdef _WIN32
  #define _debug_break() __debugbreak();
#else
  #include <signal.h>
  #define _debug_break() raise(SIGTRAP);
#endif

#include <stdio.h>

#define DEBUG_TRACE(...)                    \
{                                           \
  fwrite("\x1b[32mTrace: ", 1, 12, stdout); \
  printf(__VA_ARGS__);                      \
  puts("\033[0m");                          \
}

#define DEBUG_WARNING(...)                    \
{                                             \
  fwrite("\x1b[33mWarning: ", 1, 14, stdout); \
  printf(__VA_ARGS__);                        \
  puts("\033[0m");                            \
}

#define DEBUG_ERROR(...)                                              \
{                                                                     \
  printf("\x1b[31mError in %s, line %d:\n", __FILE_NAME__, __LINE__); \
  printf(__VA_ARGS__);                                                \
  puts("\033[0m");                                                    \
}

#define DEBUG_ASSERT(x, ...)                                             \
{                                                                        \
  if (!(x))                                                              \
  {                                                                      \
    printf("\x1b[31mAssert in %s, line %d:\n", __FILE_NAME__, __LINE__); \
    printf(__VA_ARGS__);                                                 \
    puts("\033[0m");                                                     \
    _debug_break();                                                      \
  }                                                                      \
}

#define ERROR_RETURN(R, ...) \
{                            \
  DEBUG_ERROR(__VA_ARGS__);  \
  return R;                  \
}

#endif // Debug

// Files

typedef struct {
  char *data;
  unsigned int size;
}
File;

File file_read(const char *filename);
void file_free(File file);

int binary_search(const char **arr, size_t n, const char *target);
