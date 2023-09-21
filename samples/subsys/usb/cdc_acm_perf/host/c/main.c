#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <time.h>

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: usb-cdc-perf <serial port>\n");
		return -1;
	}

	int fd = open(argv[1], O_RDWR);

	if (fd < 0) {
		perror("Error opening serial port");
		exit(-errno);
	}

	struct termios newtio;

	bzero(&newtio, sizeof(newtio));

	newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;

	uint8_t rb[1024];
	int rb_left_cnt = 0;

	// not sure the below is working
	newtio.c_cc[VMIN] = 32;

	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	int status;

	if (ioctl(fd, TIOCMGET, &status) == -1) {
		perror("TIOCMGET failed");
		exit(-1);
	}

	status |= TIOCM_DTR;

	if (ioctl(fd, TIOCMSET, &status) == -1) {
		perror("TIOCMSET failed");
		exit(-1);
	}

	uint32_t value = 0;
	int block_count = 0;

	time_t last_perf_calc;

	time(&last_perf_calc);
	int perf_bytes = 0;

	int error_cnt = 0;
	int left_cnt = 0;

	while (1) {
		int c = read(fd, &rb[rb_left_cnt], sizeof(rb) - rb_left_cnt);
		if (c > 0) {
			int val_count = (c + rb_left_cnt) / 4;

			perf_bytes += c;

			uint32_t *values = (uint32_t *)rb;

			for (int i = 0; i < val_count; i++) {
				if (value != values[i]) {
					printf("bad data, exp: 0x%x, got: 0x%x\n", value,
					       values[i]);
					// adjust expected value
					error_cnt++;
					value = values[i] + 1;
				} else {
					value++;
				}
			}

			rb_left_cnt = c % 4;

			if (rb_left_cnt != 0) {
				printf("left over date cnt: %i\n", rb_left_cnt);
				left_cnt++;
				// for now we throw away left-over data as it appears when this
				// happens, we lose a byte
				rb_left_cnt = 0;
				/*
				// copy left-over bytes down to beginning of buffer
				int start = val_count * 4;
				for (int i = 0; i < rb_left_cnt; i++) {
					rb[i] = rb[start + i];
				}
				*/
			}
		}
		block_count++;

		time_t now;
		time(&now);
		double diff = difftime(now, last_perf_calc);
		if (diff > 0.95) {
			// calc rate in bit/sec
			double rate = (perf_bytes / diff) * 8;
			char *modifier = "";
			if (rate > 1000 * 1000) {
				rate /= 1000 * 1000;
				modifier = "M";
			} else if (rate > 1000) {
				rate /= 1000;
				modifier = "K";
			}
			printf("Read %i bytes, Data rate: %.2lf %sb/sec, error cnt: %i, leftover "
			       "cnt: %i\n",
			       perf_bytes, rate, modifier, error_cnt, left_cnt);
			last_perf_calc = now;
			perf_bytes = 0;
		}
	}

	return 0;
}
