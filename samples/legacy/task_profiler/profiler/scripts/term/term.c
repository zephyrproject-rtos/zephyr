/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <pthread.h>

#define DLE 16

/* DEBUG mode allows to log all decoded profiler events in ASCII
 * Replace #undef by #define to enable this
 */
#undef DEBUG

/* TESTMODE should be set to check UART link end-to-end is working
 * In that case, target side profiler code should be build
 * in testmode as well. In that case, bytes from 0x0 to 0xff
 * are sent in sequence and this terminal will check no byte is
 * missing
 * Replace #undef by #define to enable this
 */
#undef TESTMODE

/* Redefined here since asm-generic/termbits.h is conflicting with termios.h */
struct termios2 {
	tcflag_t c_iflag;               /* input mode flags */
	tcflag_t c_oflag;               /* output mode flags */
	tcflag_t c_cflag;               /* control mode flags */
	tcflag_t c_lflag;               /* local mode flags */
	cc_t c_line;                    /* line discipline */
	cc_t c_cc[19];              /* control characters */
	speed_t c_ispeed;               /* input speed */
	speed_t c_ospeed;               /* output speed */
};

#define BOTHER 0010000

/* Global variables since used in SIGINT handler */
/* Profiler data hex output file */
FILE *prof_out_file;
/* Console ASCII output file */
FILE *console_out_file;
/* Device TTY descriptor (/dev/tty*) */
int dev_tty;
/* Device TTY settings save */
struct termios  dev_tty_save1;
struct termios2 dev_tty_save2;
/* console settings save */
struct termios host_tty_save;
/* TX handler thread */
pthread_t tx_thread;

/*
 *                   ____________        __________        __________
 *  logging   ----->|            |      |          |      |          |-----> stdout: logging (ASCII)
 *                  |            |      |          |<====>| host TTY |
 *  profiling ----->| device TTY |<====>| profterm |      |__________|<----- sdtin: shell cmd
 *                  |            |      |          |
 *  shell cmd <-----|            |      |          |-----> FILE: profiler data (HEX)
 *                  |____________|      |__________|-----> FILE: logging (ASCII)
 *
 */

void exit_term(int dummy)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &host_tty_save);
	tcsetattr(dev_tty, TCSANOW, &dev_tty_save1);
	ioctl(dev_tty, TCSETS2, dev_tty_save2);
	close(dev_tty);
	fclose(prof_out_file);
	if (console_out_file)
		fclose(console_out_file);
	printf("\n\n~~~ EXIT ~~~\n");

	exit(0);
}

unsigned char read_char(int fd)
{
	unsigned char c;
	int n;

	while (1) {
		n = read(fd, &c, 1);
		if (n > 0)
			return c;
		else if ((n < 0) && (errno != EAGAIN)) {
			printf("Error reading from device: %s\n", strerror(errno));
			exit_term(0);
		}
	}
}

void decode_profile(int fd, FILE *out)
{
	unsigned int event_id;
	unsigned int size = 0;
	unsigned int data[10];
	int i;

	while (1) {
		event_id = read_char(fd);
		size = read_char(fd);
#ifdef DEBUG
		printf("%08d Profiler event %d(%d):", count++, event_id, size);
#endif
		for (i = 0; i < size; i++) {
			data[i] = read_char(fd);
			data[i] += read_char(fd) << 8;
			data[i] += read_char(fd) << 16;
			data[i] += read_char(fd) << 24;
#ifdef DEBUG
			printf(" %08x", data[i]);
#endif
		}
#ifdef DEBUG
		printf("\n");
#endif
		fwrite(&event_id, 1, 2, out);
		fwrite(&size, 1, 2, out);
		fwrite(data, size, 4, out);
		return;
	}
}

int uart_set(int tty, uint baud_rate)
{
	struct termios ttyset1;
	struct termios2 ttyset2;
	int rc;

	rc = tcgetattr(tty, &ttyset1);
	if (rc < 0) {
		printf("Failed to get attr: %d, %s\n", rc, strerror(errno));
		return -1;
	}

	dev_tty_save1 = ttyset1;    /* preserve original settings for restoration */

	cfmakeraw(&ttyset1);

	ttyset1.c_cc[VMIN] = 1;
	ttyset1.c_cc[VTIME] = 10;

	ttyset1.c_cflag &= ~CSTOPB;
	ttyset1.c_cflag &= ~CRTSCTS;    /* no HW flow control? */
	ttyset1.c_cflag |= CLOCAL | CREAD;

	rc = tcsetattr(tty, TCSANOW, &ttyset1);

	if (rc < 0) {
		printf("Failed to set attr: %d, %s\n", rc, strerror(errno));
		return -1;
	}

	ioctl(tty, TCGETS2, &ttyset2);
	dev_tty_save2 = ttyset2;

	ttyset2.c_cflag &= ~CBAUD;
	ttyset2.c_cflag |= BOTHER;
	ttyset2.c_ispeed = baud_rate;
	ttyset2.c_ospeed = baud_rate;
	int r = ioctl(tty, TCSETS2, &ttyset2);

	if (r < 0) {
		printf("Failed to set speed: %s\n", strerror(errno));
		tcsetattr(tty, TCSANOW, &dev_tty_save1);
		return -1;
	}

	ioctl(tty, TCGETS2, &ttyset2);
	printf("UART speed set to %d\n", ttyset2.c_ispeed);

	return 0;
}

