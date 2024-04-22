//#include "../../../../soc/riscv/riscv-privilege/telink_w91/ipc/ipc_based_driver.h"
#include <ipc/ipc_based_driver.h>
#include <stdbool.h>

#define GPIO_BASE_ADDR  (0xf0700000)
#define IOMUX_BASE_ADDR	(GPIO_BASE_ADDR + 0x0020)

/* 1000 msec = 1 sec */
#define SLEEP_TIME_100_MS     100u
#define SLEEP_TIME_1_SEC      1000u
#define SLEEP_TIME_2_SEC      2000u
#define SLEEP_TIME_5_SEC      5000u
#define SLEEP_TIME_10_SEC     10000u

#define SEND_DATA_HEADER_SIZE (uint16_t)8
#define SEND_DATA_16B         (uint16_t)(16 - SEND_DATA_HEADER_SIZE)
#define SEND_DATA_32B         (uint16_t)(32 - SEND_DATA_HEADER_SIZE)
#define SEND_DATA_64B         (uint16_t)(64 - SEND_DATA_HEADER_SIZE)
#define SEND_DATA_128B        (uint16_t)(128 - SEND_DATA_HEADER_SIZE)
#define SEND_DATA_256B        (uint16_t)(256 - SEND_DATA_HEADER_SIZE)
#define SEND_DATA_512B        (uint16_t)(512 - SEND_DATA_HEADER_SIZE)
#define SEND_DATA_1KB         (uint16_t)(1024 - SEND_DATA_HEADER_SIZE)

#define SEND_DATA_SIZE_MAX    SEND_DATA_1KB
#define RECEIVE_BUFF_SIZE     (uint16_t)(SEND_DATA_SIZE_MAX + SEND_DATA_HEADER_SIZE)
#define IPC_DEV_INSTANCE      (uint8_t)0

#define RED_LED_PIN           (uint8_t)20
#define GREEN_LED_PIN         (uint8_t)19
#define PIN_STATE_ON          true
#define PIN_STATE_OFF         false

enum {
	IPC_DISPATCHER_TRNG_GET_TEST = IPC_DISPATCHER_ENTROPY_TRNG,
};

typedef enum
{
    SEND_DATA_LEN_16B   = 0,
    SEND_DATA_LEN_32B   = 1,
    SEND_DATA_LEN_64B   = 2,
    SEND_DATA_LEN_128B  = 3,
    SEND_DATA_LEN_256B  = 4,
    SEND_DATA_LEN_512B  = 5,
    SEND_DATA_LEN_1K    = 6,

    SEND_DATA_LEN_TOT   = 7,
} SEND_DATA_LEN_t;

#ifndef readl
#define readl(a) \
	({uint32_t __v = *((volatile uint32_t *)(a)); __v; })
#endif /* readl */

#ifndef writel
#define writel(v, a) \
	({uint32_t __v = v; *((volatile uint32_t *)(a)) = __v; })
#endif /* writel */

typedef struct
{
    uint16_t error;
    uint16_t data_len;
    uint8_t* data_arr;
} ipc_data_tx_t;

void gpio_enable_output(uint8_t gpio);

void gpio_set_output(uint8_t gpio, bool state);

