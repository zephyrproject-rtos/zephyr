/*
 * Copyright (c) 2025, Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <em_device.h>
#include <em_dbg.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>

#define DT_DRV_COMPAT silabs_vcom_uart

#define SILABS_VCOM DT_DRV_INST(0)

struct vcom_config {
  uint8_t type;
  uint8_t reserved1;
  uint16_t reserved2;
  uint32_t value;
} __attribute__((__packed__));

struct cos_config_header {
  char start;
  uint8_t length;
  uint8_t edm;
  uint16_t type;
  uint8_t seq;
} __attribute__((__packed__));

struct cos_config_footer {
  uint16_t crc;
  char end;
} __attribute__((__packed__));

#define VCOM_CONFIG_HEADER(config)                                                     \
  (struct cos_config_header)                                                           \
  {                                                                                    \
    .start = '[', .length = sizeof((config)) + 4, .edm = 0xD1, .type = 0x80, .seq = 0, \
  }

#define VCOM_CONFIG_FOOTER(config) (struct cos_config_footer){.crc = 0x5A5A, .end = ']' }

#define VCOM_CONFIG_ITM_CHANNEL 8

#define VCOM_CONFIG_TYPE_UART 1

#define VCOM_CONFIG_UART_BAUDRATE_MASK  0x00FFFFFFUL
#define VCOM_CONFIG_UART_STOP_BITS_MASK 0x03000000UL
#define VCOM_CONFIG_UART_PARITY_MASK    0x0C000000UL
#define VCOM_CONFIG_UART_FLOW_CTRL_MASK 0xC0000000UL

enum vcom_uart_config_stop_bits {
  VCOM_UART_CFG_STOP_BITS_1,   /**< 1 stop bit */
  VCOM_UART_CFG_STOP_BITS_1_5, /**< 1.5 stop bit */
  VCOM_UART_CFG_STOP_BITS_2    /**< 2 stop bits */
};

enum vcom_uart_config_parity {
  VCOM_UART_CFG_PARITY_NONE, /**< No parity */
  VCOM_UART_CFG_PARITY_EVEN, /**< Even parity */
  VCOM_UART_CFG_PARITY_ODD   /**< Odd parity */
};

enum vcom_uart_config_flow_control {
  VCOM_UART_CFG_FLOW_CTRL_NONE,       /**< No flow control */
  VCOM_UART_CFG_FLOW_CTRL_RTS_CTS = 2 /**< RTS/CTS flow control */
};

static inline enum vcom_uart_config_stop_bits
vcom_uart_silabs_cfg2ll_stop_bits(enum uart_config_stop_bits stop_bits)
{
  switch (stop_bits) {
    case UART_CFG_STOP_BITS_2:
      return VCOM_UART_CFG_STOP_BITS_2;
    case UART_CFG_STOP_BITS_1_5:
      return VCOM_UART_CFG_STOP_BITS_1_5;
    default:
      return VCOM_UART_CFG_STOP_BITS_1;
  }
}

static inline enum vcom_uart_config_parity
vcom_uart_silabs_cfg2ll_parity(enum uart_config_parity parity)
{
  switch (parity) {
    case UART_CFG_PARITY_ODD:
      return VCOM_UART_CFG_PARITY_ODD;
    case UART_CFG_PARITY_EVEN:
      return VCOM_UART_CFG_PARITY_EVEN;
    default:
      return VCOM_UART_CFG_PARITY_NONE;
  }
}

static inline enum vcom_uart_config_flow_control
vcom_uart_silabs_cfg2ll_flow_ctrl(enum uart_config_flow_control flow_ctrl)
{
  switch (flow_ctrl) {
    case UART_CFG_FLOW_CTRL_RTS_CTS:
      return VCOM_UART_CFG_FLOW_CTRL_RTS_CTS;
    default:
      return VCOM_UART_CFG_FLOW_CTRL_NONE;
  }
}

static inline void vcom_uart_silabs_itm_write_u8(uint8_t ch)
{
  while (ITM->PORT[VCOM_CONFIG_ITM_CHANNEL].u32 == 0UL) {
    __NOP();
  }
  ITM->PORT[VCOM_CONFIG_ITM_CHANNEL].u8 = ch;
}

