/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_flash.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef LOG_BACKEND_FLASH_BUILD_ID_HAS_HEADER
#include CONFIG_LOG_BACKEND_FLASH_BUILD_ID_HEADER
#endif

LOG_MODULE_REGISTER(log_backend_flash, CONFIG_LOG_DEFAULT_LEVEL);

/** Magic identifying a formatted partition: "ZLOG", little-endian. */
#define PART_MAGIC 0x474f4c5aU

/**
 * Version of the on-disk layout. Bump it whenever the layout changes in
 * an incompatible way. A partition carrying a version this firmware does
 * not know is left alone until it is erased.
 */
#define PART_VERSION 1U

/**
 * Header written at the start of the partition when the log is erased.
 *
 * The build id is stored in clear, NUL-padded and truncated to the field, so
 * that a reader can tell which firmware produced the log without already
 * holding its `.elf`.
 */
struct part_hdr {
	uint32_t magic;
	uint16_t version;
	uint16_t reserved;
	char build_id[LOG_BACKEND_FLASH_BUILD_ID_SIZE];
} __packed;

/**
 * Framing in front of every record: @c len is the length of the dictionary
 * record that follows and @c crc a CRC-16/CCITT over those bytes.
 *
 * The length duplicates what the dictionary header already carries. It is
 * stored anyway so that walking the log never has to parse a record.
 */
struct rec_frame {
	uint16_t len;
	uint16_t crc;
} __packed;

/** Shortest thing that can be a record: a dictionary header and nothing else. */
#define REC_LEN_MIN sizeof(struct log_dict_output_normal_msg_hdr_t)

/** Buffer for the chunked reads the CRC of a stored record is computed over. */
#define CRC_CHUNK 32U

/** Partition holding the log. Fixed at build time, so not part of the state. */
static const struct flash_area *const log_flash_fa =
	PARTITION_BY_NODE(DT_CHOSEN(zephyr_log_partition));

/**
 * Everything the backend remembers about the partition, all of it derived from
 * what is on the flash. Initialized by init(), not with a static initializer.
 * This makes sure the reboot-emulation in the tests works.
 *
 * This also includes some cached parameters of the flash area.
 */
static struct {
	/** Write block size, 0 until the partition has been validated. */
	size_t align;
	/** Value unwritten memory reads as. */
	uint8_t erase_val;
	/** True if the flash type is unsupported and no operation can be performed on it. */
	bool unsupported;
	/** True after the partition has been validated and it carries an unsupported version. */
	bool unknown_version;
	/** True after the partition has been validated and has a different build ID. */
	bool foreign;
	/** Build id found in the partition header. */
	char build_id[LOG_BACKEND_FLASH_BUILD_ID_SIZE];
	/** Where the next record goes, relative to the start of the partition. */
	off_t wr_off;
	/** Set when @c wr_off is known. */
	bool wr_off_known;
	/** Set by panic(), after which nothing more is written. */
	bool panic;
	/**
	 * Messages not written since boot. Atomic because the logging core
	 * reports dropped messages from its own thread, with no chance to take
	 * the lock first, and process() counts a message in panic context where
	 * the lock cannot be taken at all.
	 */
	atomic_t dropped;
} log_flash;

static K_MUTEX_DEFINE(log_flash_lock);

/**
 * Report a flash the backend cannot use.
 *
 * ensure_device() runs with log_flash_lock held, so a message emitted from it
 * can leave the caller waiting for the logging thread to free a message buffer
 * while the logging thread waits for that lock. CONFIG_LOG_BLOCK_IN_THREAD is
 * what makes the caller wait, so say nothing when it is set. Its timeout is no
 * help: the lock stays held for as long as the caller waits, so the logging
 * thread is stuck either way.
 */
#define SETUP_ERR(...)                                                                             \
	do {                                                                                       \
		if (!IS_ENABLED(CONFIG_LOG_BLOCK_IN_THREAD)) {                                     \
			LOG_ERR(__VA_ARGS__);                                                      \
		}                                                                                  \
	} while (0)

