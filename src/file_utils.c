#include "file_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t *load(const char *filepath, size_t *size) {
  FILE *file = NULL;
  int ret = fopen_s(&file, filepath, "rb");
  if (ret != 0 || !file) {
    fprintf(stderr, "Failed to open file: %s, ret: %d\n", filepath, ret);
    perror("");
    exit(1);
  }
  fseek(file, 0, SEEK_END);
  *size = ftell(file);
  rewind(file);
  uint32_t *content = malloc(*size);
  if (content == NULL) {
    printf("malloc failed\n");
  }
  fread(content, 1, *size, file);
  fclose(file);
  return content;
}
