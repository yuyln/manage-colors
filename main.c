#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>

//https://raw.githubusercontent.com/tsoding/nob.h/refs/heads/main/nob.h
#define nob_shift(xs, xs_sz) ((xs_sz)--, *(xs)++)

#define yu_assert(cond, msg) do {if (!(cond)) { fprintf(stderr, "%s:%d yu_assert on condition \"%s\" failed.", __FILE__, __LINE__, #cond); if (msg != NULL) fprintf(stderr, "%s", msg); fprintf(stderr, "\n"); *(volatile int*)(0) = 0; } } while(0)

#define yu_da_append(da, item) do { \
    if ((da)->len >= (da)->cap) { \
        if ((da)->cap <= 1) \
            (da)->cap = (da)->len + 2;\
        else\
            (da)->cap *= 2; \
        (da)->items = realloc((da)->items, sizeof(*(da)->items) * (da)->cap); \
        yu_assert((da)->items != NULL, "yu_da_append failed, which happens on allocation failure. Buy more RAM lol");\
    } \
    (da)->items[(da)->len] = (item); \
    (da)->len += 1; \
} while(0)

#define yu_da_remove(da, idx) do { \
    if ((idx) >= (da)->len || (idx) < 0) { \
        yu_error("%s:%d Trying to remove out of range idx %" PRIi64 " from dynamic array", __FILE__, __LINE__, (s64)idx);\
        break; \
    } \
    memmove(&(da)->items[(idx)], &(da)->items[(idx) + 1], sizeof(*(da)->items) * ((da)->len - (idx) - 1)); \
    if ((da)->len <= (da)->cap / 2) { \
        (da)->cap /= 1.5; \
        (da)->items = realloc((da)->items, sizeof(*(da)->items) * (da)->cap); \
        yu_assert((da)->items != NULL, "yu_da_remove failed, which happens on yu_reallocation to a *smaller* failure. What the actual fuck");\
        memset(&(da)->items[(da)->len], 0, sizeof(*(da)->items) * ((da)->cap - (da)->len));\
    } \
    (da)->len -= 1;\
} while(0)

typedef struct {
    const char *str;
    size_t len;
} yu_sv;

typedef struct {
    char *items;
    size_t len;
    size_t cap;
} StringBuilder;

void sb_cat_fmt(StringBuilder *s, const char *fmt, ...) {
    char *tmp = NULL;

    va_list arg_list;
    va_start(arg_list, fmt);
    if (!fmt) {
        printf("fmt is null");
        goto err;
    }

    size_t s2_len = vsnprintf(NULL, 0, fmt, arg_list) + 1;
    va_end(arg_list);

    tmp = malloc(s2_len);
    yu_assert(tmp != NULL, "allocation failed");

    va_start(arg_list, fmt);
    vsnprintf(tmp, s2_len, fmt, arg_list);
    va_end(arg_list);
    for (size_t i = 0; i < s2_len - 1; ++i)
    yu_da_append(s, tmp[i]);
err:
    free(tmp);
}

char* read_entire_file(const char *path, size_t *len_out) {
    char *ret = NULL;
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Failed at opening \"%s\": %s", path, strerror(errno));
        return NULL;
    }

    int c = 0;
    struct {char *items; size_t len; size_t cap;} ret_str = {0};

    while ((c = fgetc(f)) != EOF)
        yu_da_append(&ret_str, c);

    if (len_out)
        *len_out = ret_str.len;

    yu_da_append(&ret_str, '\0');
    ret = ret_str.items;

    fclose(f);
    return ret;
}

typedef union {
    uint32_t rgba;
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
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

char sep[] = "\n :,\0\r\t";
size_t sep_len = sizeof(sep) / sizeof(*sep);
int TOKEN_SKIP = 3;

bool char_in_string(char c, char *str, size_t str_len) {
    str_len = str_len == 0? strlen(str): str_len;
    for (size_t i = 0; i < str_len; ++i) {
        if (str[i] == c) return true;
    }
    return false;
}

ssize_t find_next(char c, char *str, size_t str_len) {
    str_len = str_len == 0? strlen(str): str_len;
    size_t i = 0;
    for (i = 0; i < str_len && str[i] != c; ++i) {}
    return str[i] == c? (ssize_t)i: (ssize_t)-1;
}

int main(int argc, const char **argv) {
    int ret = 0;
    const char *program = nob_shift(argv, argc);
    if (argc != 3) {
        printf("Usage: %s <color table> <to modify file> <output>", program);
        return 1;
    }
    const char *color_table_dir = nob_shift(argv, argc);
    const char *to_change_dir = nob_shift(argv, argc);
    const char *output_dir = nob_shift(argv, argc);

    size_t file_size = 0;
    char *file = read_entire_file(color_table_dir, &file_size);

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

    char *file_in = read_entire_file(to_change_dir, &file_size);
    FILE *fout = fopen(output_dir, "w");
    StringBuilder file_out = {0};

    for (size_t i = 0; i < file_size;) {
        while (char_in_string(file_in[i], sep, sep_len)) {
            yu_da_append(&file_out, file_in[i]);
            ++i;
        }

        for (size_t j = 0; j < options.len; ++j) {
            Option it = options.items[j];

            if (it.str_color.len + i > file_size){
                continue;
            }

            if (strncmp(it.str_color.str, file_in + i, it.str_color.len) != 0) {
                continue;
            }

            if (strncmp(it.out_format.str, "hex", it.out_format.len) == 0) {
                sb_cat_fmt(&file_out, "#%08X", it.color.rgba);
            } else if (strncmp(it.out_format.str, "rgba", it.out_format.len) == 0) {
                sb_cat_fmt(&file_out, "rgba(%d,%d,%d,%d)", it.color.r, it.color.g, it.color.b, it.color.a);
            } else if (strncmp(it.out_format.str, "frgba", it.out_format.len) == 0) {
                sb_cat_fmt(&file_out, "rgba(%f,%f,%f,%f)", it.color.r / 255.0, it.color.g / 255.0, it.color.b / 255.0, it.color.a / 255.0);
            } else {
                printf("Not reco");
                ret = 1;
                goto end;
            }
            i += it.str_color.len; //skips the portion that corresponds to the color string in the color table
        }

        while (!char_in_string(file_in[i], sep, sep_len)) {
            yu_da_append(&file_out, file_in[i]);
            ++i;
        }
    }

    fwrite(file_out.items, file_out.len, 1, fout);
end:
    free(file_out.items);
    free(options.items);
    free(file_in);
    free(sep_idxs.items);
    free(file);
    fclose(fout);
    return ret;
}
