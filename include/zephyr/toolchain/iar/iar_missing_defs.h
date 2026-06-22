/*
 * Copyright (c) 2025 IAR Systems AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Basic macro definitions that gcc and clang provide on their own
 * but that iccarm lacks. Only those that Zephyr requires are provided here.
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_ICCARM_MISSING_DEFS_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_ICCARM_MISSING_DEFS_H_

 /* We need to define NULL with a parenthesis around _NULL
  * otherwise the DEBRACE macros won't work correctly
  */

#undef NULL
#define NULL   (_NULL)

#if defined(__IAR_SYSTEMS_ICC__)
#ifndef __CHAR_BIT__
#define __CHAR_BIT__ __CHAR_BITS__
#endif
#define __SCHAR_MAX__ __SIGNED_CHAR_MAX__

#define __INT_MAX__    __SIGNED_INT_MAX__
#define __INT_WIDTH__ (__INT_SIZE__*8)
#define __SIZEOF_INT__ __INT_SIZE__

#define __SHRT_MAX__     __SIGNED_SHORT_MAX__
#define __SHRT_WIDTH__  (__SHORT_SIZE__*8)
#define __SIZEOF_SHORT__ __SHORT_SIZE__

#define __LONG_MAX__    __SIGNED_LONG_MAX__
#define __LONG_WIDTH__ (__LONG_SIZE__*8)
#define __SIZEOF_LONG__ __LONG_SIZE__

#define __LONG_LONG_MAX__    __SIGNED_LONG_LONG_MAX__
#define __LONG_LONG_WIDTH__ (__LONG_LONG_SIZE__*8)
#define __SIZEOF_LONG_LONG__ __LONG_LONG_SIZE__

#define __INTMAX_MAX__     __INTMAX_T_MAX__
#define __SIZEOF_INTMAX__  sizeof(__INTMAX_T_TYPE__)
#define __INTMAX_WIDTH__   (__SIZEOF_INTMAX__*8)
#define __UINTMAX_MAX__    __UINTMAX_T_MAX__
#define __SIZEOF_UINTMAX__ sizeof(__UINTMAX_T_TYPE__)
#define __UINTMAX_WIDTH__  (__SIZEOF_UINTMAX__*8)

#define __INTPTR_MAX__     __INTPTR_T_MAX__
#define __INTPTR_TYPE__    __INTPTR_T_TYPE__
#define __INTPTR_WIDTH__  (__INTPTR_T_SIZE__*8)
#define __SIZEOF_POINTER__ __INTPTR_T_SIZE__

#define __PTRDIFF_MAX__      __PTRDIFF_T_MAX__
#define __PTRDIFF_WIDTH__   (__PTRDIFF_T_SIZE__*8)
#define __SIZEOF_PTRDIFF_T__ __PTRDIFF_T_SIZE__

#define __UINTPTR_MAX__  __UINTPTR_T_MAX__
#define __UINTPTR_TYPE__ __UINTPTR_T_TYPE__

/*
 * ICCARM already defines __SIZE_T_MAX__ as "unsigned int" but there is no way
 * to safeguard that here with preprocessor equality.
 */

#define __SIZE_TYPE__   __SIZE_T_TYPE__
#define __SIZE_MAX__    __SIZE_T_MAX__
#define __SIZE_WIDTH__ ((__SIZEOF_SIZE_T__)*8)
/* #define __SIZEOF_SIZE_T__ 4 */

/*
 * The following defines are inferred from the ICCARM provided defines
 * already tested above.
 */


#define __INT8_MAX__  __INT8_T_MAX__
#define __INT8_TYPE__ __INT8_T_TYPE__

#define __UINT8_MAX__  __UINT8_T_MAX__
#define __UINT8_TYPE__ __UINT8_T_TYPE__

#define __INT16_MAX__  __INT16_T_MAX__
#define __INT16_TYPE__ __INT16_T_TYPE__

#define __UINT16_MAX__  __UINT16_T_MAX__
#define __UINT16_TYPE__ __UINT16_T_TYPE__

#define __INT32_MAX__  __INT32_T_MAX__
#define __INT32_TYPE__ __INT32_T_TYPE__

