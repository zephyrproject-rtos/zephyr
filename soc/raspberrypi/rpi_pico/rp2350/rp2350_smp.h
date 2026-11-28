/*
 * Copyright (c) 2026 Seunghoon Park
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Parking the other core of the RP2350 while the flash is reprogrammed
 *
 * Writing or erasing the flash takes the XIP interface away from both cores.
 * A core that is still executing from flash (or reading it) while that
 * happens gets wrong data. These functions stop the other core in a loop in
 * RAM, with all of its interrupts masked, for as long as the caller needs.
 */

#ifndef ZEPHYR_SOC_RASPBERRYPI_RPI_PICO_RP2350_RP2350_SMP_H_
#define ZEPHYR_SOC_RASPBERRYPI_RPI_PICO_RP2350_RP2350_SMP_H_

#include <stdint.h>
#include <zephyr/sys/clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Park the other core in RAM and lock interrupts on the calling core
 *
 * On success the other core is spinning in RAM with PRIMASK set and the
 * calling core has its interrupts locked, so the flash may be reprogrammed
 * until rp2350_smp_unpark_other() is called. If the other core cannot be
 * parked in a short time (it may be waiting for this core with its own
 * interrupts masked) the request is withdrawn, the interrupts of the calling
 * core are unlocked for a moment and the request is tried again, until
 * @p timeout expires.
 *
 * If the other core has not been started there is nothing to park and only
 * the interrupts of the calling core are locked, even if they were locked
 * already. Otherwise this needs interrupts unlocked. It cannot be called from
 * an interrupt handler.
 * (With CONFIG_MP_MAX_NUM_CPUS == 1 the functions are not built, see
 * CONFIG_RPI_PICO_SMP_FLASH_PARK.)
 *
 * @param key Receives the interrupt lock key, to pass to
 *            rp2350_smp_unpark_other().
 * @param timeout How long to keep trying, in total.
 *
 * @retval 0 The other core is parked (or not running).
 * @retval -EBUSY The other core did not park within @p timeout.
 * @retval -EWOULDBLOCK Called from an interrupt handler, or with interrupts
 *         locked while the other core is running.
 */
int rp2350_smp_park_other(unsigned int *key, k_timeout_t timeout);

/**
 * @brief Let the other core run again and unlock interrupts on this core
 *
 * @param key The key returned by rp2350_smp_park_other().
 */
void rp2350_smp_unpark_other(unsigned int key);

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK_DIAG
/** Counters of the parking protocol, for tests. Not atomic, read when idle. */
struct rp2350_smp_park_stats {
	/** Parked on the first try, without a retry */
	uint32_t park_ok;
	/** A request that was withdrawn because the other core did not answer in time */
	uint32_t park_cancel;
	/** Another request was in progress, tried again */
	uint32_t park_busy;
	/** The other core was started but its doorbell was not enabled yet, tried again */
	uint32_t park_not_ready;
	/** Gave up with -EBUSY */
	uint32_t park_ebusy;
	/** Doorbell 1 handled by a core that took the request, per core */
	uint32_t victim_parked[2];
	/** Doorbell 1 handled by a core that found no request for it, per core */
	uint32_t victim_stale[2];
	/** Longest time a core stayed parked, in microseconds of TIMER1, per core */
	uint32_t victim_max_us[2];
};

extern volatile struct rp2350_smp_park_stats rp2350_smp_park_stats;
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_RASPBERRYPI_RPI_PICO_RP2350_RP2350_SMP_H_ */
