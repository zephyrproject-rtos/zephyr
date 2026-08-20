/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SBOM_UNUSED_MODULE_H_
#define SBOM_UNUSED_MODULE_H_

/**
 * @brief A helper that ships with the module but is never compiled or called.
 *
 * src/unused.c is gated behind CONFIG_SBOM_UNUSED_MODULE, which is disabled for
 * this test, so this file must not appear anywhere in the generated SBOM.
 */
int sbom_unused_module_noop(void);

#endif /* SBOM_UNUSED_MODULE_H_ */
