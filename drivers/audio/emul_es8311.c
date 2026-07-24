/*
 * Copyright (c) 2026 Hsiu-Chi Tsai
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C emulator for the Everest ES8311 audio codec, for native_sim driver tests.
 *
 * Implements a 256-byte register file. The chip-id registers (0xFD/0xFE) return
 * the ES8311 identity bytes 0x83/0x11. All other registers default to 0x00 and
 * tests seed the registers the driver is expected to program to the OPPOSITE of
 * the expected post-configure value, so an assertion can only pass if the driver
 * actually performs the write. The emulator also records the ordered sequence of
 * register writes so tests can verify the configure() ordering.
 *
 * NOTE: this is a register byte-store, not a behavioural model of the codec. It
 * verifies that the driver emits the register writes it is expected to; it does
 * not verify that those values are the right ones for real silicon. That comes
 * from the ES8311 user guide and from hardware validation.
 */

#define DT_DRV_COMPAT everest_es8311

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

/*
 * The test-backend prototypes, so the definitions below are checked against them. Same
 * directory as this file: a "" include resolves regardless of the build's include path, so
 * the emulator compiles wherever CONFIG_EMUL_ES8311 puts it, not only inside its own test.
 */
#include "emul_es8311.h"

LOG_MODULE_REGISTER(emul_es8311, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define ES8311_EMUL_WLOG_LEN 64

#define ES8311_REG_RESET    0x00U
#define ES8311_RESET_BITS   0x1FU /* the digital/CMG/master/ADC/DAC resets */
#define ES8311_REG_INI      0xFAU
#define ES8311_INI_HOLD     0x01U /* INI_REG: a LEVEL, not a pulse */
#define ES8311_REG_CHIP_ID1 0xFDU
#define ES8311_REG_CHIP_ID2 0xFEU
#define ES8311_CHIP_ID1     0x83U
#define ES8311_CHIP_ID2     0x11U

struct es8311_emul_data {
	uint8_t regs[256];
	/* Fault injection: when > 0, the next N transfers return -EIO. */
	int fail_remaining;
	/*
	 * Fault injection by position: when >= 0, transfer number `fail_at` fails and
	 * the ones before it succeed. fail_remaining can only break the FIRST transfer
	 * of a sequence, which is the easy case; a driver that writes eight registers
	 * has eight places to be interrupted, and the interesting ones are in the
	 * middle, where the hardware is half reprogrammed.
	 */
	int fail_at;
	/*
	 * Fault injection PERSISTENT. fail_at() fires once and lets the sequence recover; this
	 * fails transfer number `fail_from` and never stops, so the driver's error-path quiesce
	 * fails on the same bus. Negative when disarmed.
	 */
	int fail_from;
	/*
	 * Fault injection AFTER EFFECT. fail_write_reg breaks a write before it lands; this lets
	 * the write to `fail_write_landed` reach the register file and THEN returns -EIO, modelling
	 * a NAK on the completion of a write the part already accepted. Negative when disarmed.
	 */
	int fail_write_landed;
	/*
	 * Fault injection by DIRECTION. set_fail() and fail_at() break a transfer whatever
	 * it is; this breaks only reads, and lets every write through.
	 *
	 * That is not a contrived split. A read on this bus is a write-then-read with a
	 * repeated start, and a controller or a device can fail that while a plain
	 * two-byte register write still works. It matters because it is exactly the
	 * failure that turns a read-modify-write mute into no mute at all: the read
	 * fails, the write is never attempted, and the DAC stays as loud as it was.
	 */
	bool fail_reads;
	/*
	 * Fault injection by TARGET. Breaking transfer number N is precise but brittle:
	 * it counts reads as well as writes, so inserting one register read anywhere
	 * upstream silently re-aims the fault at a different write. This says what it
	 * means -- break the write to THIS register -- and keeps saying it.
	 *
	 * Negative when disarmed. 0x00 is a real register, so it cannot be the sentinel.
	 */
	int fail_write_reg;
	/*
	 * Ordered log of writes. The addresses are what most tests care about, but the
	 * values matter for one thing in particular: proving the driver never asserts the
	 * reset bits in 0x00. That has to be checked against what the driver WROTE, not
	 * against a register-file reset the real part does not perform.
	 */
	uint8_t wlog[ES8311_EMUL_WLOG_LEN];
	uint8_t wval[ES8311_EMUL_WLOG_LEN];
	int wcount;
	/*
	 * Concurrency hook. When armed, the transfer blocks just before it applies
	 * a write to `pause_reg`, in the calling thread's own context, and signals
	 * `reached`. It resumes when a test gives `release`. That lets a test park
	 * one caller in the middle of a register sequence and drive a second one
	 * into it, which is the only way to observe whether the driver holds its
	 * lock across the whole sequence or merely across the read of its cache.
	 */
	struct k_sem reached;
	struct k_sem release;
	uint8_t pause_reg;
	bool pause_armed;
};

/*
 * Is the part currently holding its register file down? Derived from the register itself
 * rather than a separate flag, so that a driver clearing 0xFA releases it exactly the way
 * the silicon does, and a driver that sets it bricks itself exactly the way ours once did.
 */
static inline bool es8311_emul_held(const struct es8311_emul_data *data)
{
	return (data->regs[ES8311_REG_INI] & ES8311_INI_HOLD) != 0U;
}

static int es8311_emul_init(const struct emul *target, const struct device *parent);

/* Test backend hooks (declared extern in the test). */

/*
 * Seed the part into the state a previous firmware can leave it in: INI_REG asserted, the
 * register file held at its defaults, every read returning 0x00. The vendor Linux driver
 * does this deliberately at shutdown. A driver that probes such a chip reads 0x00 for both
 * chip-id registers, and if it treats that as "not an ES8311" and gives up, the chip is
 * unrecoverable.
 */
void emul_es8311_set_ini_hold(const struct emul *target, bool hold)
{
	struct es8311_emul_data *data = target->data;

	if (hold) {
		data->regs[ES8311_REG_INI] |= ES8311_INI_HOLD;
	} else {
		data->regs[ES8311_REG_INI] &= (uint8_t)~ES8311_INI_HOLD;
	}
}

/*
 * Break every READ and let every WRITE through, until disarmed.
 *
 * The off switch has to work on a bus that is only half working. A driver that mutes with a
 * read-modify-write cannot: it reads first, the read fails, and it returns without ever
 * attempting the write it was asked to make.
 */
void emul_es8311_fail_reads(const struct emul *target, bool fail)
{
	struct es8311_emul_data *data = target->data;

	data->fail_reads = fail;
}

/*
 * Break every write to `reg`, and only to `reg`. Pass a negative value to disarm.
 *
 * This exists so that a test about "what happens when the serial-port mute fails" can say
 * exactly that, instead of encoding it as "break the fourth transfer" and quietly meaning
 * something else the next time a register read is added upstream of it.
 */
void emul_es8311_fail_write_to(const struct emul *target, int reg)
{
	struct es8311_emul_data *data = target->data;

	data->fail_write_reg = reg;
}

/* The value written at write-log index `idx`, or -1 if there is no such write. */
int emul_es8311_wval_at(const struct emul *target, int idx)
{
	struct es8311_emul_data *data = target->data;

	if (idx < 0 || idx >= data->wcount || idx >= ES8311_EMUL_WLOG_LEN) {
		return -1;
	}

	return (int)data->wval[idx];
}

void emul_es8311_set_fail(const struct emul *target, int n)
{
	struct es8311_emul_data *data = target->data;

	data->fail_remaining = n;
}

/*
 * Fail transfer number `idx` (0-based), letting the ones before it through. Pass a
 * negative value to disarm. Self-disarming: it fires once.
 */
void emul_es8311_fail_at(const struct emul *target, int idx)
{
	struct es8311_emul_data *data = target->data;

	data->fail_at = idx;
}

/*
 * Fail transfer number `idx` and every transfer after it, persistently. Pass a negative value
 * to disarm. This is the bus that dies and stays dead, so a driver's best-effort cleanup fails
 * on the same bus -- the only way to prove a commit is fail-closed by ORDERING and not merely by
 * a cleanup that happened to still work.
 */
void emul_es8311_fail_from(const struct emul *target, int idx)
{
	struct es8311_emul_data *data = target->data;

	data->fail_from = idx;
}

/*
 * Fail every write to `reg`, but only after applying it: the byte lands in the register file and
 * the transfer still returns -EIO. Pass a negative value to disarm. Models a NAK on the
 * completion of a write that already reached the part -- the case a driver cannot distinguish
 * from a write that never left, and so must reason about rather than read back.
 */
void emul_es8311_fail_write_landed(const struct emul *target, int reg)
{
	struct es8311_emul_data *data = target->data;

	data->fail_write_landed = reg;
}

void emul_es8311_reset_log(const struct emul *target)
{
	struct es8311_emul_data *data = target->data;

	data->wcount = 0;
}

int emul_es8311_write_count(const struct emul *target)
{
	struct es8311_emul_data *data = target->data;

	return data->wcount;
}

int emul_es8311_write_at(const struct emul *target, int idx)
{
	struct es8311_emul_data *data = target->data;

	if (idx < 0 || idx >= data->wcount || idx >= ES8311_EMUL_WLOG_LEN) {
		return -1;
	}
	return data->wlog[idx];
}

/*
 * Arm the concurrency hook: the next write to `reg` blocks inside the transfer,
 * in the caller's own thread, until emul_es8311_release() is called.
 */
void emul_es8311_pause_before(const struct emul *target, uint8_t reg)
{
	struct es8311_emul_data *data = target->data;

	k_sem_init(&data->reached, 0, 1);
	k_sem_init(&data->release, 0, 1);
	data->pause_reg = reg;
	data->pause_armed = true;
}

/* Block until a caller has actually parked on the armed register. */
int emul_es8311_wait_paused(const struct emul *target, k_timeout_t timeout)
{
	struct es8311_emul_data *data = target->data;

	return k_sem_take(&data->reached, timeout);
}

/* Let the parked caller finish its write, and disarm the hook. */
void emul_es8311_release(const struct emul *target)
{
	struct es8311_emul_data *data = target->data;

	data->pause_armed = false;
	k_sem_give(&data->release);
}

/*
 * Override the chip-id registers (0xFD/0xFE) so a test can exercise the
 * driver's rejection of an unexpected identity. The driver reads
 * these in init() via i2c_reg_read_byte_dt().
 */
void emul_es8311_set_chip_id(const struct emul *target, uint8_t id1, uint8_t id2)
{
	struct es8311_emul_data *data = target->data;

	data->regs[ES8311_REG_CHIP_ID1] = id1;
	data->regs[ES8311_REG_CHIP_ID2] = id2;
}

static int es8311_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				int addr)
{
	struct es8311_emul_data *data = target->data;

	ARG_UNUSED(addr);
	__ASSERT_NO_MSG(msgs && num_msgs);

	if (data->fail_remaining > 0) {
		data->fail_remaining--;
		return -EIO;
	}

	if (data->fail_at >= 0) {
		if (data->fail_at == 0) {
			data->fail_at = -1;
			return -EIO;
		}
		data->fail_at--;
	}

	if (data->fail_from >= 0) {
		if (data->fail_from == 0) {
			return -EIO; /* persistent: does NOT disarm, unlike fail_at */
		}
		data->fail_from--;
	}

	if (num_msgs == 1) {
		/* Write transaction: buf = [reg, value]; only len 2 is valid. */
		struct i2c_msg *m = &msgs[0];

		if ((m->flags & I2C_MSG_READ) || m->len != 2) {
			return -EIO;
		}

		/*
		 * Park here, still in the caller's thread and therefore still holding
		 * whatever locks the caller holds, so a test can drive a second caller
		 * into the middle of this register sequence.
		 */
		if (data->pause_armed && m->buf[0] == data->pause_reg) {
			k_sem_give(&data->reached);
			(void)k_sem_take(&data->release, K_FOREVER);
		}

		/*
		 * There is deliberately NO register-file reset modelled here.
		 *
		 * An earlier version of this emulator wiped the whole register array when
		 * the low five bits of 0x00 were asserted. That is not what the part does.
		 * RST_DIG is documented as "reset digital except control port block", and
		 * the control port block is where the registers live, so no value of 0x00
		 * resets the register file. Modelling a reset the silicon does not perform
		 * would make every test that relies on it a test of a chip that does not
		 * exist -- and this is the first codec emulator in the tree, so whatever it
		 * models will be copied.
		 *
		 * A driver that asserts those bits is still a defect, and it is still
		 * caught: the write log records the value as well as the address, so a test
		 * can assert on what the driver WROTE.
		 */
		if (data->wcount < ES8311_EMUL_WLOG_LEN) {
			data->wlog[data->wcount] = m->buf[0];
			data->wval[data->wcount] = m->buf[1];
		}
		data->wcount++;

		/*
		 * INI_REG (0xFA bit 0) is a LEVEL, not a pulse. While it is set, the part
		 * holds the register file down: every write to any OTHER register is
		 * silently discarded and every read returns 0x00, while the I2C transfer
		 * itself still ACKs. The one register that remains writable is 0xFA itself,
		 * which is the only way out -- and a driver that never writes it can be
		 * handed a chip it cannot recover.
		 *
		 * The write is logged above either way, because the log is a record of what
		 * the driver attempted, not of what the chip accepted.
		 */
		if (data->fail_write_reg >= 0 && m->buf[0] == (uint8_t)data->fail_write_reg) {
			LOG_DBG("W reg=0x%02x FAILED (injected)", m->buf[0]);
			return -EIO;
		}

		if (es8311_emul_held(data) && m->buf[0] != ES8311_REG_INI) {
			LOG_DBG("W reg=0x%02x val=0x%02x DISCARDED (INI_REG held)", m->buf[0],
				m->buf[1]);
			return 0;
		}

		data->regs[m->buf[0]] = m->buf[1];
		LOG_DBG("W reg=0x%02x val=0x%02x", m->buf[0], m->buf[1]);

		/*
		 * Landed, then NAKed. The register above already took the value; returning the
		 * error here models a write that reached the part but whose completion the bus
		 * refused. The driver cannot tell this apart from a write that never left.
		 */
		if (data->fail_write_landed >= 0 && m->buf[0] == (uint8_t)data->fail_write_landed) {
			LOG_DBG("W reg=0x%02x LANDED then FAILED (injected)", m->buf[0]);
			return -EIO;
		}
		return 0;
	}

	if (num_msgs == 2) {
		/* Write reg address, then read N bytes. */
		struct i2c_msg *w = &msgs[0];
		struct i2c_msg *r = &msgs[1];
		uint8_t reg;

		if ((w->flags & I2C_MSG_READ) || w->len != 1 || !(r->flags & I2C_MSG_READ)) {
			return -EIO;
		}
		if (data->fail_reads) {
			return -EIO;
		}

		reg = w->buf[0];
		for (uint32_t i = 0; i < r->len; i++) {
			/* Held: every read returns 0x00, chip-id registers included. */
			r->buf[i] = es8311_emul_held(data) ? 0x00U : data->regs[(uint8_t)(reg + i)];
		}
		return 0;
	}

	return -EIO;
}

