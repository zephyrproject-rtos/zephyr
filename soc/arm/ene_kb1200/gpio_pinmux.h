
#ifndef GPIO_PINMUX_H
#define GPIO_PINMUX_H

#ifdef __cplusplus
extern "C" {
#endif

const struct device *kb1200_get_gpio_dev(int port);

typedef struct {
	const uint16_t port;
	const uint16_t pin;
} PINMUX_DEV_T;

static inline PINMUX_DEV_T gpio_pinmux(uint16_t gpio)
{
	PINMUX_DEV_T pinmux_dev =
	{
		(gpio & 0xE0) >> 5,
		gpio & 0x1F
	};
	return pinmux_dev;
}

int gpio_pinmux_set(uint32_t port, uint32_t pin, uint32_t func);
int gpio_pinmux_get(uint32_t port, uint32_t pin, uint32_t *func);
int gpio_pinmux_pullup(uint32_t port, uint32_t pin, uint8_t func);
int gpio_pinmux_input(uint32_t port, uint32_t pin, uint8_t func);

#define PINMUX_FUNC_A		0
#define PINMUX_FUNC_B		1
#define PINMUX_FUNC_C		2
#define PINMUX_FUNC_D		3
#define PINMUX_FUNC_E		4
#define PINMUX_FUNC_F		5
#define PINMUX_FUNC_G		6
#define PINMUX_FUNC_H		7
#define PINMUX_FUNC_I		8
#define PINMUX_FUNC_J		9
#define PINMUX_FUNC_K		10
#define PINMUX_FUNC_L		11
#define PINMUX_FUNC_M		12
#define PINMUX_FUNC_N		13
#define PINMUX_FUNC_O		14
#define PINMUX_FUNC_P		15

#define PINMUX_FUNC_GPIO   PINMUX_FUNC_A

#define PINMUX_PULLUP_ENABLE	(0x1)
#define PINMUX_PULLUP_DISABLE	(0x0)

#define PINMUX_INPUT_ENABLED	(0x1)
#define PINMUX_OUTPUT_ENABLED	(0x0)


#ifdef __cplusplus
}
#endif

#endif  /* GPIO_PINMUX_H */