#define __UINT32_MAX__  __UINT32_T_MAX__
#define __UINT32_TYPE__ __UINT32_T_TYPE__

#define __INT64_MAX__  __INT64_T_MAX__
#define __INT64_TYPE__ __INT64_T_TYPE__

#define __UINT64_MAX__  __UINT64_T_MAX__
#define __UINT64_TYPE__ __UINT64_T_TYPE__

#define __INT_FAST8_MAX__    __INT_FAST8_T_MAX__
#define __INT_FAST8_TYPE__   __INT_FAST8_T_TYPE__
#define __INT_FAST8_WIDTH__ (__INT_FAST8_T_SIZE__*8)

#define __INT_FAST16_MAX__    __INT_FAST16_T_MAX__
#define __INT_FAST16_TYPE__   __INT_FAST16_T_TYPE__
#define __INT_FAST16_WIDTH__ (__INT_FAST16_T_SIZE__*8)

#define __INT_FAST32_MAX__    __INT_FAST32_T_MAX__
#define __INT_FAST32_TYPE__   __INT_FAST32_T_TYPE__
#define __INT_FAST32_WIDTH__ (__INT_FAST32_T_SIZE__*8)

#define __INT_FAST64_MAX__    __INT_FAST64_T_MAX__
#define __INT_FAST64_TYPE__   __INT_FAST64_T_TYPE__
#define __INT_FAST64_WIDTH__ (__INT_FAST64_T_SIZE__*8)

#define __INT_LEAST8_MAX__    __INT_LEAST8_T_MAX__
#define __INT_LEAST8_TYPE__   __INT_LEAST8_T_TYPE__
#define __INT_LEAST8_WIDTH__ (__INT_LEAST8_T_SIZE__*8)

#define __INT_LEAST16_MAX__    __INT_LEAST16_T_MAX__
#define __INT_LEAST16_TYPE__   __INT_LEAST16_T_TYPE__
#define __INT_LEAST16_WIDTH__ (__INT_LEAST16_T_SIZE__*8)

#define __INT_LEAST32_MAX__    __INT_LEAST32_T_MAX__
#define __INT_LEAST32_TYPE__   __INT_LEAST32_T_TYPE__
#define __INT_LEAST32_WIDTH__ (__INT_LEAST32_T_SIZE__*8)

#define __INT_LEAST64_MAX__    __INT_LEAST64_T_MAX__
#define __INT_LEAST64_TYPE__   __INT_LEAST64_T_TYPE__
#define __INT_LEAST64_WIDTH__ (__INT_LEAST64_T_SIZE__*8)

#define __UINT_FAST8_MAX__  __UINT_FAST8_T_MAX__
#define __UINT_FAST8_TYPE__ __UINT_FAST8_T_TYPE__

#define __UINT_FAST16_MAX__  __UINT_FAST16_T_MAX__
#define __UINT_FAST16_TYPE__ __UINT_FAST16_T_TYPE__

#define __UINT_FAST32_MAX__  __UINT_FAST32_T_MAX__
#define __UINT_FAST32_TYPE__ __UINT_FAST32_T_TYPE__

#define __UINT_FAST64_MAX__  __UINT_FAST64_T_MAX__
#define __UINT_FAST64_TYPE__ __UINT_FAST64_T_TYPE__

#define __UINT_LEAST8_MAX__  __UINT_LEAST8_T_MAX__
#define __UINT_LEAST8_TYPE__ __UINT_LEAST8_T_TYPE__

#define __UINT_LEAST16_MAX__  __UINT_LEAST16_T_MAX__
#define __UINT_LEAST16_TYPE__ __UINT_LEAST16_T_TYPE__

#define __UINT_LEAST32_MAX__  __UINT_LEAST32_T_MAX__
#define __UINT_LEAST32_TYPE__ __UINT_LEAST32_T_TYPE__

#define __UINT_LEAST64_MAX__  __UINT_LEAST64_T_MAX__
#define __UINT_LEAST64_TYPE__ __UINT_LEAST64_T_TYPE__

#endif /* __IAR_SYSTEMS_ICC__ */

#endif
