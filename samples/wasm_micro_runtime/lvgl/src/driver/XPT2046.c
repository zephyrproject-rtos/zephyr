/**
 * @file XPT2046.c
*/
/*********************
 *      INCLUDES
 *********************/
#include "XPT2046.h"
#include "board_config.h"
#include "stdio.h"
#include <string.h>
#include "spi.h"

#include "zephyr.h"
#include "kernel.h"

#if USE_XPT2046

#include <stddef.h>

#define abs(x) ((x) < 0 ? -(x) : (x))

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void xpt2046_corr(int16_t * x, int16_t * y);
static void xpt2046_avg(int16_t * x, int16_t * y);

/**********************
 *  STATIC VARIABLES
 **********************/
int16_t avg_buf_x[XPT2046_AVG];
int16_t avg_buf_y[XPT2046_AVG];
uint8_t avg_last;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the XPT2046
 */
struct device *input_dev;

struct spi_config spi_conf_xpt2046;
struct spi_cs_control xpt2046_cs_ctrl;
struct device *xpt2046_pen_gpio_dev;
static struct gpio_callback gpio_cb;
lv_indev_data_t touch_point;
lv_indev_data_t last_touch_point;

#define TOUCH_READ_THREAD_STACK_SIZE 4096
static K_THREAD_STACK_DEFINE(touch_read_thread_stack, TOUCH_READ_THREAD_STACK_SIZE);
static struct k_thread touch_thread_data;
static struct k_sem sem_touch_read;

K_MUTEX_DEFINE( spi_display_touch_mutex);

int cnt = 0;
int touch_read_times = 0;
int last_pen_interrupt_time = 0;
void xpt2046_pen_gpio_callback(struct device *port, struct gpio_callback *cb,
        u32_t pins)
{
    int i;
    cnt++;
    if ((k_uptime_get_32() - last_pen_interrupt_time) > 500) {
        k_sem_give(&sem_touch_read);
        touch_read_times++;
        last_pen_interrupt_time = k_uptime_get_32();
    }

}

void disable_pen_interrupt()
{
    int ret = 0;
    ret = gpio_pin_disable_callback(xpt2046_pen_gpio_dev, XPT2046_PEN_GPIO_PIN);
    if (ret != 0) {
        printf("gpio_pin_configure GPIO_DIR_IN failed\n");
    }
}
void enable_pen_interrupt()
{
    int ret = 0;
    ret = gpio_pin_enable_callback(xpt2046_pen_gpio_dev, XPT2046_PEN_GPIO_PIN);
    if (ret != 0) {
        printf("gpio_pin_configure failed\n");
    }
}

void touch_screen_read_thread()
{
    int i;
    bool ret = false;

    for (;;) {
        k_sem_take(&sem_touch_read, K_FOREVER);
        memset(&last_touch_point, 0, sizeof(lv_indev_data_t));
        memset(&touch_point, 0, sizeof(lv_indev_data_t));
        memset(avg_buf_x, 0, sizeof(avg_buf_x));
        memset(avg_buf_y, 0, sizeof(avg_buf_y));
        k_mutex_lock(&spi_display_touch_mutex, K_FOREVER);
        disable_pen_interrupt();
        for (i = 0; i < 100; i++) {
            ret = xpt2046_read(&touch_point);
            if (ret) {
                if ((abs(last_touch_point.point.x - touch_point.point.x) < 4)
                        && (abs(last_touch_point.point.y - touch_point.point.y)
                                < 4)) {
                    break;
                }
                last_touch_point = touch_point;

            }
        }
        enable_pen_interrupt();
        k_mutex_unlock(&spi_display_touch_mutex);
    }
}

void xpt2046_init(void)
{
    int ret;
    input_dev = device_get_binding(XPT2046_SPI_DEVICE_NAME);

    if (input_dev == NULL) {
        printf("device not found.  Aborting test.");
        return;
    }
    memset((void *) &touch_point, 0, sizeof(lv_indev_data_t));

    spi_conf_xpt2046.frequency = XPT2046_SPI_MAX_FREQUENCY;
    spi_conf_xpt2046.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
    spi_conf_xpt2046.slave = 0;
    spi_conf_xpt2046.cs = NULL;
#ifdef XPT2046_CS_GPIO_CONTROLLER
    xpt2046_cs_ctrl.gpio_dev = device_get_binding(XPT2046_CS_GPIO_CONTROLLER);
    if (xpt2046_cs_ctrl.gpio_dev == NULL) {
        printk("Cannot find %s!\n", XPT2046_CS_GPIO_CONTROLLER);
        return;
    }
    gpio_pin_configure(xpt2046_cs_ctrl.gpio_dev, XPT2046_CS_GPIO_PIN,
                       GPIO_DIR_OUT);
    gpio_pin_write(xpt2046_cs_ctrl.gpio_dev, XPT2046_CS_GPIO_PIN, 1);
    xpt2046_cs_ctrl.gpio_pin = XPT2046_CS_GPIO_PIN;
    xpt2046_cs_ctrl.delay = 0;
    spi_conf_xpt2046.cs = &xpt2046_cs_ctrl;

#endif

#ifdef XPT2046_PEN_GPIO_CONTROLLER

    xpt2046_pen_gpio_dev = device_get_binding(XPT2046_PEN_GPIO_CONTROLLER);
    if (!xpt2046_pen_gpio_dev) {
        printk("Cannot find %s!\n", XPT2046_PEN_GPIO_CONTROLLER);
        return;
    }
    /* Setup GPIO input */
    ret = gpio_pin_configure(xpt2046_pen_gpio_dev, XPT2046_PEN_GPIO_PIN,
                             (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
                              | GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE)
                            );
    if (ret) {
        printk("Error configuring pin %d!\n", XPT2046_PEN_GPIO_PIN);
    }

    gpio_init_callback(&gpio_cb, xpt2046_pen_gpio_callback,
                       BIT(XPT2046_PEN_GPIO_PIN));

    ret = gpio_add_callback(xpt2046_pen_gpio_dev, &gpio_cb);
    if (ret) {
        printk("gpio_add_callback error\n");
    }
    ret = gpio_pin_enable_callback(xpt2046_pen_gpio_dev, XPT2046_PEN_GPIO_PIN);
    if (ret) {
        printk("gpio_pin_enable_callback error\n");
    }
#endif

    k_sem_init(&sem_touch_read, 0, 1);

    k_thread_create(&touch_thread_data, touch_read_thread_stack,
                    TOUCH_READ_THREAD_STACK_SIZE, touch_screen_read_thread,
                    NULL, NULL, NULL, 5,
                    0, K_NO_WAIT);
    printf("xpt2046_init ok \n");
}

