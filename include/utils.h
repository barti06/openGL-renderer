#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <log.h>

typedef struct Svec2f
{
    float x;
    float y;
} Svec2f;

// reads a file and stores it in a string
static inline char* read_file(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) { log_error("Cannot open file: %s", path); return NULL; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* buf = malloc(size + 1);
    if (!buf) { fclose(f); return NULL; }

    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

#endif