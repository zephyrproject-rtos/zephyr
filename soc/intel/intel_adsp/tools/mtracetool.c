/* Copyright (c) 2026 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * mtracetool: standalone, dependency-free capture tool for the Intel ADSP
 * "mtrace" debug-window log slot (see subsys/logging/backends/
 * log_backend_adsp_mtrace.c and soc/intel/intel_adsp/common/include/
 * adsp_debug_window.h).
 *
 * This is roughly equivalent to running cavstool.py in log-only mode
 * (-l): it does not load firmware, does not drive HDA/IPC, and does not
 * unbind the kernel's audio driver, so it can run alongside a normally
 * operating driver/firmware stack. Unlike cavstool.py it has no other
 * mode, no Python dependency, and builds as a static binary, which makes
 * it easy to deploy on products/test rigs without a Python runtime, to
 * catch the log content around a crash (e.g. while loading a library) as
 * early as possible.
 *
 * Build (fully static, no libc dependencies beyond what's linked in):
 *   gcc -O2 -Wall -static -o mtracetool mtracetool.c
 * or, for a smaller/more portable static binary:
 *   musl-gcc -O2 -Wall -static -o mtracetool mtracetool.c
 *
 * Usage: run as root while the DSP is expected to log (or crash), e.g.:
 *   ./mtracetool -o /tmp/mtrace.bin
 * It polls for the mtrace slot to appear, dumps its full current content
 * immediately when found, then keeps following newly appended bytes until
 * interrupted with Ctrl-C.
 *
 * Note on catching early-boot/hang issues: with the default
 * CONFIG_LOG_MODE_DEFERRED, log messages are only written into the mtrace
 * SRAM slot once a background logging thread later drains them. If the DSP
 * deadlocks or hangs before that thread runs again, the most recent (and
 * usually most interesting) log lines can be stuck in that internal queue
 * and never reach the window at all, no matter how fast or how early this
 * tool polls. For that kind of debugging, build the firmware with:
 *   CONFIG_LOG_MODE_DEFERRED=n
 *   CONFIG_LOG_MODE_IMMEDIATE=y
 * so log calls are written straight into the mtrace window synchronously.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* --- SRAM window layout, mirrors cavstool.py / adsp_debug_window.h --- */

#define WINDOW_BASE            0x80000u
#define WINDOW_STRIDE          0x20000u
#define WINDOW_BASE_ACE        0x180000u
#define WINDOW_STRIDE_ACE      0x8000u

#define ADSP_DW_SLOT_SIZE      4096u
#define ADSP_DW_SLOT_COUNT     15
#define ADSP_DW_SLOT_DEBUG_LOG 0x474f4c00u /* low byte carries the core id */
#define ADSP_DW_SLOT_TYPE_MASK 0xffffff00u

#define MTRACE_LOG_BUF_SIZE    (ADSP_DW_SLOT_SIZE - 2 * (uint32_t)sizeof(uint32_t))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* ACE platform PCI device ids, cribbed from cavstool.py / the SOF kernel driver */
static const unsigned int ACE15[] = { 0x7728, 0x7f50, 0x7e28 };
static const unsigned int ACE20[] = { 0xa828 };
static const unsigned int ACE30[] = { 0xe428, 0x4d28 };
static const unsigned int ACE40[] = { 0x6e50 };

static volatile sig_atomic_t g_stop;

static void on_signal(int sig)
{
	(void)sig;
	g_stop = 1;
}

static void die(const char *msg)
{
	fprintf(stderr, "mtracetool: %s\n", msg);
	exit(1);
}

static bool in_list(unsigned int id, const unsigned int *list, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (list[i] == id) {
			return true;
		}
	}
	return false;
}

/* Case-insensitive substring search, PCI_CLASS values have no "0x" prefix */
static bool uevent_class_matches(const char *path, const char *n1, const char *n2)
{
	FILE *f = fopen(path, "r");
	char line[256];
	bool found = false;

	if (!f) {
		return false;
	}

	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, "PCI_CLASS=", 10) != 0) {
			continue;
		}
		for (char *p = line; *p; p++) {
			*p = (char)tolower((unsigned char)*p);
		}
		if ((n1 && strstr(line, n1)) || (n2 && strstr(line, n2))) {
			found = true;
		}
		break;
	}
	fclose(f);
	return found;
}

/* Scan /sys/bus/pci/devices for an ADSP device, mirrors map_regs() in cavstool.py */
static char *find_pci_device(void)
{
	static const char *base = "/sys/bus/pci/devices";
	DIR *d = opendir(base);
	struct dirent *ent;
	char *primary = NULL, *fallback = NULL;

	if (!d) {
		return NULL;
	}

	while ((ent = readdir(d))) {
		char devdir[512], uevent[560];

		if (ent->d_name[0] == '.') {
			continue;
		}
		snprintf(devdir, sizeof(devdir), "%s/%s", base, ent->d_name);
		snprintf(uevent, sizeof(uevent), "%s/uevent", devdir);

		if (uevent_class_matches(uevent, "40100", "40380")) {
			free(primary);
			primary = strdup(devdir);
		} else if (!fallback && uevent_class_matches(uevent, "40300", NULL)) {
			fallback = strdup(devdir);
		}
	}
	closedir(d);

	if (primary) {
		free(fallback);
		return primary;
	}
	return fallback;
}