int console_set(int tty)
{
	struct termios ttyset;
	int rc;

	rc = tcgetattr(tty, &host_tty_save);
	if (rc < 0) {
		printf("Failed to get attr: %d, %s\n", rc, strerror(errno));
		return -1;
	}

	ttyset = host_tty_save;

	ttyset.c_lflag &= ~(ICANON | ECHO);

	rc = tcsetattr(tty, TCSANOW, &ttyset);

	if (rc < 0) {
		printf("Failed to set attr: %d, %s\n", rc, strerror(errno));
		return -1;
	}

	return 0;
}

void *tx_process(void *arg)
{
	char c;

	while (1) {
		c = getchar();
		if (c != EOF) {
			if (write(dev_tty, &c, 1) < 0) {
				printf("Failed writing to device: %s\n", strerror(errno));
				exit_term(0);
			}
			if (c == '\n') {
				char cr = '\r';

				if (write(dev_tty, &cr, 1) < 0) {
					printf("Failed writing to device: %s\n", strerror(errno));
					exit_term(0);
				}
			}
		}
		fflush(stdout);
	}
}

void rx_process(void)
{
	int count = 0;

#ifdef TESTMODE
	unsigned char prev_c;
	int enable_check = 0;
#endif

	while (1) {
		unsigned char c;

		c = read_char(dev_tty);

		count++;

#ifdef TESTMODE
		if (enable_check) {
			printf("%02x ", c);
			if (c != ++prev_c)
				printf("*ERROR*\n");
			prev_c = c;
		} else {
			if ((count > 10) && (c == 0)) {
				enable_check = 1;
				prev_c = 0;
			} else {
				printf("%c", c);
			}
		}
#else
		if (c == DLE) {
			decode_profile(dev_tty, prof_out_file);
		} else {
			putc(c, stdout);
			if (console_out_file) {
				fputc(c, console_out_file);
			}
		}
#endif /* ! TESTMODE */

		fflush(stdout);
	}
}

int main(int argc, char **argv)
{
	char *dev_tty_name = 0;
	int baud_rate = 921600;
	char *out_prof = 0;
	char *out_console = 0;
	int i = 0;
	int flag = 0;

	if (console_set(STDIN_FILENO) < 0) {
		goto exit;
	}

	signal(SIGINT, exit_term);

	for (i = 1; i < argc; i++) {
		if (strcmp("-s", argv[i]) == 0) {
			i++;
			if (i < argc) {
				int baud = atoi(argv[i]);

				if (baud > 0)
					baud_rate = baud;
			}
		} else {
			if (dev_tty_name == 0)
				dev_tty_name = (char *)argv[i];
			else if (out_prof == 0)
				out_prof = (char *)argv[i];
			else if (out_console == 0)
				out_console = (char *)argv[i];
		}
	}

	if ((dev_tty_name == 0) || (out_prof == 0)) {
		printf("Usage: profterm [-s speed] <dev_tty> <prof> [out]\n");
		printf("       <dev_tty> Device TTY\n");
		printf("       <prof>    Profiler output data\n");
		printf("       [out]     [OPTIONAL] Output file (profiler data filtered out)\n");
		printf("  Options:\n");
		printf("       -s [SPEED] UART baud rate (default:921600)\n");
		goto err_set_console;
	}

	if (out_console != 0) {
		console_out_file = fopen(out_console, "w");
		if (console_out_file == 0) {
			printf("Cannot open %s: error %s\n",
					(char *)out_console,
					strerror(errno));
			goto err_set_console;
		}
	}

	prof_out_file = fopen(out_prof, "w");
	if (prof_out_file == 0) {
		printf("Cannot open %s: error %s\n", (char *)out_prof, strerror(errno));
		goto err_out_prof;
	}

	while (dev_tty <= 0) {
		dev_tty = open(dev_tty_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
		if ((dev_tty <= 0) && (flag == 0)) {
			printf("Waiting for connection [CTRL+C to abort]...\n");
			flag = 1;
		}
	}

	if (uart_set(dev_tty, baud_rate) < 0)
		goto err_uart_set;

	/* Start TX main loop */
	if (pthread_create(&tx_thread, NULL, tx_process, NULL)) {
		printf("Cannot create TX thread: %s\n", strerror(errno));
		goto err_uart_set;
	}

	printf("~~~ READY [Press CTRL+C to stop] ~~~\n");

	/* RX main loop */
	rx_process();

	/* Should never come here... */
	return 0;

	/* Error handling */
err_uart_set:
	close(dev_tty);
	fclose(prof_out_file);
err_out_prof:
	if (console_out_file)
		fclose(console_out_file);
err_set_console:
	tcsetattr(STDIN_FILENO, TCSANOW, &host_tty_save);
exit:
	return 1;
}
