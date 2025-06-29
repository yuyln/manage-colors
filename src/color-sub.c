#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>

#define __YUTILS_C_
#include "yutils.h"

//https://raw.githubusercontent.com/tsoding/nob.h/refs/heads/main/nob.h
#define nob_shift(xs, xs_sz) ((xs_sz)--, *(xs)++)

typedef union {
    u32 rgba;
    struct { //little endian
        u8 a;
        u8 b;
        u8 g;
        u8 r;
    };
} Color;

typedef struct {
    yu_sv str_color;
    Color color;
    yu_sv out_format;
} Option;

typedef struct {
    Color *items;
    size_t len;
    size_t cap;
} Colors;

typedef struct {
    Option *items;
    size_t len;
    size_t cap;
} Options;

char sep[] = "\n :,\0\r\t()";
size_t sep_len = sizeof(sep) / sizeof(*sep);
int TOKEN_SKIP = 3;

bool char_in_string(char c, char *str, size_t str_len) {
    str_len = str_len == 0? strlen(str): str_len;
    for (size_t i = 0; i < str_len; ++i) {
        if (str[i] == c) return true;
    }
    return false;
}

int main(int argc, const char **argv) {
    int ret = 0;
    const char *program = nob_shift(argv, argc);
    if (argc != 3) {
        yu_error("Usage: %s <color table> <to modify file> <output>", program);
        return 1;
    }
    const char *color_table_dir = nob_shift(argv, argc);
    const char *to_change_dir = nob_shift(argv, argc);
    const char *output_dir = nob_shift(argv, argc);

    size_t file_size = 0;
    char *file = yu_read_entire_file(color_table_dir, &file_size);

    struct {size_t *items; size_t len; size_t cap;} sep_idxs = {0};

    for (size_t i = 0; i < file_size; ++i) {
        while (char_in_string(file[i], sep, sep_len)) {
            ++i;
        }
        yu_da_append(&sep_idxs, i);
        while (!char_in_string(file[i], sep, sep_len)) {
            ++i;
        }
        yu_da_append(&sep_idxs, i);
    }

    Options options = {0};
    for (size_t i = 0; i + 1 < sep_idxs.len; i += 2 * TOKEN_SKIP) {
        yu_da_append(&options, ((Option){0}));
        Option *it = &options.items[options.len - 1];
        it->str_color = (yu_sv){.str = file + sep_idxs.items[i], .len = sep_idxs.items[i + 1] - sep_idxs.items[i]};

        yu_assert(file[sep_idxs.items[i + 2]] == '#', "Only hex input format supported atp");
        char *end_ptr = NULL;
        it->color.rgba = strtol(file + sep_idxs.items[i + 2] + 1, &end_ptr, 16);
        yu_assert(!(it->color.rgba == LONG_MIN || it->color.rgba == LONG_MAX), "Convertion to hex failed");

        it->out_format = (yu_sv){.str = file + sep_idxs.items[i + 4], .len = sep_idxs.items[i + 5] - sep_idxs.items[i + 4]};
    }

    char *file_in = yu_read_entire_file(to_change_dir, &file_size);
    FILE *fout = yu_fopen(output_dir, "w");
    yu_sb file_out = {0};

    for (size_t i = 0; i < file_size; ++i) {
        for (size_t j = 0; j < options.len; ++j) {
            Option it = options.items[j];

            if (it.str_color.len + i > file_size){
                continue;
            }

            if (strncmp(it.str_color.str, file_in + i, it.str_color.len) != 0) {
                continue;
            }

            if (strncmp(it.out_format.str, "hex", it.out_format.len) == 0) {
                yu_sb_cat_fmt(&file_out, "#%08x", it.color.rgba);
            } else if (strncmp(it.out_format.str, "rgba", it.out_format.len) == 0) {
                yu_sb_cat_fmt(&file_out, "rgba(%d,%d,%d,%d)", it.color.r, it.color.g, it.color.b, it.color.a);
            } else if (strncmp(it.out_format.str, "hexrgba", it.out_format.len) == 0) {
                yu_sb_cat_fmt(&file_out, "rgba(%08x)", it.color.rgba);
            } else if (strncmp(it.out_format.str, "frgba", it.out_format.len) == 0) {
                yu_sb_cat_fmt(&file_out, "rgba(%f,%f,%f,%f)", it.color.r / 255.0, it.color.g / 255.0, it.color.b / 255.0, it.color.a / 255.0);
            } else {
                yu_error("Format \"%.*s\" not recognized. Skipping", YU_SV_ARG(it.out_format));
                continue;
            }
            i += it.str_color.len; //skips the portion that corresponds to the color string in the color table
        }
        yu_da_append(&file_out, file_in[i]);
    }

    fwrite(file_out.items, file_out.len - 1, 1, fout);
defer:
    yu_free(file_out.items);
    yu_free(options.items);
    yu_free(file_in);
    yu_free(sep_idxs.items);
    yu_free(file);
    yu_fclose(fout);
    return ret;
}

//TODO: Proper tokenizer to avoid things like `color1color2 color3` -> `RGBARGBA RGBA`. It should be `color1color2 RGBA`.
