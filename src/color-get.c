#define __YUTILS_C_
#include "yutils.h"
#include "stb_image_write.h"
#include "stb_image.h"

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
    Color *items;
    size_t len;
    size_t cap;
} Colors;

int main(int argc, const char **argv) {
    const char *img_path = "/home/jose/wallpapers/1610796437867.png";
    int width, height;
    Color *img = (Color*)stbi_load(img_path, &width, &height, NULL, 4);

    stbi_image_free(img);
    return 0;
}