static const struct i2c_emul_api es8311_emul_api = {
	.transfer = es8311_emul_transfer,
};

/*
 * Put the emulated part back exactly as it comes up: a zeroed register file with the chip
 * identity in it, every fault hook disarmed, an empty write log.
 *
 * A test suite whose fixture does not do this is one long test with many entry points. An
 * armed fault hook, a seeded dirty register or a spoofed chip id left behind by one case
 * silently becomes the starting state of the next, and the next one can pass BECAUSE of it.
 *
 * It cannot reach the DRIVER's cached properties -- those live in struct es8311_data. The
 * fixture resets those separately, through the codec API.
 */
void emul_es8311_reset(const struct emul *target)
{
	(void)es8311_emul_init(target, NULL);
}

static int es8311_emul_init(const struct emul *target, const struct device *parent)
{
	struct es8311_emul_data *data = target->data;

	ARG_UNUSED(parent);

	memset(data->regs, 0, sizeof(data->regs));
	data->fail_remaining = 0;
	data->fail_at = -1;        /* NOT 0: 0 means "fail transfer 0" */
	data->fail_from = -1;      /* NOT 0: 0 means "fail from transfer 0" */
	data->fail_write_reg = -1; /* NOT 0: 0x00 is a real register */
	data->fail_write_landed = -1;
	data->fail_reads = false;
	data->pause_armed = false;
	data->wcount = 0;
	memset(data->wval, 0, sizeof(data->wval));

	/* Chip identity registers. */
	data->regs[ES8311_REG_CHIP_ID1] = ES8311_CHIP_ID1;
	data->regs[ES8311_REG_CHIP_ID2] = ES8311_CHIP_ID2;

	return 0;
}

#define ES8311_EMUL(n)                                                                             \
	static struct es8311_emul_data es8311_emul_data_##n;                                       \
	EMUL_DT_INST_DEFINE(n, es8311_emul_init, &es8311_emul_data_##n, NULL, &es8311_emul_api,    \
			    NULL)

DT_INST_FOREACH_STATUS_OKAY(ES8311_EMUL)
