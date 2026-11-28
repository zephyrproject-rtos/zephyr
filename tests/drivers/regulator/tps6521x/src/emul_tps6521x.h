/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EMUL_TPS6521X_H_
#define EMUL_TPS6521X_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

#define EMUL_TPS6521X_NUM_REGS 0x40U

struct emul_tps6521x_stats {
	/* successful register writes, REG_LOCK included */
	uint32_t writes;
	uint32_t reg3_writes;
	/* TPS65214: protected writes dropped while locked */
	uint32_t blocked_writes;
};

uint8_t emul_tps6521x_get(const struct emul *target, uint8_t reg);
void emul_tps6521x_set(const struct emul *target, uint8_t reg, uint8_t val);
bool emul_tps6521x_locked(const struct emul *target);
void emul_tps6521x_stats(const struct emul *target, struct emul_tps6521x_stats *stats);
/* make writes to @p reg fail with -EIO; a negative value turns this off */
void emul_tps6521x_fail_writes_to(const struct emul *target, int reg);

#endif /* EMUL_TPS6521X_H_ */
