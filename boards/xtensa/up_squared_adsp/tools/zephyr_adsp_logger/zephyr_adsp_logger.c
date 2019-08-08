#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define MAGIC		0x55AA

struct config_t {
	char *infile;

	unsigned long int trace_size;
	unsigned long int buf_size;

	unsigned long int interval_usecs;
} cfg;

struct log_t {
	uint16_t magic;
	uint16_t id;
	char log[0];
};

char *sof_etrace_path = "/sys/kernel/debug/sof/etrace";
char *qemu_etrace_path = "/dev/shm/qemu-bridge-etrace-mem";

unsigned long int default_trace_size = 0x2000;
unsigned long int default_buf_size = 256;

void usage(char *prog_name)
{
	fprintf(stdout, "Usage: %s [options]\n\n", prog_name);
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "  -s        : Use %s as input\n", sof_etrace_path);
	fprintf(stdout, "  -q        : Use %s as input\n", qemu_etrace_path);
	fprintf(stdout, "  -i infile : Use infile as input\n");
	fprintf(stdout, "\n");
	fprintf(stdout,
		"  -t size   : Size of the trace buffer (in bytes, default %lu)\n",
		default_trace_size);
	fprintf(stdout,
		"  -b size   : Size of buffer of one log line (in bytes, default %lu)\n",
		default_buf_size);
	fprintf(stdout, "\n");
	fprintf(stdout,
		"  -n usecs  : Read logs with update interval (microseconds, default is to read once)\n");
}

int read_logs()
{
	FILE *fd;
	int ret;
	unsigned long int offset;
	struct log_t *log;
	uint8_t *buf = NULL;
	char *log_buf = NULL;
	unsigned int max_log_strlen;

	max_log_strlen = cfg.buf_size - sizeof(log->magic) - sizeof(log->id);

	/* Open file */
	fd = fopen(cfg.infile, "rb");
	if (!fd) {
		fprintf(stderr,
			"[ERROR] Cannot open %s for reading!\n", cfg.infile);
		ret = -1;
		goto read_out;
	}

	/* Allocate buffers */
	buf = malloc(cfg.trace_size);
	if (!buf) {
		fprintf(stderr,
			"[ERROR] Cannot allocate trace buffer!\n");
		ret = -2;
		goto read_out;
	}

	log_buf = malloc(cfg.buf_size + 1);
	if (!log_buf) {
		fprintf(stderr,
			"[ERROR] Cannot allocate log buffer!\n");
		ret = -2;
		goto read_out;
	}

read_repeat:
	fprintf(stdout, "\n");

	/* Read everything in file */
	rewind(fd);
	ret = fread(buf, 1, cfg.trace_size, fd);
	if (ret == 0) {
		fprintf(stderr,
			"[ERROR] Nothing to read?\n");
		ret = -3;
		goto read_out;
	}

	/* Go through each log line and display it */
	offset = 0;
	while ((offset + cfg.buf_size) <= cfg.trace_size) {
		log = (struct log_t *)(buf + offset);

		if (log->magic == MAGIC) {
			/* Avoid non-null terminated strings */
			ret = snprintf(log_buf, max_log_strlen,
				       "%s", &log->log[0]);
			log_buf[max_log_strlen] = 0;

			fprintf(stdout,	"[ID:%5u] %s", log->id, log_buf);
		}

		/* Move to next log line */
		offset += cfg.buf_size;
	};

	/* If interval is specified, wait and loop */
	if (cfg.interval_usecs) {
		usleep(cfg.interval_usecs);
		goto read_repeat;
	}

	ret = 0;

read_out:
	if (buf) {
		free(buf);
	}
	if (log_buf) {
		free(log_buf);
	}
	if (fd) {
		fclose(fd);
	}
	return ret;
}

int main(int argc, char *argv[])
{
	int opt, ret = 0;

	/* Defaults */
	cfg.infile = sof_etrace_path;
	cfg.trace_size = default_trace_size;
	cfg.buf_size = default_buf_size;
	cfg.interval_usecs = 0;

	/* Parse arguments */
	while ((opt = getopt(argc, argv, "hsqi:t:b:n:")) != -1) {
		switch (opt) {
		case 's':
			cfg.infile = sof_etrace_path;
			break;
		case 'q':
			cfg.infile = qemu_etrace_path;
			break;
		case 'i':
			cfg.infile = optarg;
			break;
		case 't':
			cfg.trace_size = strtoul(optarg, NULL, 0);
			if (cfg.trace_size == ULONG_MAX) {
				fprintf(stderr, "[ERROR] Error parsing -t\n");
				ret = errno;
				goto main_out;
			}
			break;
		case 'b':
			cfg.buf_size = strtoul(optarg, NULL, 0);
			if (cfg.buf_size == ULONG_MAX) {
				fprintf(stderr, "[ERROR] Error parsing -b\n");
				ret = errno;
				goto main_out;
			}
			break;
		case 'n':
			cfg.interval_usecs = strtoul(optarg, NULL, 0);
			if (cfg.interval_usecs == ULONG_MAX) {
				fprintf(stderr, "[ERROR] Error parsing -n\n");
				ret = errno;
				goto main_out;
			}
			break;
		case 'h':
		default:
			usage(argv[0]);
			goto main_out;
		}
	}

	fprintf(stdout, "[INFO ] Using %s as input file\n", cfg.infile);
	fprintf(stdout, "[INFO ] Trace buffer size: %lu\n", cfg.trace_size);
	fprintf(stdout, "[INFO ] Log line buffer size: %lu\n", cfg.buf_size);
	if (cfg.interval_usecs == 0) {
		fprintf(stdout, "[INFO ] Read once\n");
	} else {
		fprintf(stdout, "[INFO ] Update Interval: %lu\n",
			cfg.interval_usecs);
	}

	ret = read_logs();

main_out:
	return ret;
}
