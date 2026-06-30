#include <stdarg.h>
#include <stdio.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/fs/devfs.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include "uart_fs.h"

LOG_MODULE_REGISTER(uart_dev, LOG_LEVEL_INF);

#define UART_DEVICE_NUM DT_NUM_INST_STATUS_OKAY(st_stm32_uart)

#define UART_NODE_PROCESS(node_id)                       \
    IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, st_stm32_uart),     \
        (UART_NODE_INIT(node_id)))                       \

#define UART_NODE_INIT(node_id)                          \
    {                                                    \
        .dev = DEVICE_DT_GET(node_id),                   \
        .label = DT_PROP(node_id, label),                \
    },

struct uart_device
{
    const struct device *dev;
    const char *label;
    atomic_t is_open;
    atomic_t tx_busy;
    int fs_flag;
    struct ring_buf tx_rb;
    struct ring_buf rx_rb;
    uint8_t tx_buf[UART_FS_RB_SIZE];
    uint8_t rx_buf[UART_FS_RB_SIZE];
    struct k_poll_signal signal;
    struct k_sem rx_sem;
    struct k_sem tx_sem;
};

static struct uart_device uart_devices[] = {
    DT_FOREACH_STATUS_OKAY_NODE(UART_NODE_PROCESS)
};

static void uart_rx_handle(struct uart_device *uart_dev)
{
    uint8_t *data;
    uint32_t buf_len, rx_len;

    do {
        /* alloc free memory */
        buf_len = ring_buf_put_claim(&uart_dev->rx_rb, &data, uart_dev->rx_rb.size);

        if (buf_len > 0)
        {
            /* read uart rx fifo */
            rx_len = uart_fifo_read(uart_dev->dev, data, buf_len);
            ring_buf_put_finish(&uart_dev->rx_rb, rx_len);
            if (rx_len > 0)
            {
                k_sem_give(&uart_dev->rx_sem);
            }
        }
        else
        {
            uint8_t dummy;
            rx_len = uart_fifo_read(uart_dev->dev, &dummy, 1);
            printk("rx rb full !!\n");
        }
    } while (rx_len && (rx_len <= buf_len));

    k_poll_signal_raise(&uart_dev->signal, 1);
}

static void uart_tx_handle(struct uart_device *uart_dev)
{
    uint32_t len;
    const uint8_t *data;

    len = ring_buf_get_claim(&uart_dev->tx_rb, (uint8_t **)&data,
                    uart_dev->tx_rb.size);

    if (len)
    {
        /* fifo put */
        len = uart_fifo_fill(uart_dev->dev, data, len);
        /* update rb index */
        ring_buf_get_finish(&uart_dev->tx_rb, len);
        if (len > 0)
        {
            k_sem_give(&uart_dev->tx_sem);
        }
    }
    else
    {
        uart_irq_tx_disable(uart_dev->dev);
        atomic_set(&uart_dev->tx_busy, 0);
    }
}

static void uart_irq(const struct device *dev, void *user_data)
{
    struct uart_device *uart = user_data;
    uart_irq_update(uart->dev);

    if (uart_irq_rx_ready(uart->dev))
    {
        uart_rx_handle(uart);
    }

    if (uart_irq_tx_ready(uart->dev))
    {
        uart_tx_handle(uart);
    }
}

static int uart_open(struct fs_file_t *zfp, const char *name, fs_mode_t mode)
{
    struct uart_device *uart = NULL;

    for (int i = 0; i < UART_DEVICE_NUM; i++)
    {
        if (strcmp(uart_devices[i].label, name) == 0)
        {
            uart = &uart_devices[i];
            break;
        }
    }

    if (!uart || !device_is_ready(uart->dev))
    {
        LOG_ERR("Device %s not ready", name);
        return -ENODEV;
    }

    if (!atomic_cas(&uart->is_open, 0, 1))
    {
        LOG_ERR("Device %s already opened", name);
        return -EBUSY;
    }

    ring_buf_init(&uart->rx_rb, UART_FS_RB_SIZE, uart->rx_buf);
    ring_buf_init(&uart->tx_rb, UART_FS_RB_SIZE, uart->tx_buf);
    k_poll_signal_init(&uart->signal);
    k_sem_init(&uart->rx_sem, 0, 1);
    k_sem_init(&uart->tx_sem, 0, 1);

    uart_irq_callback_user_data_set(uart->dev, uart_irq, (void *)uart);
    uart_irq_rx_enable(uart->dev);

    zfp->filep = uart;

    return 0;
}

