#pragma once

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

#define DEFINE_PTR_CLEANUP_FUNC(type, func)         \
    __attribute__((__unused__))                     \
    static inline void _cleanup_##func(type *p) {   \
        if (*p) {                                   \
            func(*p);                               \
            *p = NULL;                              \
        }                                           \
    }

#define _cleanup(func) __attribute__((__cleanup__(_cleanup_##func)))

#define _steal_ptr(var)                             \
    ({                                              \
        typeof(var) *_pvar = &(var);                \
        typeof(var) _var = *_pvar;                  \
        *_pvar = NULL;                              \
        _var;                                       \
    })