static int read_device_id(const char *pcidir, unsigned int *id_out)
{
	char path[512], buf[64];
	FILE *f;

	snprintf(path, sizeof(path), "%s/device", pcidir);
	f = fopen(path, "r");
	if (!f) {
		return -1;
	}
	if (!fgets(buf, sizeof(buf), f)) {
		fclose(f);
		return -1;
	}
	fclose(f);
	*id_out = (unsigned int)strtoul(buf, NULL, 16);
	return 0;
}

/* mmap BAR4 (Intel Audio DSP registers/SRAM windows) read-only */
static volatile uint8_t *map_bar4(const char *pcidir, size_t *len_out)
{
	char path[512];
	int fd;
	struct stat st;
	void *map;

	snprintf(path, sizeof(path), "%s/resource4", pcidir);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "mtracetool: could not open %s (need root?): %s\n",
			path, strerror(errno));
		exit(1);
	}
	if (fstat(fd, &st) != 0 || st.st_size == 0) {
		die("could not determine BAR4 size");
	}
	map = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);
	if (map == MAP_FAILED) {
		fprintf(stderr, "mtracetool: mmap of %s failed: %s\n", path, strerror(errno));
		exit(1);
	}
	*len_out = (size_t)st.st_size;
	return (volatile uint8_t *)map;
}

static inline uint32_t rd32(volatile uint8_t *bar4, uint32_t off)
{
	return *(volatile uint32_t *)(bar4 + off);
}

/* Scan the debug window descriptor table for an mtrace (DEBUG_LOG) slot.
 * Returns the byte offset (within BAR4) of the matching slot, or -1.
 * *desc_off_out, if given, receives the offset of the matching descriptor
 * itself, so the caller can later re-check that it still describes the
 * same slot (a DSP reset can tear the window down and reassign slots).
 */
static long find_mtrace_slot(volatile uint8_t *bar4, uint32_t debug_off,
			      int want_core, uint32_t *desc_off_out, int *core_out)
{
	for (int i = 0; i < ADSP_DW_SLOT_COUNT; i++) {
		uint32_t desc_off = debug_off + (uint32_t)i * 12;
		uint32_t type = rd32(bar4, desc_off + 4);

		if ((type & ADSP_DW_SLOT_TYPE_MASK) != ADSP_DW_SLOT_DEBUG_LOG) {
			continue;
		}
		int core = (int)(type & 0xff);

		if (want_core >= 0 && core != want_core) {
			continue;
		}
		if (core_out) {
			*core_out = core;
		}
		if (desc_off_out) {
			*desc_off_out = desc_off;
		}
		return (long)(debug_off + ADSP_DW_SLOT_SIZE * (uint32_t)(1 + i));
	}
	return -1;
}

/* True while the descriptor found earlier by find_mtrace_slot() still
 * describes the same live mtrace slot for the same core. A DSP reset (or
 * the PCI device losing power/memory decode entirely, which reads back
 * as an all-ones descriptor) will make this false.
 */
static bool mtrace_slot_still_live(volatile uint8_t *bar4, uint32_t desc_off, int core)
{
	uint32_t type = rd32(bar4, desc_off + 4);

	return (type & ADSP_DW_SLOT_TYPE_MASK) == ADSP_DW_SLOT_DEBUG_LOG
		&& (int)(type & 0xff) == core;
}

/* Poll until the mtrace slot appears (for the requested core, or any core
 * if want_core < 0), or until interrupted. Used both for the initial wait
 * and to re-arm after a DSP reset tears the slot down.
 */
static long wait_for_slot(volatile uint8_t *bar4, uint32_t debug_off, int want_core,
			   long interval_us, uint32_t *desc_off_out, int *core_out)
{
	while (!g_stop) {
		long slot_off = find_mtrace_slot(bar4, debug_off, want_core,
						  desc_off_out, core_out);

		if (slot_off >= 0) {
			return slot_off;
		}
		if (interval_us > 0) {
			usleep((useconds_t)interval_us);
		}
	}
	return -1;
}

static void write_all(int fd, const void *buf, size_t len)
{
	const uint8_t *p = buf;

	while (len) {
		ssize_t n = write(fd, p, len);

		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			die("write failed");
		}
		p += (size_t)n;
		len -= (size_t)n;
	}
}

static void usage(const char *argv0)
{
	fprintf(stderr,
		"Usage: %s [options]\n"
		"  -o, --output FILE   Write captured data to FILE (default: stdout)\n"
		"  -c, --core N        Only match the mtrace slot for core N (default: any)\n"
		"  -i, --interval US   Poll interval in microseconds (default: 200)\n"
		"                      0 = busy-poll with no sleep at all (max CPU, max speed)\n"
		"  -h, --help          This message\n"
		"\n"
		"To capture a DSP deadlock/hang and not just a crash, build the firmware\n"
		"with CONFIG_LOG_MODE_DEFERRED=n and CONFIG_LOG_MODE_IMMEDIATE=y: with the\n"
		"default deferred logging, messages only reach this window once a\n"
		"background thread later flushes them, so a hang can strand the most\n"
		"recent lines in an internal queue where no amount of polling can see them.\n",
		argv0);
}