static void uart_silabs_vcom_write_config(const struct vcom_config *vcom_config)
{
  const struct cos_config_header vcom_hdr = VCOM_CONFIG_HEADER(*vcom_config);
  const struct cos_config_footer vcom_ftr = VCOM_CONFIG_FOOTER(*vcom_config);

  /* Corruption sometimes occurs on first byte, flush to avoid this */
  vcom_uart_silabs_itm_write_u8(0xFF);

  /* Write header */
  for (size_t i = 0; i < sizeof(vcom_hdr); i++) {
    vcom_uart_silabs_itm_write_u8(((uint8_t *)&vcom_hdr)[i]);
  }

  /* Write config */
  for (size_t i = 0; i < sizeof(*vcom_config); i++) {
    vcom_uart_silabs_itm_write_u8(((uint8_t *)vcom_config)[i]);
  }

  /* Write footer */
  for (size_t i = 0; i < sizeof(vcom_ftr); i++) {
    vcom_uart_silabs_itm_write_u8(((uint8_t *)&vcom_ftr)[i]);
  }
}

static int uart_silabs_vcom_init(const struct device *dev)
{
  static const struct gpio_dt_spec enable = GPIO_DT_SPEC_GET(SILABS_VCOM, en_gpios);
  const struct uart_config *uart_cfg = dev->data;
  struct vcom_config vcom_cfg = { 0 };
  uint32_t stop_bits;
  uint32_t flow_ctrl;
  uint32_t parity;
  int ret;

  /* Enable SWO */
  DBG_SWOEnable(0);

  /* Enable ITM VCOM config port */
  ITM->TER |= 1 << VCOM_CONFIG_ITM_CHANNEL;

  /* Enable VCOM */
  ret = gpio_pin_configure_dt(&enable, GPIO_OUTPUT_ACTIVE);
  if (ret) {
    return ret;
  }

  /* Build VCOM configuration */
  stop_bits = vcom_uart_silabs_cfg2ll_stop_bits(uart_cfg->stop_bits);
  flow_ctrl = vcom_uart_silabs_cfg2ll_flow_ctrl(uart_cfg->flow_ctrl);
  parity = vcom_uart_silabs_cfg2ll_parity(uart_cfg->parity);

  vcom_cfg.type = VCOM_CONFIG_TYPE_UART;
  vcom_cfg.value = FIELD_PREP(VCOM_CONFIG_UART_BAUDRATE_MASK, uart_cfg->baudrate)
                   | FIELD_PREP(VCOM_CONFIG_UART_STOP_BITS_MASK, uart_cfg->stop_bits)
                   | FIELD_PREP(VCOM_CONFIG_UART_FLOW_CTRL_MASK, uart_cfg->flow_ctrl)
                   | FIELD_PREP(VCOM_CONFIG_UART_PARITY_MASK, uart_cfg->parity);

  /* Send configuration via ITM */
  uart_silabs_vcom_write_config(&vcom_cfg);

  return 0;
}

/* Grab UART configuration from parent */
#define SILABS_VCOM_UART_CONTROLLER DT_PROP(DT_DRV_INST(0), controller)

static struct uart_config uart_parent_cfg = {
  .baudrate = DT_PROP(SILABS_VCOM_UART_CONTROLLER, current_speed),
  .parity = DT_ENUM_IDX(SILABS_VCOM_UART_CONTROLLER, parity),
  .stop_bits = DT_ENUM_IDX(SILABS_VCOM_UART_CONTROLLER, stop_bits),
  .flow_ctrl = DT_PROP(SILABS_VCOM_UART_CONTROLLER, hw_flow_control)
               ? UART_CFG_FLOW_CTRL_RTS_CTS
               : UART_CFG_FLOW_CTRL_NONE,
};

DEVICE_DT_INST_DEFINE(0, uart_silabs_vcom_init, NULL, &uart_parent_cfg, NULL, POST_KERNEL,
                      CONFIG_SERIAL_INIT_PRIORITY, NULL);
