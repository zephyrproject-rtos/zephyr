/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SBOM_USED_MODULE_ANSWER_H_
#define SBOM_USED_MODULE_ANSWER_H_

/**
 * @brief Return the answer computed by the "used" SBOM demo module.
 *
 * The application calls this from main() so that the module's compiled code is
 * pulled into the final image and its source appears in the generated SBOM.
 *
 * @return The answer to the ultimate question of life, the universe, and everything.
 */
int sbom_used_module_answer(void);

/**
 * @brief A helper whose source file carries no license/copyright header.
 *
 * src/no_header.c has no SPDX tags of its own; its license and copyright are
 * supplied by the module's REUSE.toml. It exists to prove that 'west spdx'
 * honours REUSE.toml annotations when analyzing a source file.
 *
 * @return An arbitrary value.
 */
int sbom_used_module_from_reuse_toml(void);

#endif /* SBOM_USED_MODULE_ANSWER_H_ */