/**
 * Validate the device itself and set cached values.
 *
 * This cannot be called from init() because that is called before the device
 * may be ready. Instead, it should be called before each attempt to access
 * the flash. If it is not ready, this function returns -ENODEV and the next
 * access attempt can retry.
 *
 * To avoid re-querying these all the time, the @c align field is used to
 * mark that it was successful.
 *
 * If the flash parameters are not supported for any reason, this function
 * returns -ENOTSUP.
 *
 * Logging from here is bounded: the checks run once, and the message that comes
 * back takes the early return above without reaching them again. Where even
 * that is too much, SETUP_ERR() says nothing.
 */
static int ensure_device(void)
{
	const struct flash_parameters *params;

	if (log_flash.align != 0) {
		return log_flash.unsupported ? -ENOTSUP : 0;
	}

	if (!flash_area_device_is_ready(log_flash_fa)) {
		/* Not cached: the device may still come up. */
		return -ENODEV;
	}

	log_flash.erase_val = flash_area_erased_val(log_flash_fa);
	log_flash.align = flash_area_align(log_flash_fa);
	log_flash.unsupported = true;
	params = flash_get_parameters(flash_area_get_device(log_flash_fa));

	if ((flash_params_get_erase_cap(params) & FLASH_ERASE_C_EXPLICIT) != 0) {
		SETUP_ERR("Log flash area requires explicit erase");
		return -ENOTSUP;
	}
	if (log_flash.align > CONFIG_LOG_BACKEND_FLASH_WRITE_BLOCK_MAX) {
		SETUP_ERR("Log flash area write block size %zu is larger than the supported "
			  "maximum %u",
			  log_flash.align, CONFIG_LOG_BACKEND_FLASH_WRITE_BLOCK_MAX);
		return -ENOTSUP;
	}
	/* format() writes the header in one flash write, whose length has to be
	 * a multiple of the write block size. That also keeps the block size
	 * within the header, which format() relies on when it clears the magic
	 * first.
	 */
	if ((sizeof(struct part_hdr) % log_flash.align) != 0) {
		SETUP_ERR("Log flash area write block size is not aligned with partition header");
		return -ENOTSUP;
	}
	if (log_flash_fa->fa_size <= sizeof(struct part_hdr)) {
		SETUP_ERR("Log flash area too small for partition header");
		return -ENOTSUP;
	}

	log_flash.unsupported = false;

	return 0;
}

/** Compute the CRC of a stored record. */
static int record_crc(off_t off, uint16_t len, uint16_t *crc)
{
	uint8_t buf[CRC_CHUNK];
	uint16_t acc = 0;

	while (len > 0) {
		size_t chunk = MIN((size_t)len, sizeof(buf));
		int rc = flash_area_read(log_flash_fa, off, buf, chunk);

		if (rc < 0) {
			return rc;
		}

		acc = crc16_ccitt(acc, buf, chunk);
		off += (off_t)chunk;
		len -= (uint16_t)chunk;
	}

	*crc = acc;

	return 0;
}

/**
 * Generic multiplexer function to walk the log and stop at the first empty or
 * corrupted record.
 *
 * Records are counted starting from @p first and up to @p max_cnt (if not 0),
 * and returned in @p cnt. Setting @p first and @p max_cnt to 0 counts the
 * number of records in the log.
 *
 * If @p buf is not NULL, copy the records (without framing) into it, starting
 * with the @p first one and up to @p max_cnt records, or until the buffer is
 * full (sized by @p size ). If the @p first record doesn't fit in the buffer,
 * return -ENOSPC.
 *
 * If @p max_cnt is not 0, it exits after that many records.
 *
 * If the walk runs through the end, wr_off and wr_off_known are set.
 *
 * @param first   Index of the first record to copy; earlier ones are skipped.
 * @param max_cnt Most records to copy, or 0 for no limit.
 * @param buf     Where to copy them, or NULL to count them only.
 * @param size    Bytes available in @p buf.
 * @param[out] cnt Records copied. May be NULL.
 * @param[out] len Bytes copied into @p buf. May be NULL.
 */
