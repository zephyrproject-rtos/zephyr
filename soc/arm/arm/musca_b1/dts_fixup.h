/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#if defined (CONFIG_ARM_NONSECURE_FIRMWARE)

/* SCC */
#define DT_ARM_SCC_BASE_ADDRESS			DT_ARM_SCC_4010B000_BASE_ADDRESS

#else

/* SCC */
#define DT_ARM_SCC_BASE_ADDRESS			DT_ARM_SCC_5010B000_BASE_ADDRESS

#endif

/* End of SoC Level DTS fixup file */
