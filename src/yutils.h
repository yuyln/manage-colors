#ifndef __YUTILS_H_
#define __YUTILS_H_
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#ifndef yu_malloc
#include <stdlib.h>
#define yu_malloc malloc
#define yu_realloc realloc
#define yu_free free
#endif

#ifndef yu_fopen
#define yu_fopen fopen
#define yu_fclose fclose
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef u8 byte;
typedef float f32;
typedef double f64;

typedef u64 yu_xor64_state;

typedef struct {
    const char *str;
    u64 len;
} yu_sv;

typedef struct {
    char *items;
    u64 len;
    u64 cap;
} yu_sb;

#define yu_return_defer(value) do { ret = value; goto defer; } while(0)
#define YU_UNUSED(x) ((void)x)
#define YU_SIGN(x) ((x) > 0 ? 1: -1)
#define YU_INCEPTION(M)
#define YU_CLOSE_ENOUGH(x, y, eps) ((YU_SIGN((x) - (y)) * ((x) - (y))) <= (eps))
#define YU_EPS 1e-12
#define YU_SV_ARG(sv) (s32)(sv).len, (sv).str
#define YU_SV_SB(sb) ((yu_sv){.str = (const char *)(sb).items, .len = (sb).len})
#define YU_SV_CSTR(cstr) ((yu_sv){.str = (cstr), .len = strlen((cstr))})

#ifndef YU_MAX_STRS_TMP
#define YU_MAX_STRS_TMP 256
#endif
#ifndef YU_MAX_STR_TMP_LEN
#define YU_MAX_STR_TMP_LEN 1024
#endif

#define yu_assert(cond, msg) do {if (!(cond)) { fprintf(stderr, "%s:%d yu_assert on condition \"%s\" failed.", __FILE__, __LINE__, #cond); if (msg != NULL) fprintf(stderr, "%s", msg); fprintf(stderr, "\n"); *(volatile int*)(0) = 0; } } while(0)

#define yu_da_append(da, item) do { \
    if ((da)->len >= (da)->cap) { \
        if ((da)->cap <= 1) \
            (da)->cap = (da)->len + 2;\
        else\
            (da)->cap *= 2; \
        (da)->items = yu_realloc((da)->items, sizeof(*(da)->items) * (da)->cap); \
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
        (da)->items = yu_realloc((da)->items, sizeof(*(da)->items) * (da)->cap); \
        yu_assert((da)->items != NULL, "yu_da_remove failed, which happens on yu_reallocation to a *smaller* failure. What the actual fuck");\
        memset(&(da)->items[(da)->len], 0, sizeof(*(da)->items) * ((da)->cap - (da)->len));\
    } \
    (da)->len -= 1;\
} while(0)

#define yu_rb_init(rb, cap) do {\
    (rb)->items = yu_realloc((rb)->items, sizeof(*(rb)->items) * (cap));\
    yu_assert((rb)->items != NULL, "yu_rb_init failed, which happens on allocation failure. Buy more RAM lol");\
    (rb)->cap = (cap);\
    (rb)->len = 0;\
    (rb)->start = 0;\
} while(0);

#define yu_rb_at(rb, idx) ((rb)->items[((idx) + (rb)->start) % (rb)->cap])

#define yu_rb_append(rb, item) do {\
    yu_rb_at((rb), (rb)->len) = (item); \
    (rb)->len += 1; \
} while (0)

#define yu_rb_pop(rb) ((rb)->items[(((rb)->len--) + (rb)->start) % (rb)->cap)

#define yu_rb_popf(rb) ((rb)->items[(((rb)->len--) + ((rb)->start++)) % (rb)->cap)

#define yu_rb_resize(rb, size) do {\
    (rb)->items = yu_realloc((rb)->items, sizeof(*(rb)->items * (size))); \
    yu_assert((rb)->items != NULL, "yu_rb_resize failed, which happens on yu_reallocation failure. Buy more RAM lol");\
    for (u64 i = 0; i < (rb)->len; ++i) { (rb)->items[(i + rb->start) % size] = (rb)->items[(i + (rb)->start) % (rb)->cap]; }\
    (rb)->cap = size;\
} while(0)

//I HATE this. But it is still better than using void *
//I am almost sure that there is some logic error in here, but I
//am lazy to check. Maybe when this error bite me I will try to figure it
//out.
#define yu_ht_init(ht, size, hash_func, key_compare_func) do {\
    (ht)->cap = (size);\
    (ht)->len = 0;\
    (ht)->hash = hash_func;\
    (ht)->key_compare = key_compare_func;\
    (ht)->entries = yu_realloc((ht)->entries, (size) * (sizeof(*(ht)->entries)));\
    yu_assert((ht)->entries != NULL, "yu_ht_init failed, which happens on allocation failure. Buy more RAM lol");\
    for (u64 i = 0; i < size; ++i) {\
        memset(&(ht)->entries[i], 0, sizeof(*(ht)->entries));\
        (ht)->entries[i].rhash = -1;\
    }\
} while (0)

