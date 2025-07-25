/**
 * Copyright (c) 2023 Arducam Technology Co., Ltd. <www.arducam.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/uart.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

#define SET_PICTURE_RESOLUTION   0X01
#define SET_VIDEO_RESOLUTION     0X02
#define GET_CAMERA_INFO          0X0F
#define TAKE_PICTURE             0X10
#define STOP_STREAM              0X21
#define CONTROL_WRITE			 0xCC
#define CONTROL_QUERY_GET		 0xCD

#define TYPE_BUF_SIZE 8
#define NAME_BUF_SIZE 20
#define QUERY_BUF_SIZE 800

#define MSG_SIZE 12
/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

const struct device *console;
const struct device *video;
struct video_buffer *vbuf;

volatile uint8_t preview_on;
volatile uint8_t capture_flag;

const uint32_t pixel_format_table[] = {
	VIDEO_PIX_FMT_JPEG,
	VIDEO_PIX_FMT_RGB565,
	VIDEO_PIX_FMT_YUYV,
};

const uint16_t resolution_table[][2] = {
	{160, 120},  {320, 240},   {640, 480},   {800, 600},   {1280, 720},
	{1280, 960}, {1600, 1200}, {1920, 1080}, {2048, 1536}, {2592, 1944},
	{96, 96},    {128, 128},   {320, 320},
};

const uint8_t resolution_num = sizeof(resolution_table) / 4;
static uint8_t current_resolution;
static uint8_t take_picture_fmt = 0x1a;
static uint8_t head_and_tail[] = {0xff, 0xaa, 0x00, 0xff, 0xbb};

enum video_buf_type type = VIDEO_BUF_TYPE_OUTPUT;

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

void serial_cb(const struct device *dev, void *user_data);

static int set_mega_resolution(uint8_t sfmt)
{
	uint8_t resolution = sfmt & 0x0f;
	uint8_t pixelformat = (sfmt & 0x70) >> 4;

	if (resolution > resolution_num || pixelformat > 3) {
		return -1;
	}
	struct video_format fmt = {.width = resolution_table[resolution][0],
				   .height = resolution_table[resolution][1],
				   .pixelformat = pixel_format_table[pixelformat - 1],
					.type = type};
	current_resolution = resolution;
	return video_set_format(video, &fmt);
}

static void uart_buffer_send(const struct device *dev, uint8_t *buffer, uint32_t length)
{
	for (uint32_t i = 0; i < length; i++) {
		uart_poll_out(dev, buffer[i]);
	}
}

static void uart_packet_send(uint8_t type, uint8_t *buffer, uint32_t length)
{
	head_and_tail[2] = type;
	uart_buffer_send(console, &head_and_tail[0], 3);
	uart_buffer_send(console, (uint8_t *)&length, 4);
	uart_buffer_send(console, buffer, length);
	uart_buffer_send(console, &head_and_tail[3], 2);
}

static int take_picture(void)
{
	int err;
	enum video_frame_fragmented_status f_status;

	err = video_dequeue(video, &vbuf, K_FOREVER);
	if (err) {
		LOG_ERR("Unable to dequeue video buf");
		return -1;
	}

	f_status = vbuf->flags;

	head_and_tail[2] = 0x01;
	uart_buffer_send(console, &head_and_tail[0], 3);
	uart_buffer_send(console, (uint8_t *)&vbuf->bytesframe, 4);
	uart_poll_out(console, ((current_resolution & 0x0f) << 4) | 0x01);

	uart_buffer_send(console, vbuf->buffer, vbuf->bytesused);

	video_enqueue(video, vbuf);
	while (f_status == VIDEO_BUF_FRAG) {
		video_dequeue(video, &vbuf, K_FOREVER);
		f_status = vbuf->flags;
		uart_buffer_send(console, vbuf->buffer, vbuf->bytesused);
		video_enqueue(video, vbuf);
	}
	uart_buffer_send(console, &head_and_tail[3], 2);

	return 0;
}

static void video_preview(void)
{
	int err;
	enum video_frame_fragmented_status f_status;

	if (!preview_on) {
		return;
	}

	err = video_dequeue(video, &vbuf, K_FOREVER);
	if (err) {
		LOG_ERR("Unable to dequeue video buf");
		return;
	}

	f_status = vbuf->flags;

	if (capture_flag == 1) {
		capture_flag = 0;
		head_and_tail[2] = 0x01;
		uart_buffer_send(console, &head_and_tail[0], 3);
		uart_buffer_send(console, (uint8_t *)&vbuf->bytesframe, 4);
		uart_poll_out(console, ((current_resolution & 0x0f) << 4) | 0x01);
	}

	uart_buffer_send(console, vbuf->buffer, vbuf->bytesused);

	if (f_status == VIDEO_BUF_EOF) {
		uart_buffer_send(console, &head_and_tail[3], 2);
		capture_flag = 1;
	}

	err = video_enqueue(video, vbuf);
	if (err) {
		LOG_ERR("Unable to requeue video buf");
		return;
	}
}

static int get_video_query_info(uint32_t id)
{
    char str_buf[QUERY_BUF_SIZE];
    uint32_t str_len;
    int offset = 0;

    const char *type_names[] = {
        "invalid",
        "bool",
        "int",
        "int64",
        "int64",
        "menu",
        "integer menu",
        "string"
    };
    struct video_ctrl_query cq = {
        .dev = video,
        .id = id
    };
    struct video_control vc = {.id = cq.id, .val = 0};

	if (video_query_ctrl(&cq) != 0) {
		LOG_ERR("Video control query did not find item with an id %d", cq.id);
		return -1;
	}

	vc.id = cq.id;
	video_get_ctrl(cq.dev, &vc);

	const char *type = type_names[cq.type];
	char typebuf[TYPE_BUF_SIZE];
	char namebuf[NAME_BUF_SIZE];

	snprintf(typebuf, sizeof(typebuf), "(%s)", type);

	if (cq.name == NULL) {
		snprintf(namebuf, sizeof(namebuf), "Private Control");
		cq.name = namebuf;
	}

	offset += snprintf(str_buf + offset, QUERY_BUF_SIZE - offset,
		"Video ctrl query result: %s 0x%08x %-8s (flags=0x%02x) : min=%d max=%d step=%d default=%d value=%d \r\n",
		cq.name, cq.id, typebuf, cq.flags, cq.range.min, cq.range.max,
		cq.range.step, cq.range.def, vc.val);

    str_len = strlen(str_buf);
    uart_packet_send(0x02, str_buf, str_len);
    return 0;
}

static uint8_t recv_process(uint8_t *buff)
{
	struct video_control ctrl = {.id = VIDEO_CID_BRIGHTNESS, .val = 1};

	switch (buff[0]) {
		case SET_PICTURE_RESOLUTION:
			if (set_mega_resolution(buff[1]) == 0) {
				take_picture_fmt = buff[1];
			}
			break;
		case SET_VIDEO_RESOLUTION:
			if (preview_on == 0) {
				set_mega_resolution(buff[2] | 0x10);
				video_stream_start(video, type);
				capture_flag = 1;
			}
			preview_on = 1;
			break;
		case TAKE_PICTURE:
			video_stream_start(video, type);
			take_picture();
			video_stream_stop(video, type);
			break;
		case STOP_STREAM:
			if (preview_on) {
				uart_buffer_send(console, &head_and_tail[3], 2);
				video_stream_stop(video, type);
				set_mega_resolution(take_picture_fmt);
			}
			preview_on = 0;
			break;
		case CONTROL_WRITE:
			ctrl.id = (buff[1] << 24) | (buff[2] << 16) | (buff[3] << 8) | buff[4];
			ctrl.val = (buff[5] << 24) | (buff[6] << 16) | (buff[7] << 8) | buff[8];
			video_set_ctrl(video, &ctrl);
			break;
		case CONTROL_QUERY_GET:
			uint32_t q_id = (buff[1] << 24) | (buff[2] << 16) | (buff[3] << 8) | buff[4];
			get_video_query_info(q_id);
			break;
		default:
			break;
	}

	return buff[0];
}

static uint8_t uart_available(uint8_t *p)
{
	return k_msgq_get(&uart_msgq, p, K_NO_WAIT);
}

int main(void)
{
	uint8_t recv_buffer[12] = {0};
	struct video_buffer *buffers[3];
	int i = 0;

	console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!device_is_ready(console)) {
		LOG_ERR("%s: device not ready.", console->name);
		return -1;
	}
	uart_irq_callback_user_data_set(console, serial_cb, NULL);
	uart_irq_rx_enable(console);

	video = DEVICE_DT_GET(DT_NODELABEL(arducam_mega0));
	
	if (!device_is_ready(video)) {
		LOG_ERR("Video device %s not ready.", video->name);
		return -1;
	}
	/* Alloc video buffers and enqueue for capture */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		buffers[i] = video_buffer_alloc(1024, K_NO_WAIT);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return -1;
		}
		video_enqueue(video, buffers[i]);
	}

	printk("- Device name: %s\n", video->name);

	while (1) {
		if (!uart_available(recv_buffer)) {
			recv_process(recv_buffer);
		}
		video_preview();
		k_msleep(1);
	}
	return 0;
}

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(dev)) {
		return;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	/* read until FIFO empty */
	while (uart_fifo_read(dev, &c, 1) == 1) {
		if (c == 0xAA && rx_buf_pos > 0) {
			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
			rx_buf_pos = 0;
		} else if (c == 0x55) {
			rx_buf_pos = 0;
		} else {
			rx_buf[rx_buf_pos] = c;
			if (++rx_buf_pos >= MSG_SIZE) {
				rx_buf_pos = 0;
			}
		}
	}
}
