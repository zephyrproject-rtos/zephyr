/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * This file should be included into every unit test before the unit under test!
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/

#ifndef CMOCK_UNIT_H_
#define CMOCK_UNIT_H_

/** Remove the device tree driver init macro */
#undef DT_INST_FOREACH_STATUS_OKAY
#define DT_INST_FOREACH_STATUS_OKAY(macro)

#endif
