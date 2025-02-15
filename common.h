#pragma once

#include <stdlib.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

#define rounddown(x, y)                             \
    ({                                              \
        typeof(x) _x = (x);                         \
        _x - (_x % (y));                            \
    })

#define DEFINE_PTR_CLEANUP_FUNC(type, func)         \
    __attribute__((__unused__))                     \
    static inline void _cleanup_##func(type *p) {   \
        if (*p) {                                   \
            func(*p);                               \
            *p = NULL;                              \
        }                                           \
    }

#define _cleanup(func) __attribute__((__cleanup__(_cleanup_##func)))

static inline void _cleanup_free(void *p) {
    void **_p = (void **)p;
    free(*_p);
    *_p = NULL;
}

#define _autofree __attribute__((__cleanup__(_cleanup_free)))

#define _steal_ptr(var)                             \
    ({                                              \
        typeof(var) *_pvar = &(var);                \
        typeof(var) _var = *_pvar;                  \
        *_pvar = NULL;                              \
        _var;                                       \
    })

#define _maybe_unused __attribute__((__unused__))
