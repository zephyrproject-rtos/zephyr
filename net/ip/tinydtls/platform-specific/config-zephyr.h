#include <misc/__assert.h>

/* Do not include assert.h as that does not exists */
#undef HAVE_ASSERT_H

#ifndef NDEBUG
#ifndef assert
#define assert(...) __ASSERT(...)
#endif
#else
#ifndef assert
#define assert(...)
#endif
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define BYTE_ORDER LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BYTE_ORDER BIG_ENDIAN
#else
#error "Unknown byte order"
#endif

#ifndef MAY_ALIAS
#define MAY_ALIAS __attribute__((__may_alias__))
#endif
