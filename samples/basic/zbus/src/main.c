#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

/* ===========================
 * Message Structure
 * =========================== */
struct sensor_data {
    int temperature;
    int humidity;
};

/* ===========================
 * Channel Definition
 * =========================== */
ZBUS_CHAN_DEFINE(sensor_chan,
                 struct sensor_data,
                 NULL,
                 NULL,
                 ZBUS_OBSERVERS(sensor_sub),
                 ZBUS_MSG_INIT(0)
);

/* ===========================
 * Subscriber Definition
 * (Requires queue_size in latest Zephyr)
 * =========================== */
ZBUS_SUBSCRIBER_DEFINE(sensor_sub, 4);

/* ===========================
 * Listener Callback
 * =========================== */
static void sensor_listener(const struct zbus_channel *chan)
{
    struct sensor_data msg;
    zbus_chan_read(chan, &msg, K_NO_WAIT);

    printk("[Listener] Temp=%d  Hum=%d\n",
           msg.temperature, msg.humidity);
}

ZBUS_LISTENER_DEFINE(sensor_listen, sensor_listener);

/* ===========================
 * Publisher Thread
 * =========================== */
void publisher_thread(void)
{
    struct sensor_data data = {0};

    while (1) {
        data.temperature += 1;
        data.humidity += 2;

        printk("[Publisher] Temp=%d  Hum=%d\n",
               data.temperature, data.humidity);

        zbus_chan_pub(&sensor_chan, &data, K_NO_WAIT);

        k_msleep(1000);
    }
}

/* ===========================
 * Subscriber Thread
 * =========================== */
void subscriber_thread(void)
{
    const struct zbus_channel *chan;
    struct sensor_data msg;

    while (1) {
        zbus_sub_wait(&sensor_sub, &chan, K_FOREVER);
        zbus_chan_read(chan, &msg, K_NO_WAIT);

        printk("[Subscriber] Temp=%d Hum=%d\n",
               msg.temperature, msg.humidity);
    }
}

/* ===========================
 * Thread Definitions
 * =========================== */
K_THREAD_DEFINE(pub_thread_id, 1024,
                publisher_thread, NULL, NULL, NULL,
                4, 0, 0);

K_THREAD_DEFINE(sub_thread_id, 1024,
                subscriber_thread, NULL, NULL, NULL,
                5, 0, 0);

