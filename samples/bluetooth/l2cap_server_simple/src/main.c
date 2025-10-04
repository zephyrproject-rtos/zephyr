/* samples/bluetooth/l2cap_server_simple/src/main.c
 *
 * Minimal L2CAP server sample demonstrating fixed allocation per-connection.
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/printk.h>

#define PSM 0x29 /* example PSM */

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);
static void l2cap_connected(struct bt_l2cap_chan *chan);
static void l2cap_disconnected(struct bt_l2cap_chan *chan);

/* ops used for every channel instance */
static struct bt_l2cap_chan_ops l2cap_ops = {
    .recv       = l2cap_recv,
    .connected  = l2cap_connected,
    .disconnected = l2cap_disconnected,
};

/* Fixed array of channel instances, one per possible connection */
static struct bt_l2cap_chan fixed_chan[CONFIG_BT_MAX_CONN];

static int accept_cb(struct bt_conn *conn, struct bt_l2cap_server *server,
                     struct bt_l2cap_chan **chan)
{
    uint8_t conn_index = bt_conn_index(conn);

    /* initialize the chosen entry */
    fixed_chan[conn_index] = (struct bt_l2cap_chan){
        .ops = &l2cap_ops,
    };

    *chan = &fixed_chan[conn_index];

    printk("L2CAP: accepted connection, assigned chan[%d]\n", conn_index);

    return 0; /* accept */
}

static struct bt_l2cap_server server = {
    .psm = PSM,
    .sec_level = BT_SECURITY_L1,
    .accept = accept_cb,
};

static void l2cap_connected(struct bt_l2cap_chan *chan)
{
    printk("L2CAP connected\n");
}

static void l2cap_disconnected(struct bt_l2cap_chan *chan)
{
    printk("L2CAP disconnected\n");
}

static int l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
    printk("L2CAP received %u bytes\n", buf->len);
    /* For synchronous processing, return 0 and the stack frees buf. */
    return 0;
}

void main(void)
{
    int err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed: %d\n", err);
        return;
    }

    err = bt_l2cap_server_register(&server);
    if (err) {
        printk("L2CAP server register failed: %d\n", err);
    } else {
        printk("L2CAP server registered, PSM %u\n", PSM);
    }
}