static int walk(uint32_t first, uint32_t max_cnt, uint8_t *buf, size_t size, uint32_t *cnt,
		size_t *len)
{
	off_t off = (off_t)sizeof(struct part_hdr);
	uint32_t seen = 0;
	uint32_t taken = 0;
	size_t copied = 0;

	while (((size_t)off + sizeof(struct rec_frame)) <= log_flash_fa->fa_size) {
		struct rec_frame frame;
		uint16_t crc;
		int rc;

		rc = flash_area_read(log_flash_fa, off, &frame, sizeof(frame));
		if (rc < 0) {
			return rc;
		}

		/* Both bytes of the length hold the erase value, so nothing was
		 * ever written here. Two equal bytes, so it does not matter
		 * which end of the length they are read as.
		 */
		if (frame.len == (uint16_t)((log_flash.erase_val << 8) | log_flash.erase_val)) {
			break;
		}

		if (frame.len < REC_LEN_MIN) {
			/* Can't be a valid record, no use to check CRC. */
			break;
		}
		if (((size_t)off + sizeof(frame) + frame.len) > log_flash_fa->fa_size) {
			/* Can't be a whole record, no use to check CRC. */
			break;
		}

		rc = record_crc(off + (off_t)sizeof(frame), frame.len, &crc);
		if (rc < 0) {
			return rc;
		}

		if (crc != frame.crc) {
			/* The log ends here, and this is where the next record
			 * goes. A record that fails its CRC is one that was
			 * being written when the power went, which can only be
			 * the last of them; carrying on would mean trusting the
			 * length in front of it, and a length that is wrong
			 * lands the walk in the middle of what follows, where
			 * anything that happens to check out is handed back as
			 * a record that was never logged.
			 */
			break;
		}

		if (seen >= first) {
			if (max_cnt != 0 && taken == max_cnt) {
				goto stopped;
			}

			if (buf != NULL) {
				if ((copied + frame.len) > size) {
					if (seen == first) {
						/* Not even the first record fits. */
						return -ENOSPC;
					}
					goto stopped;
				}

				rc = flash_area_read(log_flash_fa, off + (off_t)sizeof(frame),
						     &buf[copied], frame.len);
				if (rc < 0) {
					return rc;
				}

				copied += frame.len;
			}

			taken++;
		}

		seen++;
		off += (off_t)(sizeof(frame) + frame.len);
	}

	log_flash.wr_off = off;
	log_flash.wr_off_known = true;

stopped:
	if (cnt != NULL) {
		*cnt = taken;
	}

	if (len != NULL) {
		*len = copied;
	}

	return 0;
}

/**
 * Build id of the running firmware. Never NULL: the configured expression is
 * allowed to be, and turning that into an empty string here keeps it out of the
 * string functions below.
 */
static const char *own_build_id(void)
{
	const char *id = LOG_BACKEND_FLASH_BUILD_ID;

	return (id != NULL) ? id : "";
}

/**
 * True when the running firmware does not identify itself. Nothing is then
 * stored and nothing is compared: the log is taken as this firmware's whatever
 * wrote it.
 */
static bool no_build_id(void)
{
	return own_build_id()[0] == '\0';
}

/**
 * Clear the partition and write a fresh header.
 *
 * Everything is cleared, not only as far as the log reached. What terminates
 * the walk as the log grows back is the two zero bytes written after each
 * record, and those do not land atomically with it: a record ending on or one
 * byte short of a write block boundary puts them in the next block, a separate
 * flash write. Lose power in between and the record sits there without them,
 * and anything left further up would be read as a record that was never
 * logged.
 */