#define yu_ht_get(ht, k, item_) do {\
    s64 hash = (ht)->hash((k));\
    u64 idx = hash % (ht)->cap;\
    if ((ht)->entries[idx].rhash == -1) {\
        item_ = NULL; \
        break; \
    } \
    if ((ht)->key_compare((ht)->entries[idx].key, (k))) {\
        item_ = &(ht)->entries[idx].item; \
        break; \
    } \
    bool not_found = true;\
    for (u64 i = 0; i < (ht)->cap || (not_found = !(ht)->key_compare((ht)->entries[idx].key, k)); ++i) \
        idx = (idx + 1) % (ht)->cap;\
    if (not_found) {\
        item_ = NULL; \
        break; \
    }\
    item_ = &(ht)->entries[idx].item;\
} while (0)

#define yu_ht_get_entry(ht, k, entry) do {\
    s64 hash = (ht)->hash((k));\
    u64 idx = hash % (ht)->cap;\
    if ((ht)->key_compare((ht)->entries[idx].key, (k))) {\
        entry = &(ht)->entries[idx]; \
        break; \
    } \
    for (u64 i = 0; i < (ht)->cap; ++i) {\
        if ((ht)->key_compare((ht)->entries[idx].key, k))\
            break;\
        idx = (idx + 1) % (ht)->cap;\
    }\
    entry = &(ht)->entries[idx];\
} while (0)

#define yu_ht_insert(ht, k, item_) do {\
    s64 hash = (ht)->hash((k));\
    u64 idx = hash % (ht)->cap;\
    if ((ht)->entries[idx].rhash == -1) {\
        (ht)->entries[idx].item = item_;\
        (ht)->entries[idx].key = k;\
        (ht)->entries[idx].rhash = hash;\
        (ht)->len += 1;\
        break; \
    } \
    if ((ht)->key_compare((ht)->entries[idx].key, (k))) {\
        (ht)->entries[idx].item = item_; \
        break; \
    } \
    idx = (idx + 1) % (ht)->cap;\
    bool not_found = true;\
    for (u64 i = 0; i < (ht)->cap; ++i) {\
        if ((ht)->entries[idx].rhash == -1) {\
            (ht)->entries[idx].item = item_;\
            (ht)->entries[idx].key = k;\
            (ht)->entries[idx].rhash = hash;\
            (ht)->len += 1;\
            break; \
        } \
        if ((ht)->key_compare((ht)->entries[idx].key, (k))) {\
            (ht)->entries[idx].item = item_; \
            break; \
        } \
        idx = (idx + 1) % (ht)->cap;\
    }\
} while (0)

void yu_logf(FILE *f, const char *str, ...);
void yu_log(const char *str,  ...);

void yu_warnf(FILE *f, const char *str, ...);
void yu_warn(const char *str, ...);

void yu_errorf(FILE *f, const char *str, ...);
void yu_error(const char *str, ...);

char* yu_read_entire_file(const char *path, u64 *len_out);
char* yu_read_entire_filef(FILE *f, u64 *len_out);

f64 yu_get_seconds(void);
const char *yu_get_date(void);

void yu_sb_cat_cstr(yu_sb *sb, const char *str);
void yu_sb_cat_fmt(yu_sb *sb, const char *fmt, ...);
void yu_sb_cat_sb(yu_sb *sb, const yu_sb sb2);
void yu_sb_cat_sv(yu_sb *sb, const yu_sv sv);
const char* yu_sb_as_cstr(yu_sb *sb);
yu_sv yu_sv_chop(yu_sv *sv, const char delim);
yu_sv yu_sv_chops(yu_sv *sv, const char *delims);
const char *yu_str_tmp(const char *fmt, ...);
u64 yu_xor64_u64(yu_xor64_state *state);
f64 yu_xor64_f64(yu_xor64_state *state, f64 min, f64 max);
f64 yu_xor64_normal_distribution(yu_xor64_state *state);

#endif //__YUTILS_H_

#ifdef __YUTILS_C_
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

