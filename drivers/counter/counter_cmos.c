/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 *
 * This barebones driver enables the use of the PC AT-style RTC
 * (the so-called "CMOS" clock) as a primitive, 1Hz monotonic counter.
 *
 * Reading a reliable value from the RTC is a fairly slow process, because
 * we use legacy I/O ports and do a lot of iterations with spinlocks to read
 * the RTC state. Plus we have to read the state multiple times because we're
 * crossing clock domains (no pun intended). Use accordingly.
 */

#include <drivers/counter.h>
#include <device.h>
#include <soc.h>

/* The "CMOS" device is accessed via an address latch and data port. */

#define X86_CMOS_ADDR 0x70
#define X86_CMOS_DATA 0x71

/*
 * A snapshot of the RTC state, or at least the state we're
 * interested in. This struct should not be modified without
 * serious consideraton, for two reasons:
 *
 *	1. Order of the element is important, and must correlate
 *	   with addrs[] and NR_BCD_VALS (see below), and
 *	2. if it doesn't remain exactly 8 bytes long, the
 *	   type-punning to compare states will break.
 */

struct state {
	u8_t second,
	     minute,
	     hour,
	     day,
	     month,
	     year,
	     status_a,
	     status_b;
};

/*
 * If the clock is in BCD mode, the first NR_BCD_VALS
 * valies in 'struct state' are BCD-encoded.
 */

#define NR_BCD_VALS 6

/*
 * Indices into the CMOS address space that correspond to
 * the members of 'struct state'.
 */

const u8_t addrs[] = { 0, 2, 4, 7, 8, 9, 10, 11 };

/*
 * Interesting bits in 'struct state'.
 */

#define STATUS_B_24HR	0x02	/* 24-hour (vs 12-hour) mode */
#define STATUS_B_BIN	0x01	/* binary (vs BCD) mode */
#define HOUR_PM		0x80	/* high bit of hour set = PM */

/*
 * Read a value from the CMOS. Because of the address latch,
 * we have to spinlock to make the access atomic.
 */

static u8_t read_register(u8_t addr)
{
	static struct k_spinlock lock;
	k_spinlock_key_t k;
	u8_t val;

	k = k_spin_lock(&lock);
	sys_out8(addr, X86_CMOS_ADDR);
	val = sys_in8(X86_CMOS_DATA);
	k_spin_unlock(&lock, k);

	return val;
}

/* Populate 'state' with current RTC state. */

void read_state(struct state *state)
{
	int i;
	u8_t *p;

	p = (u8_t *) state;
	for (i = 0; i < sizeof(*state); ++i) {
		*p++ = read_register(addrs[i]);
	}
}

/* Convert 8-bit (2-digit) BCD to binary equivalent. */

static inline u8_t decode_bcd(u8_t val)
{
	return (((val >> 4) & 0x0F) * 10) + (val & 0x0F);
}

/*
 * Hinnant's algorithm to calculate the number of days offset from the epoch.
 */

static u32_t hinnant(int y, int m, int d)
{
	unsigned yoe;
	unsigned doy;
	unsigned doe;
	int era;

	y -= (m <= 2);
	era = ((y >= 0) ? y : (y - 399)) / 400;
	yoe = y - era * 400;
	doy = (153 * (m + ((m > 2) ? -3 : 9)) + 2)/5 + d - 1;
	doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

	return era * 146097 + ((int) doe) - 719468;
}

/*
 * Returns the Unix epoch time (assuming UTC) read from the CMOS RTC.
 * This function is long, but linear and easy to follow.
 */

u32_t read(struct device *dev)
{
	struct state state, state2;
	u64_t *pun = (u64_t *) &state;
	u64_t *pun2 = (u64_t *) &state2;
	bool pm;
	u32_t epoch;

	ARG_UNUSED(dev);

	/*
	 * Read the state until we see the same state twice in a row.
	 */

	read_state(&state2);
	do {
		state = state2;
		read_state(&state2);
	} while (*pun != *pun2);

	/*
	 * Normalize the state; 12hr -> 24hr, BCD -> decimal.
	 * The order is a bit awkward because we need to interpret
	 * the HOUR_PM flag before we adjust for BCD.
	 */

	if (state.status_b & STATUS_B_24HR) {
		pm = false;
	} else {
		pm = ((state.hour & HOUR_PM) == HOUR_PM);
		state.hour &= ~HOUR_PM;
	}

	if (!(state.status_b & STATUS_B_BIN)) {
		u8_t *cp = (u8_t *) &state;
		int i;

		for (i = 0; i < NR_BCD_VALS; ++i) {
			*cp = decode_bcd(*cp);
			++cp;
		}
	}

	if (pm) {
		state.hour = (state.hour + 12) % 24;
	}

	/*
	 * Convert date/time to epoch time. We don't care about
	 * timezones here, because we're just creating a mapping
	 * that results in a monotonic clock; the absolute value
	 * is irrelevant.
	 */

	epoch = hinnant(state.year + 2000, state.month, state.day);
	epoch *= 86400; /* seconds per day */
	epoch += state.hour * 3600; /* seconds per hour */
	epoch += state.minute * 60; /* seconds per minute */
	epoch += state.second;

	return epoch;
}

static int init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static const struct counter_config_info info = {
	.max_top_value = UINT_MAX,
	.freq = 1
};

static const struct counter_driver_api api = {
	.read = read
};

DEVICE_AND_API_INIT(counter_cmos, "CMOS", init, NULL, &info,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api);
