#include <ctype.h>
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define mkdir _mkdir
#else
#include <sys/stat.h>
#endif

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
    yu_sv identifier;
    Color color;
    yu_sv format;
    const char *formatted_color;
} Entry;

typedef struct {
    Color *items;
    size_t len;
    size_t cap;
} Colors;

typedef struct {
    Entry *items;
    size_t len;
    size_t cap;
} Table;

int create_dir_tree(const char *path) {
    yu_sb path_to_mkdir = {0};
    char *head = strchr(path, '/');
    while (head != NULL) {
        path_to_mkdir.len = 0;
        size_t len = head - path;
        if (len == 0) {
            head = strchr(head + 1, '/');
            continue;
        }
        yu_sb_cat_fmt(&path_to_mkdir, "%.*s", (int)len, path);
        head = strchr(head + 1, '/');
        if (mkdir(yu_sb_as_cstr(&path_to_mkdir), S_IRWXU | S_IRWXG | S_IROTH) < 0) {
//            if (errno == EEXIST && head == NULL) {
//                yu_warn("Creation of directory \"%.*s\" failed: %s", (int)path_to_mkdir.len, path_to_mkdir.items, strerror(errno));
//            }
            if (errno != EEXIST) {
                yu_error("Creation of directory \"%.*s\" failed: %s", (int)path_to_mkdir.len, path_to_mkdir.items, strerror(errno));
                return errno;
            }
        }
    }
    yu_free(path_to_mkdir.items);
    return 0;
}

bool sstr_has_char(const char *str, size_t len, char c) {
    for (const char *ptr = str; ptr < (str + len); ++ptr) {
        if (*ptr == c)
            return true;
    }
    return false;
}

//TODO: refactor: noisy
Color parse_color(yu_sv color_str) {
    Color ret = {0};
    //TODO: Should #ffff be 0xFFFFFFFF?
    if (*color_str.str == '#') {
        char *endptr = NULL;
        ret.rgba = strtoll(color_str.str + 1, &endptr, 16);
        if (endptr == (color_str.str + 1) && ret.rgba == 0) {
            yu_error("Could not parse any value from \"%.*s\". Defaulting to 0x00", (int)color_str.len, color_str.str);
            goto defer;
        }
        //because of endianess and user not providing alpha channel
        //+1 because of #
        if (color_str.len == (6 + 1)) {
            //yu_log("Color being interpreted as %.*s with no transparency", (int)color_str.len, color_str.str);
            ret.rgba = (ret.rgba << 8) | 0xff;
        }
    } else if (strncmp(color_str.str, "rgb", 3) == 0) {
        yu_sv inside_paren = color_str;
        yu_sv_chop(&inside_paren, '('); //dont write to inside_paren. yu_sv_chop modifies the first argument to be the sv after the delimiter. So this points to right after (
        inside_paren = yu_sv_chop(&inside_paren, ')'); //write to inside_parem. yu_sv_chop returnrs a sv with the contents of first param up to the delimiter. So this ends right before )
        yu_sv red_sv = yu_sv_chop(&inside_paren, ',');
        yu_sv gre_sv = yu_sv_chop(&inside_paren, ',');
        yu_sv blu_sv = inside_paren;
        yu_sv alp_sv = YU_SV_CSTR("255");
        if (*(color_str.str + 3) == 'a') {
            blu_sv = yu_sv_chop(&inside_paren, ',');
            alp_sv = inside_paren;
        }
        yu_sv_trim(&red_sv);
        yu_sv_trim(&gre_sv);
        yu_sv_trim(&blu_sv);
        yu_sv_trim(&alp_sv);
        //always .a because it is the first in the union
        ret.r = parse_color(red_sv).a;
        ret.g = parse_color(gre_sv).a;
        ret.b = parse_color(blu_sv).a;
        ret.a = parse_color(alp_sv).a;
    } else if (sstr_has_char(color_str.str, color_str.len, '.')) {
        char *endptr = NULL;
        ret.a = strtod(color_str.str, &endptr) * 0xff;
        if (endptr == (color_str.str) && ret.a == 0) {
            yu_error("Could not parse any value from \"%.*s\". Defaulting to 0x00", (int)color_str.len, color_str.str);
            goto defer;
        }
    } else if (isdigit(*color_str.str)) {
        char *endptr = NULL;
        ret.a = strtoll(color_str.str, &endptr, 10);
        if (endptr == (color_str.str) && ret.r == 0) {
            yu_error("Could not parse any value from \"%.*s\". Defaulting to 0x00", (int)color_str.len, color_str.str);
            goto defer;
        }
    } else {
        //yu_error("Could not parse any value from \"%.*s\". Defaulting to 0x00", (int)color_str.len, color_str.str);
        goto defer;
    }

 defer:
    return ret;
}