static int uart_close(struct fs_file_t *zfp)
{
    struct uart_device *uart = zfp->filep;

    uart_irq_rx_disable(uart->dev);
    atomic_set(&uart->is_open, 0);
    uart->fs_flag = 0;

    return 0;
}

static int uart_ioctl(struct fs_file_t* zfp, unsigned long request, va_list args)
{
    struct uart_device *dev = zfp->filep;
    int ret = 0;

    switch (request) {
    case TCGETATTR:
    {
        struct uart_config *config =  va_arg(args, struct uart_config*);
        ret = uart_config_get(dev->dev, config);
    }
    break;
    case TCSETATTR:
    {
        struct uart_config *config  = va_arg(args, struct uart_config *);
        ret = uart_configure(dev->dev, config);
    }
    break;
    case F_GETFL:
    {
        return O_RDWR;
    }
    break;
    case F_SETFL:
    {
        int flags = va_arg(args, int);
        dev->fs_flag = flags;
    }
    break;
    case ZFD_IOCTL_POLL_PREPARE:
    {
        struct zvfs_pollfd* pfd = va_arg(args, struct zvfs_pollfd*);
        struct k_poll_event** pev = va_arg(args, struct k_poll_event**);
        struct k_poll_event* pev_end = va_arg(args, struct k_poll_event*);
        if (pfd->events & ZVFS_POLLIN)
        {
            if (*pev == pev_end)
            {
                LOG_ERR("ZFD_IOCTL_POLL_PREPARE: no free poll event slot");
                return -ENOMEM;
            }

            (*pev)->obj = &dev->signal;
            (*pev)->type = K_POLL_TYPE_SIGNAL;
            (*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
            (*pev)->state = K_POLL_STATE_NOT_READY;
            (*pev)++;

            if (ring_buf_size_get(&dev->rx_rb))
            {
                /* rasie signal */
                k_poll_signal_raise(&dev->signal, 1);
            }
        }
    }
    break;
    case ZFD_IOCTL_POLL_UPDATE:
    {
        struct zvfs_pollfd* pfd = va_arg(args, struct zvfs_pollfd*);
        struct k_poll_event** pev = va_arg(args, struct k_poll_event**);
        if (pfd->events & ZVFS_POLLIN)
        {
            if ((*pev)->state == K_POLL_STATE_SIGNALED)
            {
                pfd->revents |= ZVFS_POLLIN;
                (*pev)->state = K_POLL_STATE_NOT_READY;
                k_poll_signal_reset(&dev->signal);
            }
            (*pev)++;
        }
    }
    break;
    default:
        return -ENOTTY;
    }
    return ret;
}

static ssize_t uart_read(struct fs_file_t *filp, void *dest, size_t nbytes)
{
    struct uart_device *uart_dev = filp->filep;
    size_t total = 0;
    bool nonblock = (uart_dev->fs_flag & O_NONBLOCK);

    while (total < nbytes)
    {
        size_t rx_len = ring_buf_get(&uart_dev->rx_rb,
                                (uint8_t *)dest + total,
                                nbytes - total);
        if (rx_len > 0)
        {
            total += rx_len;
            continue;
        }

        if (nonblock)
        {
            break;
        }

        k_sem_take(&uart_dev->rx_sem, K_FOREVER);
    }

    return total;
}

static ssize_t uart_write(struct fs_file_t *filp, const void *src, size_t size)
{
    struct uart_device *uart_dev = filp->filep;
    size_t total = 0;
    bool nonblock = (uart_dev->fs_flag & O_NONBLOCK);

    while (total < size)
    {
        size_t tx_len = ring_buf_put(&uart_dev->tx_rb,
                                (uint8_t *)src + total,
                                size - total);

        if (tx_len > 0)
        {
            total += tx_len;
            if (atomic_cas(&uart_dev->tx_busy, 0, 1))
            {
                uart_irq_tx_enable(uart_dev->dev);
            }
            continue;
        }

        if (nonblock)
        {
            return total ? total : -EAGAIN;
        }

        k_sem_take(&uart_dev->tx_sem, K_FOREVER);
    }

    return total;
}

static const struct fs_file_system_t uart_fs = {
    .open = uart_open,
    .close = uart_close,
    .ioctl = uart_ioctl,
    .read = uart_read,
    .write = uart_write
};

static int uart_fs_init(void)
{
    char name[32];
    for (int i = 0; i < UART_DEVICE_NUM; i++)
    {
        snprintf(name, sizeof(name) - 1, "/dev/%s", uart_devices[i].label);
        devfs_register(name, &uart_fs);
    }

    return 0;
}
SYS_INIT(uart_fs_init, APPLICATION, CONFIG_FILE_SYSTEM_INIT_PRIORITY);
