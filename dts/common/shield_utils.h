/*
 * Copyright (c) 2024 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DT_SHIELD_UTILS_H
#define __DT_SHIELD_UTILS_H

#include <zephyr/dt-bindings/dt-util.h>

/**
 * Determines whether a specified option is defined.
 *
 * @param o option name
 */
#define SHIELD_OPTION_DEFINED(o)                                                                   \
	IS_ENABLED(UTIL_CAT(UTIL_CAT(SHIELD_OPTION_,                                               \
			     UTIL_CAT(SHIELD_DERIVED_NAME,                                         \
			      UTIL_CAT(_,                                                          \
			       UTIL_CAT(o, _DEFINED))))))

/**
 * Get an option which is specified on the command line.
 *
 * @param o option name
 */
#define SHIELD_OPTION(o)                                                                           \
	COND_CODE_1(SHIELD_OPTION_DEFINED(o),                                                      \
                    (UTIL_CAT(SHIELD_OPTION_,                                                      \
		      UTIL_CAT(SHIELD_DERIVED_NAME,                                                \
			UTIL_CAT(_, o)))),                                                         \
                    (UTIL_CAT(SHIELD_OPTION_DEFAULT_,                                              \
		      UTIL_CAT(SHIELD_BASE_NAME,                                                   \
		       UTIL_CAT(_, o)))))

/*
 * Aliases for common options...
 */

#define SHIELD_CONN_DEFINED             SHIELD_OPTION_DEFINED(CONN)
#define SHIELD_CONN                     SHIELD_OPTION(CONN)
#define SHIELD_LABEL_DEFINED            SHIELD_OPTION_DEFINED(LABEL)
#define SHIELD_LABEL                    SHIELD_OPTION(LABEL)
#define SHIELD_ADDR_DEFINED             SHIELD_OPTION_DEFINED(ADDR)
#define SHIELD_ADDR                     SHIELD_OPTION(ADDR)

/*
 * Various utility macros...
 */

#define SHIELD_CONVENTIONAL_LABEL_(stem)                                                           \
	COND_CODE_1(SHIELD_OPTION_DEFINED(ADDR),                                                   \
	            (COND_CODE_1(SHIELD_OPTION_DEFINED(CONN),                                      \
				 (UTIL_CAT(stem,                                                   \
				   UTIL_CAT(_,                                                     \
				    UTIL_CAT(SHIELD_ADDR,                                          \
				     UTIL_CAT(_, SHIELD_CONN))))),                                 \
				 (UTIL_CAT(stem,                                                   \
				   UTIL_CAT(_, SHIELD_ADDR))))),                                   \
	            (COND_CODE_1(SHIELD_OPTION_DEFINED(CONN),                                      \
				 (UTIL_CAT(stem,                                                   \
			           UTIL_CAT(_, SHIELD_CONN))),                                     \
				 (stem))))
#define SHIELD_CONVENTIONAL_LABEL(stem)                                                            \
	COND_CODE_1(SHIELD_OPTION_DEFINED(LABEL),                                                  \
		    (SHIELD_LABEL),                                                                \
		    (SHIELD_CONVENTIONAL_LABEL_(stem)))

#define SHIELD_0X_ADDR UTIL_CAT(0x, SHIELD_OPTION(ADDR))

#endif /* __DT_SHIELD_UTILS_H */
