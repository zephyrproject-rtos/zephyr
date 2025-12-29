/*
 * Copyright (c) 2025 Paul Wedeck <paulwedeck@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Compile using `make recv_can_log` or `gcc -o recv_can_log recv_can_log.c`.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

struct topic_data {
	unsigned char *str;
	size_t capacity;
	size_t length;
	canid_t can_id;
};

struct topic_data *topics;
size_t topic_count;

static void post_topic(struct topic_data *topic, uint8_t *data, uint8_t length)
{
	ssize_t last_nl = -1;
	size_t req_cap = topic->length + length + 1;
	unsigned char prev;

	if (topic->capacity < req_cap) {
		topic->str = realloc(topic->str, req_cap);
		topic->capacity = req_cap;
	}

	/* append received message */
	memcpy(topic->str + topic->length, data, length);
	topic->length += length;
	topic->str[topic->length] = '\0';

	/* detect if a full line is in the buffer */
	for (size_t i = topic->length - length; i < topic->length; i++) {
		if (topic->str[i] == '\n') {
			last_nl = i;
		}

		if (topic->str[i] != '\n' && topic->str[i] < ' ') {
			/* mask out special characters except for newline */
			topic->str[i] = ' ';
		}
	}

	if (last_nl == -1) {
		return;
	}

	/* print out message */
	prev = topic->str[last_nl + 1];
	topic->str[last_nl + 1] = '\0';
	if (topic_count > 1) {
		if ((topic->can_id & CAN_EFF_FLAG) > 0) {
			printf("[%08x] ", topic->can_id & CAN_EFF_MASK);
		} else {
			printf("[%03x] ", topic->can_id & CAN_SFF_MASK);
		}
	}
	printf("%s", topic->str);

	/* restore previous state and remove send lines */
	topic->str[last_nl + 1] = prev;
	memmove(&topic->str[0], &topic->str[last_nl + 1], topic->length - last_nl);
	topic->length -= (last_nl + 1);
}

static void handle_frame(canid_t can_id, uint8_t *data, uint8_t length)
{
	for (size_t i = 0; i < topic_count; i++) {
		if (topics[i].can_id == can_id) {
			post_topic(&topics[i], data, length);
			return;
		}
	}
}

static int parse_can_id(char *canid, struct can_filter *filter)
{
	long value = 0;
	char *endptr = NULL;
	size_t len = strlen(canid);

	if (len != 3 && len != 8) {
		fprintf(stderr,
			"\"%s\" is not a valid can id!\n"
			"can ids are either 3 (standard id) or 8 (extended id) hex chars!\n",
			canid);
		return -1;
	}

	value = strtol(canid, &endptr, 16);
	if (*endptr != '\0') {
		fprintf(stderr, "%s is not a valid hex string, starting at %s\n", canid, endptr);
		return -1;
	}

	if (len == 8) {
		if ((value & CAN_EFF_MASK) != value) {
			fprintf(stderr, "%s out of bounds for an extended can id\n", canid);
			return -1;
		}

		filter->can_id = value | CAN_EFF_FLAG;
		filter->can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_EFF_MASK);
	} else if (len == 3) {
		if ((value & CAN_SFF_MASK) != value) {
			fprintf(stderr, "%s out of bounds for an a standard can id\n", canid);
			return -1;
		}

		filter->can_id = value;
		filter->can_mask = (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_SFF_MASK);
	} else {
		return -1; /* unreachable */
	}

	return 0;
}

static int setup_filter(int sock, int count, char **canid_str)
{
	struct can_filter *filter = malloc(count * sizeof(struct can_filter));

	for (int i = 0; i < count; i++) {
		if (parse_can_id(canid_str[i], &filter[i]) < 0) {
			free(filter);
			return -1;
		}
	}

	if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, filter,
		       count * sizeof(struct can_filter)) < 0) {
		perror("setsockopt(CAN_RAW_FILTER)");
		free(filter);
		return -1;
	}

	topics = calloc(count, sizeof(struct topic_data));
	for (int i = 0; i < count; i++) {
		topics[i].can_id = filter[i].can_id;
	}
	topic_count = count;

	free(filter);
	return 0;
}

int create_socket(char *ifname)
{
	int sock;
	struct sockaddr_can addr;
	struct ifreq ifr;
	int one = 1;
	int ret;

	sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	strcpy(ifr.ifr_name, ifname);
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		perror("ioctl");
		return -1;
	}

	ret = setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &one, sizeof(one));
	if (ret < 0) {
		fprintf(stderr, "Failed to put socket into CAN FD mode!\n");
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}
	return sock;
}

int main(int argc, char **argv)
{
	int sock;
	int ret;
	struct canfd_frame cfd;

	if (argc <= 1) {
		fprintf(stderr,
			"%s: <can if> <can id...>\n"
			"can id as 3 (short id) or 8 (extended id) hex chars\n",
			argv[0]);
		return 1;
	}

	sock = create_socket(argv[1]);
	if (sock < 0) {
		return 1;
	}

	if (setup_filter(sock, argc - 2, argv + 2) < 0) {
		return 1;
	}

	while (true) {
		ret = read(sock, &cfd, CANFD_MTU);
		if (ret < 0 && errno != EINTR) {
			perror("read");
			return 1;
		}

		if (ret > 0) {
			handle_frame(cfd.can_id, cfd.data, cfd.len);
		}
	}
}