/**
 * Get the current position and state of the touchpad
 * @param data store the read data here
 * @return false: because no ore data to be read
 */
bool xpt2046_read(lv_indev_data_t * data)
{
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    bool valid = true;
    int s32_ret = 0;
    uint8_t buf;

    int16_t x = 0;
    int16_t y = 0;

    char tx1[16] = { 0 };
    char rx1[16] = { 0 };

    struct spi_buf tx_buf = { .buf = &tx1, .len = 3 };
    struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };
    struct spi_buf rx_buf = { .buf = &rx1, .len = 3 };
    struct spi_buf_set rx_bufs = { .buffers = &rx_buf, .count = 1 };

    tx1[0] = CMD_X_READ;
    s32_ret = spi_transceive(input_dev, &spi_conf_xpt2046, &tx_bufs, &rx_bufs);
    if (s32_ret != 0) {
        printf("spi_transceive return failed:%d\n", s32_ret);
    }
    x = rx1[1] << 8;
    x += rx1[2];

    tx1[0] = CMD_Y_READ;
    s32_ret = spi_transceive(input_dev, &spi_conf_xpt2046, &tx_bufs, &rx_bufs);
    if (s32_ret != 0) {
        printf("spi_transceive return failed:%d\n", s32_ret);
    }
    y = rx1[1] << 8;
    y += rx1[2];
    x = x >> 3;
    y = y >> 3;

    xpt2046_corr(&x, &y);
    if (y <= 0 || (x > 320)) {
        valid = false;
    }

    last_x = x;
    last_y = y;

    data->point.x = x;
    data->point.y = y;
    data->state = valid == false ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;

    return valid;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void xpt2046_corr(int16_t * x, int16_t * y)
{
#if XPT2046_XY_SWAP != 0
    int16_t swap_tmp;
    swap_tmp = *x;
    *x = *y;
    *y = swap_tmp;
#endif

    if ((*x) > XPT2046_X_MIN)
        (*x) -= XPT2046_X_MIN;
    else
        (*x) = 0;

    if ((*y) > XPT2046_Y_MIN)
        (*y) -= XPT2046_Y_MIN;
    else
        (*y) = 0;

    (*x) = (uint32_t)((uint32_t)(*x) * XPT2046_HOR_RES)
            / (XPT2046_X_MAX - XPT2046_X_MIN);

    (*y) = (uint32_t)((uint32_t)(*y) * XPT2046_VER_RES)
            / (XPT2046_Y_MAX - XPT2046_Y_MIN);

#if XPT2046_X_INV != 0
    (*x) = XPT2046_HOR_RES - (*x);
#endif

#if XPT2046_Y_INV != 0
    (*y) = XPT2046_VER_RES - (*y);
#endif

}

static void xpt2046_avg(int16_t * x, int16_t * y)
{
    /*Shift out the oldest data*/
    uint8_t i;
    for (i = XPT2046_AVG - 1; i > 0; i--) {
        avg_buf_x[i] = avg_buf_x[i - 1];
        avg_buf_y[i] = avg_buf_y[i - 1];
    }

    /*Insert the new point*/
    avg_buf_x[0] = *x;
    avg_buf_y[0] = *y;
    if (avg_last < XPT2046_AVG)
        avg_last++;

    /*Sum the x and y coordinates*/
    int32_t x_sum = 0;
    int32_t y_sum = 0;
    for (i = 0; i < avg_last; i++) {
        x_sum += avg_buf_x[i];
        y_sum += avg_buf_y[i];
    }

    /*Normalize the sums*/
    (*x) = (int32_t) x_sum / avg_last;
    (*y) = (int32_t) y_sum / avg_last;
}

bool touchscreen_read(lv_indev_data_t * data)
{
    /*Store the collected data*/
    data->point.x = last_touch_point.point.x;
    data->point.y = last_touch_point.point.y;
    data->state = last_touch_point.state;

    if (last_touch_point.state == LV_INDEV_STATE_PR) {
        last_touch_point.state = LV_INDEV_STATE_REL;
    }
    return false;
}

#endif