static int format(void)
{
	struct part_hdr hdr = {
		.magic = PART_MAGIC,
		.version = PART_VERSION,
		.reserved = 0,
	};
	int rc;

	strncpy(hdr.build_id, own_build_id(), sizeof(hdr.build_id));

	log_flash.wr_off_known = false;

	rc = flash_area_flatten(log_flash_fa, 0, log_flash_fa->fa_size);
	if (rc < 0) {
		return rc;
	}

	rc = flash_area_write(log_flash_fa, 0, &hdr, sizeof(hdr));
	if (rc < 0) {
		return rc;
	}

	log_flash.unknown_version = false;
	log_flash.foreign = false;
	memcpy(log_flash.build_id, hdr.build_id, sizeof(log_flash.build_id));
	log_flash.wr_off = (off_t)sizeof(struct part_hdr);
	log_flash.wr_off_known = true;

	return 0;
}

/**
 * Validate the partition and its contents, formatting it if necessary.  After
 * this, @c log_flash is fully filled in.
 *
 * Validation is done only once, the function returns immediately if it was
 * already called before (and didn't encounter an error).
 */
static int ensure_header(void)
{
	struct part_hdr hdr;
	int rc;

	rc = ensure_device();
	if (rc < 0) {
		return rc;
	}

	if (log_flash.unknown_version) {
		return -ENOTSUP;
	}

	if (log_flash.wr_off_known) {
		return 0;
	}

	rc = flash_area_read(log_flash_fa, 0, &hdr, sizeof(hdr));
	if (rc < 0) {
		return rc;
	}

	if (hdr.magic != PART_MAGIC) {
		/* Nothing this firmware can read anything out of, so it becomes
		 * this firmware's empty log.
		 */
		return format();
	}

	if (hdr.version != PART_VERSION) {
		/* A header this firmware recognizes, carrying a layout it does
		 * not. Nothing in the partition can be located, not even its
		 * end, so the only thing to do with it is to say so and leave
		 * it alone.
		 */
		log_flash.unknown_version = true;

		return -ENOTSUP;
	}

	memcpy(log_flash.build_id, hdr.build_id, sizeof(log_flash.build_id));
	log_flash.foreign = !no_build_id() && (strncmp(log_flash.build_id, own_build_id(),
						       LOG_BACKEND_FLASH_BUILD_ID_SIZE) != 0);

	return walk(0, 0, NULL, 0, NULL, NULL);
}

/**
 * Appending state.
 *
 * Records are packed end to end, so a record rarely ends on a write block
 * boundary. Before writing, the last block is re-read into a line buffer,
 * the new data is appended, and the full block is written back. This only
 * works on memory that needs no explicit erase.
 */
static struct {
	/** Write block being filled, relative to the start of the partition. */
	off_t block;
	/** Bytes of @c line the record written so far reaches. */
	size_t fill;
	/** Contents of that block. append() refills it from the start (no state is kept).
	 *  Only @c align bytes are actually used.
	 */
	uint8_t line[CONFIG_LOG_BACKEND_FLASH_WRITE_BLOCK_MAX];
} writer;

static int writer_start(void)
{
	writer.block = (off_t)ROUND_DOWN((size_t)log_flash.wr_off, log_flash.align);
	writer.fill = (size_t)(log_flash.wr_off - writer.block);

	if (writer.fill == 0) {
		memset(writer.line, log_flash.erase_val, log_flash.align);
		return 0;
	}

	return flash_area_read(log_flash_fa, writer.block, writer.line, log_flash.align);
}

static int writer_add(const void *src, size_t len)
{
	const uint8_t *p = src;

	while (len > 0) {
		size_t n = MIN(len, log_flash.align - writer.fill);

		memcpy(&writer.line[writer.fill], p, n);
		writer.fill += n;
		p += n;
		len -= n;

		if (writer.fill == log_flash.align) {
			int rc = flash_area_write(log_flash_fa, writer.block, writer.line,
						  log_flash.align);

			if (rc < 0) {
				return rc;
			}

			writer.block += (off_t)log_flash.align;
			writer.fill = 0;
			memset(writer.line, log_flash.erase_val, log_flash.align);
		}
	}

	return 0;
}