int main(int argc, char **argv)
{
	const char *out_path = NULL;
	int want_core = -1;
	long interval_us = 200;

	static const struct option longopts[] = {
		{ "output",   required_argument, NULL, 'o' },
		{ "core",     required_argument, NULL, 'c' },
		{ "interval", required_argument, NULL, 'i' },
		{ "help",     no_argument,       NULL, 'h' },
		{ 0, 0, 0, 0 },
	};
	int opt;

	while ((opt = getopt_long(argc, argv, "o:c:i:h", longopts, NULL)) != -1) {
		switch (opt) {
		case 'o':
			out_path = optarg;
			break;
		case 'c':
			want_core = atoi(optarg);
			break;
		case 'i':
			interval_us = atol(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	char *pcidir = find_pci_device();

	if (!pcidir) {
		die("no ADSP PCI device found in /sys/bus/pci/devices");
	}
	fprintf(stderr, "mtracetool: using PCI device %s\n", pcidir);

	unsigned int devid = 0;

	if (read_device_id(pcidir, &devid) != 0) {
		die("could not read PCI device id");
	}
	bool is_ace = in_list(devid, ACE15, ARRAY_SIZE(ACE15))
		|| in_list(devid, ACE20, ARRAY_SIZE(ACE20))
		|| in_list(devid, ACE30, ARRAY_SIZE(ACE30))
		|| in_list(devid, ACE40, ARRAY_SIZE(ACE40));

	uint32_t win_base = is_ace ? WINDOW_BASE_ACE : WINDOW_BASE;
	uint32_t win_stride = is_ace ? WINDOW_STRIDE_ACE : WINDOW_STRIDE;
	uint32_t debug_off = win_base + win_stride * 2;

	fprintf(stderr, "mtracetool: device id 0x%04x (%s), debug window at 0x%x\n",
		devid, is_ace ? "ACE" : "cAVS", debug_off);

	size_t bar4_len;
	volatile uint8_t *bar4 = map_bar4(pcidir, &bar4_len);

	free(pcidir);

	int out_fd = STDOUT_FILENO;

	if (out_path) {
		out_fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (out_fd < 0) {
			fprintf(stderr, "mtracetool: could not open %s: %s\n",
				out_path, strerror(errno));
			return 1;
		}
	}

	uint32_t buflen = MTRACE_LOG_BUF_SIZE;
	bool first_time = true;

	while (!g_stop) {
		fprintf(stderr, "mtracetool: waiting for mtrace slot to appear...\n");

		uint32_t desc_off = 0;
		int core = -1;
		long slot_off = wait_for_slot(bar4, debug_off, want_core,
					       interval_us, &desc_off, &core);

		if (g_stop) {
			break;
		}

		fprintf(stderr, "mtracetool: %s mtrace slot for core %d at BAR4 offset 0x%lx\n",
			first_time ? "found" : "re-acquired", core, slot_off);
		first_time = false;

		uint32_t data_off = (uint32_t)slot_off + 2 * (uint32_t)sizeof(uint32_t);

		/* Snapshot the whole ring buffer right away, oldest byte first, to
		 * grab as much history as possible around the event that made the
		 * slot show up in the first place.
		 */
		uint32_t w0 = rd32(bar4, (uint32_t)slot_off + 4);
		uint8_t *snapshot = malloc(buflen);

		if (!snapshot) {
			die("out of memory");
		}
		for (uint32_t i = 0; i < buflen; i++) {
			snapshot[i] = bar4[data_off + (w0 + i) % buflen];
		}
		write_all(out_fd, snapshot, buflen);
		free(snapshot);
		if (out_fd != STDOUT_FILENO) {
			fsync(out_fd);
		}

		fprintf(stderr, "mtracetool: initial %u-byte snapshot captured, following new "
			"output (Ctrl-C to stop)\n", buflen);

		uint32_t last_w = w0;

		while (!g_stop) {
			if (!mtrace_slot_still_live(bar4, desc_off, core)) {
				fprintf(stderr, "mtracetool: mtrace slot lost (DSP reset or "
					"device removed?), waiting for it to reappear\n");
				break;
			}

			uint32_t w = rd32(bar4, (uint32_t)slot_off + 4);

			if (w != last_w) {
				uint32_t avail = (w - last_w + buflen) % buflen;
				uint8_t chunk[MTRACE_LOG_BUF_SIZE];

				for (uint32_t i = 0; i < avail; i++) {
					chunk[i] = bar4[data_off + (last_w + i) % buflen];
				}
				write_all(out_fd, chunk, avail);
				last_w = w;
			} else if (interval_us > 0) {
				usleep((useconds_t)interval_us);
			}
		}
	}

	if (out_fd != STDOUT_FILENO) {
		close(out_fd);
	}
	return g_stop ? 130 : 0;
}
