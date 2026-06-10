/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/drivers/sdev.h>
#include <zephyr/sd_dev/sd_dev.h>
#include <zephyr/sd_dev/sdio_dev.h>

LOG_MODULE_REGISTER(sdio_device_app, LOG_LEVEL_INF);

/* ============================================
 * Protocol Definitions (same as Linux side)
 * ============================================ */
typedef enum {
    MSDIO_APP_CMD_TYPE = 0,
    MSDIO_APP_DATA_TYPE
} app_data_type;

typedef struct {
    uint32_t type;
    uint32_t length;
    uint8_t data[0];
} __attribute__((packed)) msdio_packet_t;

typedef enum {
    TEST_CMD_LOOPBACK = 1,
    TEST_CMD_DATA_TX  = 2,
    TEST_CMD_DATA_RX  = 3,
    TEST_CMD_STOP     = 99
} test_command_t;

typedef struct {
    uint32_t cmd;
    uint32_t packet_size;
    uint32_t packet_count;
} __attribute__((packed)) test_cmd_packet_t;

/* ============================================
 * Buffer Management
 * ============================================ */
#define MAX_PACKET_SIZE 4096
#define RX_BUFFER_SIZE  (sizeof(msdio_packet_t) + MAX_PACKET_SIZE)
#define TX_BUFFER_SIZE  (sizeof(msdio_packet_t) + MAX_PACKET_SIZE)

static uint8_t tx_buffer[TX_BUFFER_SIZE] __aligned(4);

/* SDIO function handle */
static struct sdio_dev_func *sdio_func;

/* Statistics */
static uint32_t rx_packet_count;
static uint32_t rx_byte_count;

/* ============================================
 * Packet Send/Receive
 * ============================================ */
static int send_packet(app_data_type type, const void *data,
               uint32_t data_len)
{
    msdio_packet_t *pkt = (msdio_packet_t *)tx_buffer;
    uint32_t total_len;
    int ret;

    if (data_len > MAX_PACKET_SIZE) {
        LOG_ERR("Data too large: %u", data_len);
        return -1;
    }

    pkt->type   = type;
    pkt->length = data_len;

    if (data && data_len > 0) {
        memcpy(pkt->data, data, data_len);
    }

    total_len = sizeof(msdio_packet_t) + data_len;

    LOG_DBG("Sending packet: type=%d, len=%u, total=%u",
        type, data_len, total_len);

    ret = sdio_write(sdio_func, tx_buffer, total_len);
    if (ret < 0) {
        LOG_ERR("sdev_write failed: %d", ret);
        return ret;
    }

    LOG_DBG("Packet sent successfully");
    return 0;
}

static sdev_pkt_t *recv_packet(void)
{
    sdev_pkt_t *sdev_pkt = sdio_read_pkt(sdio_func);

    if (!sdev_pkt) {
        LOG_ERR("sdev_read failed");
        return NULL;
    }

    if (sdev_pkt->len < sizeof(msdio_packet_t)) {
        LOG_ERR("Packet too small");
        return NULL;
    }

    return sdev_pkt;
}

/* ============================================
 * Command Handlers
 * ============================================ */
static void handle_cmd_loopback(test_cmd_packet_t *cmd)
{
    test_cmd_packet_t response;

    LOG_INF("CMD_LOOPBACK received");

    response.cmd          = cmd->cmd;
    response.packet_size  = cmd->packet_size;
    response.packet_count = cmd->packet_count;

    send_packet(MSDIO_APP_CMD_TYPE, &response, sizeof(response));

    LOG_INF("Loopback response sent");
}

static void handle_cmd_data_tx(test_cmd_packet_t *cmd)
{
    msdio_packet_t *pkt;
    int64_t start_time, end_time, elapsed_ms;
    uint32_t throughput_bps;
    uint32_t expect_packet_count = cmd->packet_count;

    LOG_INF("CMD_DATA_TX: size=%u, count=%u",
        cmd->packet_size, cmd->packet_count);

    rx_packet_count = 0;
    rx_byte_count   = 0;

    start_time = k_uptime_get();

    for (uint32_t i = 0; i < expect_packet_count; i++) {
        sdev_pkt_t *sdev_pkt = recv_packet();

        if (!sdev_pkt) {
            LOG_ERR("Failed to receive packet %u", i);
            break;
        }

        pkt = (msdio_packet_t *)sdev_pkt->data;

        if (pkt->type != MSDIO_APP_DATA_TYPE) {
            LOG_WRN("Wrong packet type: %d", pkt->type);
            continue;
        }

        sdev_pkt_free(sdev_pkt);

        rx_packet_count++;
        rx_byte_count += pkt->length;

        if ((i + 1) % 1000 == 0) {
            LOG_INF("Received %u packets", i + 1);
        }
    }

    end_time   = k_uptime_get();
    elapsed_ms = end_time - start_time;

    LOG_INF("Data RX completed: %u packets, %u bytes",
        rx_packet_count, rx_byte_count);

    if (elapsed_ms > 0) {
        throughput_bps =
            (uint32_t)((uint64_t)rx_byte_count * 8 * 1000 /
                   elapsed_ms);

        LOG_INF("Elapsed time: %lld ms", elapsed_ms);
        LOG_INF("Throughput: %u bps (%u.%03u Kbps, %u.%03u Mbps)",
            throughput_bps,
            throughput_bps / 1000,
            throughput_bps % 1000,
            throughput_bps / 1000000,
            (throughput_bps % 1000000) / 1000);
    } else {
        LOG_WRN("Elapsed time is 0, cannot calculate throughput");
    }
}