static int writer_finish(void)
{
	if (writer.fill == 0) {
		return 0;
	} else {
		return flash_area_write(log_flash_fa, writer.block, writer.line, log_flash.align);
	}
}

/** Append one record, its framing written before its body. */
static int append(const struct log_dict_output_normal_msg_hdr_t *hdr, const uint8_t *package,
		  size_t package_len, const uint8_t *data, size_t data_len)
{
	struct rec_frame frame;
	uint16_t crc;
	size_t total;
	off_t end;
	int rc;

	total = sizeof(*hdr) + package_len + data_len;
	if (total > UINT16_MAX) {
		return -EMSGSIZE;
	}

	if (((size_t)log_flash.wr_off + sizeof(frame) + total) > log_flash_fa->fa_size) {
		return -ENOSPC;
	}

	crc = crc16_ccitt(0, (const uint8_t *)hdr, sizeof(*hdr));
	if (package_len > 0) {
		crc = crc16_ccitt(crc, package, package_len);
	}
	if (data_len > 0) {
		crc = crc16_ccitt(crc, data, data_len);
	}

	frame.len = (uint16_t)total;
	frame.crc = crc;

	rc = writer_start();
	if (rc < 0) {
		goto out;
	}

	rc = writer_add(&frame, sizeof(frame));
	if (rc < 0) {
		goto out;
	}

	rc = writer_add(hdr, sizeof(*hdr));
	if (rc < 0) {
		goto out;
	}

	if (package_len > 0) {
		rc = writer_add(package, package_len);
		if (rc < 0) {
			goto out;
		}
	}

	if (data_len > 0) {
		rc = writer_add(data, data_len);
		if (rc < 0) {
			goto out;
		}
	}

	/* Where the next record goes, before the length of that next record is
	 * cleared below.
	 */
	end = writer.block + (off_t)writer.fill;

	/*
	 * Everything passed the end should have been erased. However, there
	 * are power failure scenarios where a valid record may have been
	 * left behind. As an extra safety net, reset the length field of the
	 * subsequent record.
	 */
	if (((size_t)end + sizeof(frame.len)) <= log_flash_fa->fa_size) {
		frame.len = (log_flash.erase_val << 8) | log_flash.erase_val;

		rc = writer_add(&frame.len, sizeof(frame.len));
		if (rc < 0) {
			goto out;
		}
	}

	rc = writer_finish();
	if (rc < 0) {
		goto out;
	}

	log_flash.wr_off = end;

	return 0;

out:
	/* Part of a record may be on the medium. Its CRC will not check out, so
	 * a reader steps over it, but where the log now ends has to be worked
	 * out again.
	 */
	log_flash.wr_off_known = false;

	return rc;
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	struct log_dict_output_normal_msg_hdr_t hdr;
	const void *source;
	size_t package_len;
	size_t data_len;
	uint8_t *package;
	uint8_t *data;
	int rc;

	ARG_UNUSED(backend);

	if (log_flash.panic) {
		/* After log_panic() the core processes messages inline, in
		 * whatever context produced them and with interrupts locked,
		 * which is not a context a flash write can happen in.
		 */
		atomic_inc(&log_flash.dropped);
		return;
	}

	package = log_msg_get_package(&msg->log, &package_len);
	data = log_msg_get_data(&msg->log, &data_len);
	source = log_msg_get_source(&msg->log);

	/* The dictionary format has a second record type, MSG_DROPPED_MSG, but
	 * it is not something process() is handed: it is what a backend emits
	 * of its own accord from dropped(). Every message that arrives here is
	 * a normal one.
	 *
	 * Keep in sync with log_dict_output_msg_process().
	 */
	hdr.type = MSG_NORMAL;
	hdr.domain = msg->log.hdr.desc.domain;
	hdr.level = msg->log.hdr.desc.level;
	hdr.package_len = package_len;
	hdr.data_len = data_len;
	hdr.source = (source != NULL) ? log_source_id(source) : 0;
	hdr.timestamp = msg->log.hdr.timestamp;

	k_mutex_lock(&log_flash_lock, K_FOREVER);

	rc = ensure_header();

	if (rc == 0 && log_flash.foreign) {
		/* Somebody else's log. It stays as it is until it is erased:
		 * a record of this firmware's could not be decoded against the
		 * image the rest of them belong to.
		 */
		rc = -EPERM;
	}

	if (rc == 0) {
		rc = append(&hdr, package, package_len, data, data_len);
	}

	if (rc < 0) {
		atomic_inc(&log_flash.dropped);
	}

	k_mutex_unlock(&log_flash_lock);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	atomic_add(&log_flash.dropped, (atomic_val_t)cnt);
}

