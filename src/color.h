#ifndef __COLOR_H_
#define __COLOR_H_
#include "yutils.h"
#include <math.h>

#define max(a, b) ((a) > (b)? (a): (b))
#define min(a, b) ((a) < (b)? (a): (b))

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
    double h, s, v;
} ColorHSV;

int get_color_comp(Color color, char comp);
Color hsv_to_rgb(ColorHSV c);
ColorHSV rgb_to_hsv(Color c);
#endif

#ifdef __COLOR_C_

int get_color_comp(Color color, char comp) {
    switch (comp) {
    case 'r': return color.r;
    case 'g': return color.g;
    case 'b': return color.b;
    case 'a': return color.a;
    default: return -1;
    }
}

Color hsv_to_rgb(ColorHSV hsv) {
    double C = hsv.v * hsv.s;
    double X = C * (1.0 - fabs(fmod(hsv.h * 6, 2.0) - 1));
    double m = hsv.v - C;
    double r = C * (0         <= hsv.h && hsv.h < 1.0 / 6.0) +
               X * (1.0 / 6.0 <= hsv.h && hsv.h < 1.0 / 3.0) +
               0 * (1.0 / 3.0 <= hsv.h && hsv.h < 0.5) +
               0 * (0.5       <= hsv.h && hsv.h < 2.0 / 3.0) +
               X * (2.0 / 3.0 <= hsv.h && hsv.h < 5.0 / 6.0) +
               C * (5.0 / 6.0 <= hsv.h && hsv.h < 1);

    double g = X * (        0 <= hsv.h && hsv.h < 1.0 / 6.0) +
               C * (1.0 / 6.0 <= hsv.h && hsv.h < 1.0 / 3.0) +
               C * (1.0 / 3.0 <= hsv.h && hsv.h < 0.5) +
               X * (0.5       <= hsv.h && hsv.h < 2.0 / 3.0) +
               0 * (2.0 / 3.0 <= hsv.h && hsv.h < 5.0 / 6.0) +
               0 * (5.0 / 6.0 <= hsv.h && hsv.h < 1);

    yu_warn("%f %f | %f %f", C, X, r, g);

    double b = 0 * (0         <= hsv.h && hsv.h < 1.0 / 6.0) +
               0 * (1.0 / 6.0 <= hsv.h && hsv.h < 1.0 / 3.0) +
               X * (1.0 / 3.0 <= hsv.h && hsv.h < 0.5) +
               C * (0.5       <= hsv.h && hsv.h < 2.0 / 3.0) +
               C * (2.0 / 3.0 <= hsv.h && hsv.h < 5.0 / 6.0) +
               X * (5.0 / 6.0 <= hsv.h && hsv.h < 1);

    return (Color){.r = (r + m) * 0xff, .g = (g + m) * 0xff, .b = (b + m) * 0xff, .a = 0xff};
}

ColorHSV rgb_to_hsv(Color rgb) {
    ColorHSV ret = {0};
    double r = rgb.r / 255.0;
    double g = rgb.g / 255.0;
    double b = rgb.b / 255.0;
    double Cmax = max(r, max(g, b));
    double Cmin = min(r, min(g, b));
    double d = Cmax - Cmin;
    ret.v = Cmax;
    ret.s = YU_CLOSE_ENOUGH(Cmax, 0, 1e-6)? 0: d / Cmax;
    ret.h = YU_CLOSE_ENOUGH(d, 0, 1e-6)? 0:
    YU_CLOSE_ENOUGH(Cmax, r, 1e-6)? 1.0 / 6.0 * ((int)((g - b) / d) % 6):
    YU_CLOSE_ENOUGH(Cmax, g, 1e-6)? 1.0 / 6.0 * ((b - r) / d + 2):
    YU_CLOSE_ENOUGH(Cmax, b, 1e-6)? 1.0 / 6.0 * ((r - g) / d + 4): 0;
    return ret;
}

#endif