int get_color_comp(Color color, char comp) {
    switch (comp) {
    case 'r': return color.r;
    case 'g': return color.g;
    case 'b': return color.b;
    case 'a': return color.a;
    default: return -1;
    }
}

const char *format_color(yu_sv format, Color color) {
    yu_sb sb = {0};
    while (format.len > 0) {
        yu_sv before = yu_sv_chop(&format, '%');
        yu_sb_cat_sv(&sb, before);

        if (format.len >= 2 && !(*(format.str + 1) == 'r' ||
                                 *(format.str + 1) == 'g' ||
                                 *(format.str + 1) == 'b' ||
                                 *(format.str + 1) == 'a')) {
            yu_error("Invalid format. Printing 0 and skipping.");
            yu_sb_cat_fmt(&sb, "0");
            format.str += 1;
            format.len -= 1;
            continue;
        }

        switch (*format.str) {
        case 'i':
            yu_sb_cat_fmt(&sb, "%d", get_color_comp(color, *(format.str + 1)));
            format.str += 2;
            format.len -= 2;
            break;
        case 'f':
            yu_sb_cat_fmt(&sb, "%.5f", get_color_comp(color, *(format.str + 1)) / (double) 0xff);
            format.str += 2;
            format.len -= 2;
            break;
        case 'x':
            yu_sb_cat_fmt(&sb, "%02X", get_color_comp(color, *(format.str + 1)));
            format.str += 2;
            format.len -= 2;
            break;
        case 'X':
            yu_log("A");
            yu_sb_cat_fmt(&sb, "%08X", 0);
            format.str += 1;
            format.len -= 1;
            break;
        }
    }
    return yu_sb_as_cstr(&sb);
}

Entry parse_entry(yu_sv *line) {
    yu_sv identifier = yu_sv_chop(line, '=');

    yu_sv_trim(&identifier);
    yu_sv_trim(line);
    if (identifier.len == 0) {
        yu_log("Couldn't find next '='. Terminating parsing loop");
    }

    yu_sv color = (yu_sv){.str = line->str, .len = 0};
    for (size_t i = 0; i < line->len && strncmp(line->str, "->", 2) != 0; ++i, ++line->str, ++color.len) {}
    if (color.len < line->len) {
        line->str += 2;
        line->len -= color.len + 2;
    } else {
        line->len = 0;
    }
    yu_sv_trim(&color);
    yu_sv_trim(line);

    Color color_rgba = parse_color(color);
    if (color.len == 0) {
        yu_log("Couldn't find next '->'. Terminating parsing loop");
    }

    yu_sv format = *line;
    if (format.len == 0) {
        yu_log("Couldn't find format. Terminating parsing loop");
    }

    const char *formatted_color = format_color(format, color_rgba);
    return (Entry){.identifier = identifier, .format = format, .color = color_rgba, .formatted_color = formatted_color};
}

int main(int argc, const char **argv) {
    int ret = 0;
    Table table = {0};
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
    char *ptr = file;

    while (ptr < file + file_size) {
        yu_sv ptr_sv = YU_SV_CSTR(ptr);
        yu_sv line = yu_sv_chop(&ptr_sv, '\n');
        yu_sv_trim(&line);
        ptr += line.len + 1;
        if (line.len == 0)
            continue;

        Entry entry = parse_entry(&line);
        yu_da_append(&table, entry);
    }

    char *file_in = yu_read_entire_file(to_change_dir, &file_size);
    {
        char *last_part = strrchr(output_dir, '/') + 1;
        size_t len = last_part - output_dir;
        yu_sb base_path = {0};
        yu_sb_cat_fmt(&base_path, "%.*s", (int)len, output_dir);
        create_dir_tree(yu_sb_as_cstr(&base_path));
        yu_free(base_path.items);
    }
    FILE *fout = yu_fopen(output_dir, "wb");
    yu_assert(fout != NULL, "Could not open output_dir");
    yu_sb file_out = {0};

    for (size_t i = 0; i < file_size; ++i) {
        for (size_t j = 0; j < table.len; ++j) {
            Entry it = table.items[j];

            if (it.identifier.len + i > file_size){
                continue;
            }

            if (strncmp(it.identifier.str, file_in + i, it.identifier.len) != 0) {
                continue;
            }

            yu_sb_cat_cstr(&file_out, it.formatted_color);

            i += it.identifier.len; //skips the portion that corresponds to the color string in the color table
        }
        yu_da_append(&file_out, file_in[i]);
    }

    fwrite(file_out.items, file_out.len, 1, fout);
    free(file_out.items);
    fclose(fout);
    free(file_in);

    for (size_t i = 0; i < table.len; ++i) {
        free((void *)table.items[i].formatted_color);
    }

    free(table.items);
    free(file);
    return ret;
}