void yu_logf(FILE *f, const char *str, ...) {
    if (!f) {
        yu_error("File `f` is NULL, can't log to a NULL file");
        return;
    }

    if (!str)
        return;

    va_list ap;
    va_start(ap, str);
    fprintf(f, "[ %s ][ INFO ] ", yu_get_date());
    vfprintf(f, str, ap);
    fprintf(f, "\n");
    va_end(ap);
}

void yu_log(const char *str, ...) {
    if (!str)
        return;

    va_list ap;
    va_start(ap, str);
    fprintf(stdout, "[ %s ][ INFO ] ", yu_get_date());
    vfprintf(stdout, str, ap);
    fprintf(stdout, "\n");
    va_end(ap);
}

void yu_warnf(FILE *f, const char *str, ...) {
    if (!f) {
        yu_error("File `f` is NULL, can't log to a NULL file");
        return;
    }

    if (!str)
        return;

    va_list ap;
    va_start(ap, str);
    fprintf(f, "[ %s ][ WARNING ] ", yu_get_date());
    vfprintf(f, str, ap);
    fprintf(f, "\n");
    va_end(ap);
}

void yu_warn(const char *str, ...) {
    if (!str)
        return;

    va_list ap;
    va_start(ap, str);
    fprintf(stderr, "[ %s ][ WARNING ] ", yu_get_date());
    vfprintf(stderr, str, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void yu_errorf(FILE *f, const char *str, ...) {
    if (!f) {
        yu_error("File `f` is NULL, can't log to a NULL file");
        return;
    }

    if (!str)
        return;

    va_list ap;
    va_start(ap, str);
    fprintf(f, "[ %s ][ ERROR ] ", yu_get_date());
    vfprintf(f, str, ap);
    fprintf(f, "\n");
    va_end(ap);
}

void yu_error(const char *str, ...) {
    if (!str)
        return;

    va_list ap;
    va_start(ap, str);
    fprintf(stderr, "[ %s ][ ERROR ] ", yu_get_date());
    vfprintf(stderr, str, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

char* yu_read_entire_file(const char *path, u64 *len_out) {
    char *ret = NULL;
    FILE *f = yu_fopen(path, "rb");
    if (!f) {
        yu_error("Failed at opening \"%s\": %s", path, strerror(errno));
        return NULL;
    }

    s32 c = 0;
    yu_sb ret_str = {0};

    while ((c = fgetc(f)) != EOF)
        yu_da_append(&ret_str, c);

    if (len_out)
	*len_out = ret_str.len;

    yu_da_append(&ret_str, '\0');
    ret = ret_str.items;

    yu_fclose(f);
    return ret;
}

char* yu_read_entire_filef(FILE *f, u64 *len_out) {
    char *ret = NULL;
    if (!f) {
         yu_error("File provided is NULL");
         return NULL;
    }

    s32 location = ftell(f);
    if (location < 0) {
        yu_error("Could not get location of provided file (%p)", f);
        return NULL;
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        yu_error("Could not set position of file to SEEK_SET");
        return NULL;
    }

    s32 c = 0;
    yu_sb ret_str = {0};

    while ((c = fgetc(f)) != EOF)
        yu_da_append(&ret_str, c);

    if (len_out)
	*len_out = ret_str.len;

    yu_da_append(&ret_str, '\0');
    ret = ret_str.items;

    if (fseek(f, location, SEEK_SET) < 0)
        yu_warn("Could not jump back to starting location of file (%p)", f);

    return ret;
}

#ifdef _WIN32
#include <windows.h>
LARGE_INTEGER getFILETIMEoffset() {
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return (t);
}

//https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
int clock_gettime(int X, struct timespec *tv) {
    YU_UNUSED(X);
    LARGE_INTEGER t;
    FILETIME f;
    double microseconds;
    static LARGE_INTEGER offset;
    static double frequencyToMicroseconds;
    static int initialized = 0;
    static BOOL usePerformanceCounter = 0;

    if (!initialized) {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter) {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        } else {
            offset = getFILETIMEoffset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter)
        QueryPerformanceCounter(&t);
    else {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = microseconds;
    tv->tv_sec = t.QuadPart / 1000000;
    tv->tv_nsec = (t.QuadPart % 1000000) * 1000;
    return 0;
}
#endif //_WIN32

f64 yu_get_seconds(void) {
    struct timespec tmp = {0};
    clock_gettime(CLOCK_REALTIME, &tmp);
    return tmp.tv_sec + tmp.tv_nsec * 1.0e-9;
}

const char *yu_get_date(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    f64 total_seconds = yu_get_seconds();
    static char s[64];
    u64 ret = strftime(s, 127, "%d/%m/%Y %H:%M:%S", tm);
    yu_assert(ret && ret < 127, "Failed getting date format");

    ret = snprintf(s + ret, 127 - ret, ".%04.0f", (total_seconds - (u64)(total_seconds)) * 1e4);
    yu_assert(ret, "Failed formating string");

    return s;
}

void yu_sb_cat_cstr(yu_sb *sb, const char *str) {
    if (!str)
        return;
    u64 str_len = strlen(str);
    for (u64 i = 0; i < str_len; ++i)
        yu_da_append(sb, str[i]);
}

void yu_sb_cat_fmt(yu_sb *sb, const char *fmt, ...) {
    if (!fmt)
        return;
    va_list pa;
    va_start(pa, fmt);
    u64 str_len = vsnprintf(NULL, 0, fmt, pa) + 1;
    va_end(pa);

    char *tmp = yu_malloc(str_len);
    yu_assert(tmp != NULL, "Failed to allocate more memory. Buy more RAM lol");

    va_start(pa, fmt);
    str_len = vsnprintf(tmp, str_len, fmt, pa);
    va_end(pa);

    for (u64 i = 0; i < str_len; ++i)
        yu_da_append(sb, tmp[i]);

    yu_free(tmp);
}

void yu_sb_cat_sb(yu_sb *sb, const yu_sb sb2) {
    for (u64 i = 0; i < sb2.len; ++i)
        yu_da_append(sb, sb2.items[i]);
}

const char* yu_sb_as_cstr(yu_sb *sb) {
    if (sb->items == NULL || sb->len == 0 || sb->items[sb->len - 1] != '\0')
        yu_da_append(sb, '\0');
    return sb->items;
}

void yu_sb_cat_sv(yu_sb *sb, const yu_sv sv) {
    for (u64 i = 0; i < sv.len; ++i)
        yu_da_append(sb, sv.str[i]);
}

//Inspired on Tsoding's `nob.c`. Check `https://github.com/tsoding/musializer/blob/master/nob.c`
yu_sv yu_sv_chop(yu_sv *sv, const char delim) {
    yu_sv ret = {.str = sv->str, .len = 0};
    for (u64 i = 0; i < sv->len && *sv->str != delim; ++i, ++ret.len, ++sv->str) {}

    if (ret.len < sv->len) {
	    sv->str += 1;
	    sv->len -= ret.len + 1;
    } else {
	    sv->len = 0;
    }
    return ret;
}

yu_sv yu_sv_chops(yu_sv *sv, const char *delims) {
    u64 delims_len = strlen(delims);
    yu_sv ret = {.str = sv->str, .len = 0};
    for (u64 i = 0; i < sv->len; ++i, ++ret.len, ++sv->str) {
	    bool found = false;
	    for (u64 j = 0; j < delims_len; ++j) {
	        if (*sv->str == delims[j]) {
		        found = true;
		        break;
	        }
	    }
	    if (found)
	    break;
    }

    if (ret.len < sv->len) {
	    sv->str += 1;
	    sv->len -= ret.len + 1;
    } else {
	    sv->len = 0;
    }

    return ret;
}

const char* yu_str_tmp(const char *fmt, ...) {
    static char strs[YU_MAX_STRS_TMP][YU_MAX_STR_TMP_LEN];
    static u64 counter = 0;
    const char *ret = NULL;
    if (!fmt) {
	    strs[counter][0] = 0;
	    yu_return_defer(strs[counter]);
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(strs[counter], YU_MAX_STR_TMP_LEN, fmt, ap);
    va_end(ap);
    ret = strs[counter];
defer:
    counter = (counter + 1) % YU_MAX_STRS_TMP;
    return ret;
}

u64 yu_xor64_u64(yu_xor64_state *state) {
	u64 x = *state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	return *state = x;
}

f64 yu_xor64_f64(yu_xor64_state *state, f64 min, f64 max) {
    u64 rng = yu_xor64_u64(state);
    return min + rng / (f64)UINT64_MAX * (max - min);
}

//f64 yu_xor64_normal_distribution(yu_xor64_state *state) {
//    for (;;) {
//        f64 U = yu_xor64_f64(state, 0, 1);
//        f64 V = yu_xor64_f64(state, 0, 1);
//        f64 X = sqrt(8.0 / M_E) * (V - 0.5) / U;
//        f64 X2 = X * X;
//        if (X2 <= (5.0 - 4.0 * exp(0.25) * U))
//            return X;
//        else if (X2 >= (4.0 * exp(-1.35) / U + 1.4))
//            continue;
//        else if (X2 <= (-4.0 * log(U)))
//            return X;
//    }
//}
#endif //__YUTILS_C_
