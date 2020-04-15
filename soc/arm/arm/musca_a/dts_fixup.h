/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#if defined (CONFIG_ARM_NONSECURE_FIRMWARE)

/* SCC */
#define DT_ARM_SCC_BASE_ADDRESS			DT_ARM_SCC_4010C000_BASE_ADDRESS

#else

/* SCC */
#define DT_ARM_SCC_BASE_ADDRESS			DT_ARM_SCC_5010C000_BASE_ADDRESS

#endif

/* End of SoC Level DTS fixup file */