static void panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);

	log_flash.panic = true;
}

/**
 * Forget everything and start from the medium again.
 *
 * The log core calls this once before the backend is activated. Calling it
 * again is how an application, or a test, gets the backend back into the state
 * it would be in after a reboot.
 */
static void init(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);

	k_mutex_lock(&log_flash_lock, K_FOREVER);
	memset(&log_flash, 0, sizeof(log_flash));
	k_mutex_unlock(&log_flash_lock);
}

static int is_ready(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);

	/* The core polls this and buffers messages until it returns 0, which is
	 * what keeps early messages out of the bin when the logging thread runs
	 * before the flash driver is initialized.
	 */
	return flash_area_device_is_ready(log_flash_fa) ? 0 : -EBUSY;
}

static const struct log_backend_api log_backend_flash_api = {
	.process = process,
	.dropped = dropped,
	.panic = panic,
	.init = init,
	.is_ready = is_ready,
};

LOG_BACKEND_DEFINE(log_backend_flash, log_backend_flash_api,
		   IS_ENABLED(CONFIG_LOG_BACKEND_FLASH_AUTOSTART));

int log_backend_flash_query(struct log_backend_flash_info *info)
{
	int rc;

	if (info == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&log_flash_lock, K_FOREVER);

	rc = ensure_header();
	if (rc < 0) {
		goto unlock;
	}

	rc = walk(0, 0, NULL, 0, &info->record_cnt, NULL);
	if (rc < 0) {
		goto unlock;
	}

	info->dropped_cnt = (uint32_t)atomic_get(&log_flash.dropped);
	memcpy(info->build_id, log_flash.build_id, LOG_BACKEND_FLASH_BUILD_ID_SIZE);
	info->build_id[LOG_BACKEND_FLASH_BUILD_ID_SIZE] = '\0';
	info->used = (size_t)log_flash.wr_off - sizeof(struct part_hdr);
	info->size = log_flash_fa->fa_size - sizeof(struct part_hdr);
	info->build_id_matches = !log_flash.foreign;

unlock:
	k_mutex_unlock(&log_flash_lock);

	return rc;
}

ssize_t log_backend_flash_read(uint32_t index, uint32_t max_cnt, void *buf, size_t size,
			       uint32_t *cnt)
{
	size_t len;
	int rc;

	if (buf == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&log_flash_lock, K_FOREVER);

	rc = ensure_header();
	if (rc == 0) {
		rc = walk(index, max_cnt, buf, size, cnt, &len);
	}

	k_mutex_unlock(&log_flash_lock);

	if (rc < 0) {
		return rc;
	}

	return (ssize_t)len;
}

int log_backend_flash_erase(void)
{
	int rc;

	k_mutex_lock(&log_flash_lock, K_FOREVER);

	rc = ensure_device();
	if (rc < 0) {
		goto unlock;
	}

	rc = format();
	if (rc < 0) {
		goto unlock;
	}

	atomic_clear(&log_flash.dropped);

unlock:
	k_mutex_unlock(&log_flash_lock);

	return rc;
}