static void handle_cmd_data_rx(test_cmd_packet_t *cmd)
{
    test_cmd_packet_t response;
    uint8_t *data_buf;
    uint32_t i, j;
    uint32_t expect_packet_count = cmd->packet_count;

    LOG_INF("CMD_DATA_RX: size=%u, count=%u",
        cmd->packet_size, cmd->packet_count);

    response.cmd          = cmd->cmd;
    response.packet_size  = cmd->packet_size;
    response.packet_count = cmd->packet_count;

    send_packet(MSDIO_APP_CMD_TYPE, &response, sizeof(response));

    k_msleep(100);

    data_buf = k_malloc(cmd->packet_size);
    if (!data_buf) {
        LOG_ERR("Failed to allocate buffer");
        return;
    }

    for (j = 0; j < cmd->packet_size; j++) {
        data_buf[j] = j & 0xFF;
    }

    LOG_INF("Starting to send %u packets...", cmd->packet_count);

    for (i = 0; i < expect_packet_count; i++) {
        if (send_packet(MSDIO_APP_DATA_TYPE, data_buf,
                cmd->packet_size) < 0) {
            LOG_ERR("Failed to send packet %u", i);
            break;
        }

        if ((i + 1) % 1000 == 0) {
            LOG_INF("Sent %u packets", i + 1);
        }

        k_yield();
    }

    k_free(data_buf);

    LOG_INF("Data TX completed: %u packets", i);
}

static void handle_cmd_stop(test_cmd_packet_t *cmd)
{
    test_cmd_packet_t response;

    LOG_INF("CMD_STOP received");

    response.cmd          = cmd->cmd;
    response.packet_count = rx_packet_count;
    response.packet_size  = rx_byte_count;

    send_packet(MSDIO_APP_CMD_TYPE, &response, sizeof(response));

    LOG_INF("Statistics sent: packets=%u, bytes=%u",
        rx_packet_count, rx_byte_count);
}

/* ============================================
 * Main Command Processing
 * ============================================ */
static void process_command(void *data)
{
    msdio_packet_t *pkt = (msdio_packet_t *)data;
    test_cmd_packet_t *cmd;

    if (pkt->type != MSDIO_APP_CMD_TYPE) {
        LOG_WRN("Not a command packet: type=%d", pkt->type);
        return;
    }

    cmd = (test_cmd_packet_t *)pkt->data;

    LOG_INF("Processing command: %d", cmd->cmd);

    switch (cmd->cmd) {
    case TEST_CMD_LOOPBACK:
        handle_cmd_loopback(cmd);
        break;
    case TEST_CMD_DATA_TX:
        handle_cmd_data_tx(cmd);
        break;
    case TEST_CMD_DATA_RX:
        handle_cmd_data_rx(cmd);
        break;
    case TEST_CMD_STOP:
        handle_cmd_stop(cmd);
        break;
    default:
        LOG_WRN("Unknown command: %d", cmd->cmd);
        break;
    }
}

/* ============================================
 * Command Processing Thread
 * ============================================ */
#define CMD_THREAD_STACK_SIZE 4096
#define CMD_THREAD_PRIORITY   3

K_THREAD_STACK_DEFINE(cmd_thread_stack, CMD_THREAD_STACK_SIZE);
static struct k_thread cmd_thread_data;

static void cmd_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("Command processing thread started");

    while (1) {
        sdev_pkt_t *sdev_pkt = recv_packet();

        if (sdev_pkt) {
            process_command(sdev_pkt->data);
            sdev_pkt_free(sdev_pkt);
        }
    }
}

/* ============================================
 * Zephyr application entry
 * ============================================ */
int main(void)
{
    const struct device *sdio_dev =
        DEVICE_DT_GET(DT_NODELABEL(sdio_device0));
    struct sdev_card *card = sdio_dev->data;
    struct sdio_dev *sdio = card->sdio;
    k_tid_t tid;

    sdio_func = sdio->funcs[0];

    if (!device_is_ready(sdio_dev)) {
        LOG_ERR("SDIO device not ready");
        return -1;
    }

    LOG_INF("RW6xx SDIO device ready");

    tid = k_thread_create(&cmd_thread_data, cmd_thread_stack,
                  CMD_THREAD_STACK_SIZE,
                  cmd_thread_entry,
                  NULL, NULL, NULL,
                  CMD_THREAD_PRIORITY, 0, K_NO_WAIT);

    if (tid == NULL) {
        LOG_ERR("Failed to create command thread");
        return -1;
    }

    k_thread_name_set(tid, "cmd_thread");

    LOG_INF("SDIO Application Initialized");
    LOG_INF("Command thread created successfully");

    while (1) {
        if (sdev_card_is_enumed(card)) {
            if (card->is_enum == false) {
                card->is_enum = true;

                LOG_INF("SD CARD unenum ---> enumed");

                sdio_func->block_size =
                    sdio_dev_get_block_size(sdio_func);
                sdio_func->func_code =
                    sdio_dev_read_fn_code(sdio_func);

                sdio->bus_speed =
                    sdio_dev_get_bus_speed(sdio);
                sdio->bus_width =
                    sdio_dev_get_bus_width(sdio);

                LOG_INF("sdio func%d enumed value: block_size=%d func_code=%d",
                    sdio_func->fn,
                    sdio_func->block_size,
                    sdio_func->func_code);

                LOG_INF("sdio enumed value: speed=%d width=%d",
                    sdio->bus_speed,
                    sdio->bus_width);
            }
        } else if (card->is_enum == true) {
            card->is_enum = false;

            LOG_INF("SD CARD change enumed ---> unenum");
        }

        k_sleep(K_MSEC(10 * 1000));
    }

    return 0;
}
