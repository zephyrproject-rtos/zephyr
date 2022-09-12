/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_i2c

#include <soc.h>
#include <zephyr/drivers/i2c.h>
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#endif
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_numaker, CONFIG_I2C_LOG_LEVEL);

#include "NuMicro.h"
#include "i2c-priv.h"

/* Implementation notes
 *
 * 1. Support dual role
 * 2. Support at most one slave at one time, though, H/W can support multiple.
 * 3. The following APIs will fail with slave being busy:
 *    (1) i2c_numaker_configure()
 *    (2) i2c_numaker_transfer()
 *    (3) i2c_numaker_slave_unregister()
 */

/* Transfer control type
 *
 * Active transfer (master/slave):
 * 1. Prepare buffer (thread context)
 * 2. Initiate transfer (thread context)
 * 3. Do data transfer via above buffer (interrupt context)
 *
 * Passive transfer (slave only):
 * 1. Prepare callback (thread context)
 * 2. Do data transfer via above callback (interrupt context)
 *
 * Mbed HAL requires master/slave in active transfer.
 * Zephyr HAL requires master in active transfer, slave in passive transfer.
 */

static void nu_int_i2c_fsm_reset(const struct device *dev, uint32_t i2c_ctl);
static void nu_int_i2c_fsm_tranfini(const struct device *dev, int lastdatanaked);

static int nu_int_i2c_do_tran(const struct device *dev, char *buf, int length, int read, int naklastdata);
static int nu_int_i2c_do_trsn(const struct device *dev, uint32_t i2c_ctl, int sync);
#define NU_I2C_TIMEOUT_STAT_INT     500000  // us
#define NU_I2C_TIMEOUT_STOP         500000  // us
static int nu_int_i2c_poll_status_timeout(const struct device *dev, int (*is_status)(const struct device *dev), uint32_t timeout);
static int nu_int_i2c_poll_tran_heatbeat_timeout(const struct device *dev, uint32_t timeout);
static int nu_int_i2c_is_trsn_done(const struct device *dev);
static int nu_int_i2c_is_tran_started(const struct device *dev);
static int nu_int_i2c_addr2data(int address, int read);
#ifdef CONFIG_I2C_SLAVE
/* Convert zephyr address to BSP address. */
static int nu_int_i2c_addr2bspaddr(int address);
static void nu_int_i2c_enable_slave_if_registered(const struct device *dev);
static int nu_int_i2c_is_slave_registered(const struct device *dev);
static int nu_int_i2c_is_slave_busy(const struct device *dev);
#endif
static void nu_int_i2c_enable_int(const struct device *dev);
static void nu_int_i2c_disable_int(const struct device *dev);
static int nu_int_i2c_set_int(const struct device *dev, int inten);

/* Control flags for active transfer */
#define TRANCTRL_STARTED        (1)         // Guard I2C ISR from data transfer prematurely
#define TRANCTRL_NAKLASTDATA    (1 << 1)    // Request NACK on last data
#define TRANCTRL_LASTDATANAKED  (1 << 2)    // Last data NACKed
#define TRANCTRL_RECVDATA       (1 << 3)    // Receive data available

#ifdef CONFIG_I2C_SLAVE
#define NoData         0    // the slave has not been addressed
#define ReadAddressed  1    // the master has requested a read from this slave (slave = transmitter)
#define WriteGeneral   2    // the master is writing to all slave
#define WriteAddressed 3    // the master is writing to this slave (slave = receiver)
#endif

static int nu_int_i2c_start(const struct device *dev);
static int nu_int_i2c_stop(const struct device *dev);
static int nu_int_i2c_reset(const struct device *dev);
static int nu_int_i2c_byte_read(const struct device *dev, int last);
static int nu_int_i2c_byte_write(const struct device *dev, int data);

struct i2c_numaker_config {
    I2C_T *     i2c_base;
    uint32_t    id_rst;
    uint32_t    clk_modidx;
    uint32_t    clk_src;
    uint32_t    clk_div;
#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
    const struct device *               clkctrl_dev;
#endif
    uint32_t    irq_n;
    void (*irq_config_func)(const struct device *dev);
#ifdef CONFIG_PINCTRL
    const struct pinctrl_dev_config *   pincfg;
#endif  
    uint32_t bitrate;
};

struct i2c_numaker_data {
    struct          k_sem lock;
    struct          k_sem xfer_sync;
    uint32_t        dev_config;
#ifdef CONFIG_I2C_SLAVE
    struct i2c_slave_config *   slave_config;
#endif

    struct {
        uint32_t    tran_ctrl;
        char *      tran_beg;
        char *      tran_pos;
        char *      tran_end;
        int         inten;
#ifdef CONFIG_I2C_SLAVE
        int         slaveaddr_state;
#endif
    } i2c;    
};

static int i2c_numaker_configure(const struct device *dev,
                uint32_t dev_config)
{
    const struct i2c_numaker_config *config = dev->config;
    struct i2c_numaker_data *data = dev->data;

    /* Check address size */
    if (dev_config & I2C_ADDR_10_BITS) {
        LOG_ERR("10-bits address not supported");
        return -ENOTSUP;
    }

    /* Ignore I2C_MODE_MASTER flag here because master/slave mode switch is done elsewhere. */

    uint32_t bitrate;
    switch (I2C_SPEED_GET(dev_config)) {
    case I2C_SPEED_STANDARD:
        bitrate = KHZ(100);
        break;

    case I2C_SPEED_FAST:
        bitrate = KHZ(400);
        break;

    case I2C_SPEED_FAST_PLUS:
        bitrate = MHZ(1);
        break;

    default:
        LOG_ERR("Speed code %d not supported", I2C_SPEED_GET(dev_config));
        return -ENOTSUP;
    }

    I2C_T *i2c_base = config->i2c_base;
    int err = 0;

    k_sem_take(&data->lock, K_FOREVER);
    nu_int_i2c_disable_int(dev);

#ifdef CONFIG_I2C_SLAVE
    if (nu_int_i2c_is_slave_busy(dev)) {
        LOG_ERR("Reconfigure with slave being busy");
        err = -EBUSY;
        goto cleanup;
    }
#endif

    I2C_Close(i2c_base);
    I2C_Open(i2c_base, bitrate);
    /* INTEN bit and FSM control bits (STA, STO, SI, AA) are packed in one register CTL0. We cannot control interrupt through
       INTEN bit without impacting FSM control bits. Use NVIC_EnableIRQ/NVIC_DisableIRQ instead for interrupt control. */
    i2c_base->CTL0 |= (I2C_CTL0_INTEN_Msk | I2C_CTL0_I2CEN_Msk);

    data->dev_config = dev_config;

cleanup:

    nu_int_i2c_enable_int(dev);
    k_sem_give(&data->lock);

    return err;
}

static int i2c_numaker_get_config(const struct device *dev,
                 uint32_t *dev_config)
{
    struct i2c_numaker_data *data = dev->data;

    if (!dev_config) {
        return -EINVAL;
    }

    k_sem_take(&data->lock, K_FOREVER);

    *dev_config = data->dev_config;

    k_sem_give(&data->lock);

    return 0;
}

static int i2c_numaker_transfer(const struct device *dev, struct i2c_msg *msgs,
                 uint8_t num_msgs, uint16_t addr)
{
    struct i2c_numaker_data *data = dev->data;
    int err = 0;
    struct i2c_msg *msg_iter = msgs;
    struct i2c_msg *msg_end = msgs + num_msgs;
    /* Do I2C start to acquire bus ownership and for first message */
    int new_start = 1;

    k_sem_take(&data->lock, K_FOREVER);
    nu_int_i2c_disable_int(dev);

    if (nu_int_i2c_is_slave_busy(dev)) {
        LOG_ERR("Master transfer with slave being busy");
        err = -EBUSY;
        goto cleanup;
    }

    /* Iterate over all the messages */
    for (; msg_iter != msg_end; msg_iter ++) {
        /* Message flags */
        int is_addr_10_bits = (msg_iter->flags & I2C_MSG_ADDR_10_BITS) ? 1 : 0;
        int is_read = ((msg_iter->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) ? 1 : 0;
        int do_restart = (msg_iter->flags & I2C_MSG_RESTART) ? 1 : 0;
        int do_stop = (msg_iter->flags & I2C_MSG_STOP) ? 1 : 0;

        /* Not support 10-bit address */
        if (is_addr_10_bits) {
            LOG_ERR("10-bits address not supported");
            err = -ENOTSUP;
            break;
        }

        /* New start on request */
        if (!new_start) {
            new_start = do_restart;
        }

        /* On new start, resend start condition and address */
        if (new_start) {
            if (nu_int_i2c_start(dev)) {
                nu_int_i2c_stop(dev);
                err = -EIO;
                break;
            }

            if (nu_int_i2c_byte_write(dev, nu_int_i2c_addr2data(addr, is_read)) != 1) {
                nu_int_i2c_stop(dev);
                err = -EIO;
                break;
            }

            /* Flag next message is not new start */
            new_start = 0;
        }

        /* Read in/write out bytes */
        int length = nu_int_i2c_do_tran(dev, msg_iter->buf, msg_iter->len, is_read, 1);
        if (length != msg_iter->len) {
            LOG_ERR("Expected %d bytes transferred, but actual %d", msg_iter->len, length);
            err = -EIO;
            break;
        }

        /* Do I2C stop on request */
        if (do_stop) {
            if (nu_int_i2c_stop(dev)) {
                err = -EIO;
                break;
            }
            /* Flag next message is new start */
            new_start = 1;
        }
    }

    /* Do I2C stop to release bus ownership */
    nu_int_i2c_stop(dev);

#ifdef CONFIG_I2C_SLAVE
    /* Enable slave mode if any slave registered */
    nu_int_i2c_enable_slave_if_registered(dev);
#endif

cleanup:

    nu_int_i2c_enable_int(dev);
    k_sem_give(&data->lock);

    return err;
}

#ifdef CONFIG_I2C_SLAVE
static int i2c_numaker_slave_register(const struct device *dev,
                       struct i2c_slave_config *slave_config)
{
    if (!slave_config) {
        return -EINVAL;
    }

    if (slave_config->flags & I2C_SLAVE_FLAGS_ADDR_10_BITS) {
        LOG_ERR("10-bits address not supported");
        return -ENOTSUP;
    }

    const struct i2c_numaker_config *config = dev->config;
    struct i2c_numaker_data *data = dev->data;
    I2C_T *i2c_base = config->i2c_base;
    int err = 0;

    k_sem_take(&data->lock, K_FOREVER);
    nu_int_i2c_disable_int(dev);

    if (data->slave_config) {
        err = -EBUSY;
        goto cleanup;
    }
    data->slave_config = slave_config;

    /* Slave address */
    I2C_SetSlaveAddr(i2c_base,
                     0,
                     nu_int_i2c_addr2bspaddr(slave_config->address),
                     I2C_GCMODE_DISABLE);

    /* Slave address state */
    data->i2c.slaveaddr_state = NoData;

    /* Enable slave mode due to slave registered */
    nu_int_i2c_enable_slave_if_registered(dev);

cleanup:

    nu_int_i2c_enable_int(dev);
    k_sem_give(&data->lock);

    return err;
}

static int i2c_numaker_slave_unregister(const struct device *dev,
                     struct i2c_slave_config *slave_config)
{
    if (!slave_config) {
        return -EINVAL;
    }

    const struct i2c_numaker_config *config = dev->config;
    struct i2c_numaker_data *data = dev->data;
    I2C_T *i2c_base = config->i2c_base;
    int err = 0;

    k_sem_take(&data->lock, K_FOREVER);
    nu_int_i2c_disable_int(dev);

    if (data->slave_config != slave_config) {
        err = -EINVAL;
        goto cleanup;
    }

    if (nu_int_i2c_is_slave_busy(dev)) {
        LOG_ERR("Unregister slave driver with slave being busy");
        err = -EBUSY;
        goto cleanup;
    }

    /* Slave address: Zero */
    I2C_SetSlaveAddr(i2c_base,
                     0,
                     nu_int_i2c_addr2bspaddr(0),
                     I2C_GCMODE_DISABLE);

    /* Slave address state */
    data->i2c.slaveaddr_state = NoData;

    /* Disable slave mode due to no slave registered */
    nu_int_i2c_enable_slave_if_registered(dev);

    data->slave_config = NULL;

cleanup:

    nu_int_i2c_enable_int(dev);
    k_sem_give(&data->lock);

    return err;
}
#endif

static int i2c_numaker_recover_bus(const struct device *dev)
{
    struct i2c_numaker_data *data = dev->data;

    k_sem_take(&data->lock, K_FOREVER);
    nu_int_i2c_reset(dev);
    k_sem_give(&data->lock);

    return 0;
}

static void i2c_numaker_isr(const struct device *dev)
{
    const struct i2c_numaker_config *config = dev->config;
    struct i2c_numaker_data *data = dev->data;
    I2C_T *i2c_base = config->i2c_base;
#ifdef CONFIG_I2C_SLAVE
    struct i2c_slave_config *slave_config = data->slave_config;
    const struct i2c_slave_callbacks *slave_callbacks = slave_config ? slave_config->callbacks : NULL;
    int err = 0;
    uint8_t data_tran;
#endif
    uint32_t status;
    /* Active transfer started? */
    bool act_tran_started = (data->i2c.tran_ctrl & TRANCTRL_STARTED);

    if (I2C_GET_TIMEOUT_FLAG(i2c_base)) {
        I2C_ClearTimeoutFlag(i2c_base);
        return;
    }

    status = I2C_GET_STATUS(i2c_base);

    switch (status) {
    // Master Transmit
    case 0x28:  // Master Transmit Data ACK
    case 0x18:  // Master Transmit Address ACK
    case 0x08:  // Start
    case 0x10:  // Master Repeat Start
        if ((data->i2c.tran_ctrl & TRANCTRL_STARTED) && data->i2c.tran_pos) {
            if (data->i2c.tran_pos < data->i2c.tran_end) {
                I2C_SET_DATA(i2c_base, *data->i2c.tran_pos ++);
                I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
            } else {
                nu_int_i2c_fsm_tranfini(dev, 0);
            }
        } else {
            nu_int_i2c_disable_int(dev);
        }
        break;

    case 0x30:  // Master Transmit Data NACK
        nu_int_i2c_fsm_tranfini(dev, 1);
        break;

    case 0x20:  // Master Transmit Address NACK
        nu_int_i2c_fsm_tranfini(dev, 1);
        break;

    case 0x38:  // Master Arbitration Lost
        nu_int_i2c_fsm_reset(dev, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
        break;

    case 0x48:  // Master Receive Address NACK
        nu_int_i2c_fsm_tranfini(dev, 1);
        break;

    case 0x40:  // Master Receive Address ACK
    case 0x50:  // Master Receive Data ACK
    case 0x58:  // Master Receive Data NACK
        if ((data->i2c.tran_ctrl & TRANCTRL_STARTED) && data->i2c.tran_pos) {
            if (data->i2c.tran_pos < data->i2c.tran_end) {
                if (status == 0x50 || status == 0x58) {
                    if (data->i2c.tran_ctrl & TRANCTRL_RECVDATA) {
                        *data->i2c.tran_pos ++ = I2C_GET_DATA(i2c_base);
                        data->i2c.tran_ctrl &= ~TRANCTRL_RECVDATA;
                    }
                }

                if (status == 0x58) {
                    nu_int_i2c_fsm_tranfini(dev, 1);
                } else if (data->i2c.tran_pos == data->i2c.tran_end) {
                    data->i2c.tran_ctrl &= ~TRANCTRL_STARTED;
                    nu_int_i2c_disable_int(dev);
                } else {
                    uint32_t i2c_ctl = I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk;
                    if ((data->i2c.tran_end - data->i2c.tran_pos) == 1 &&
                            data->i2c.tran_ctrl & TRANCTRL_NAKLASTDATA) {
                        // Last data
                        i2c_ctl &= ~I2C_CTL0_AA_Msk;
                    }
                    I2C_SET_CONTROL_REG(i2c_base, i2c_ctl);
                    data->i2c.tran_ctrl |= TRANCTRL_RECVDATA;
                }
            } else {
                data->i2c.tran_ctrl &= ~TRANCTRL_STARTED;
                nu_int_i2c_disable_int(dev);
                break;
            }
        } else {
            nu_int_i2c_disable_int(dev);
        }
        break;

    //case 0x00:  // Bus error

#ifdef CONFIG_I2C_SLAVE
    // Slave Transmit
    case 0xB8:  // Slave Transmit Data ACK
    case 0xA8:  // Slave Transmit Address ACK
    case 0xB0:  // Slave Transmit Arbitration Lost
        if ((data->i2c.tran_ctrl & TRANCTRL_STARTED) && data->i2c.tran_pos) {
            if (data->i2c.tran_pos < data->i2c.tran_end) {
                uint32_t i2c_ctl = I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk;

                I2C_SET_DATA(i2c_base, *data->i2c.tran_pos ++);
                if (data->i2c.tran_pos == data->i2c.tran_end &&
                        data->i2c.tran_ctrl & TRANCTRL_NAKLASTDATA) {
                    // Last data
                    i2c_ctl &= ~I2C_CTL0_AA_Msk;
                }
                I2C_SET_CONTROL_REG(i2c_base, i2c_ctl);
            } else {
                data->i2c.tran_ctrl &= ~TRANCTRL_STARTED;
                nu_int_i2c_disable_int(dev);
                break;
            }
        } else if (slave_callbacks) {
            uint32_t i2c_ctl = I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk;
            switch (status) {
            case 0xA8:  // Slave Transmit Address ACK
            case 0xB0:  // Slave Transmit Arbitration Lost
                err = slave_callbacks->read_requested(slave_config, &data_tran);
                if (err < 0) {
                    i2c_ctl &= ~I2C_CTL0_AA_Msk;
                }
                I2C_SET_DATA(i2c_base, data_tran);
                break;
            case 0xB8:  // Slave Transmit Data ACK
                err = slave_callbacks->read_processed(slave_config, &data_tran);
                if (err < 0) {
                    i2c_ctl &= ~I2C_CTL0_AA_Msk;
                }
                I2C_SET_DATA(i2c_base, data_tran);
                break;
            default:
                __ASSERT_NO_MSG(false);
            }
            I2C_SET_CONTROL_REG(i2c_base, i2c_ctl);
        } else {
            nu_int_i2c_disable_int(dev);
        }
        data->i2c.slaveaddr_state = ReadAddressed;
        break;
    //case 0xA0:  // Slave Transmit Repeat Start or Stop
    case 0xC0:  // Slave Transmit Data NACK
    case 0xC8:  // Slave Transmit Last Data ACK
        data->i2c.slaveaddr_state = NoData;
        nu_int_i2c_fsm_reset(dev, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
        break;

    // Slave Receive
    case 0x80:  // Slave Receive Data ACK
    case 0x88:  // Slave Receive Data NACK
    case 0x60:  // Slave Receive Address ACK
    case 0x68:  // Slave Receive Arbitration Lost
        data->i2c.slaveaddr_state = WriteAddressed;
        if ((data->i2c.tran_ctrl & TRANCTRL_STARTED) && data->i2c.tran_pos) {
            if (data->i2c.tran_pos < data->i2c.tran_end) {
                if (status == 0x80 || status == 0x88) {
                    if (data->i2c.tran_ctrl & TRANCTRL_RECVDATA) {
                        *data->i2c.tran_pos ++ = I2C_GET_DATA(i2c_base);
                        data->i2c.tran_ctrl &= ~TRANCTRL_RECVDATA;
                    }
                }

                if (status == 0x88) {
                    data->i2c.slaveaddr_state = NoData;
                    nu_int_i2c_fsm_reset(dev, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
                } else if (data->i2c.tran_pos == data->i2c.tran_end) {
                    data->i2c.tran_ctrl &= ~TRANCTRL_STARTED;
                    nu_int_i2c_disable_int(dev);
                } else {
                    uint32_t i2c_ctl = I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk;
                    if ((data->i2c.tran_end - data->i2c.tran_pos) == 1 &&
                            data->i2c.tran_ctrl & TRANCTRL_NAKLASTDATA) {
                        // Last data
                        i2c_ctl &= ~I2C_CTL0_AA_Msk;
                    }
                    I2C_SET_CONTROL_REG(i2c_base, i2c_ctl);
                    data->i2c.tran_ctrl |= TRANCTRL_RECVDATA;
                }
            } else {
                data->i2c.tran_ctrl &= ~TRANCTRL_STARTED;
                nu_int_i2c_disable_int(dev);
                break;
            }
        } else if (slave_callbacks) {
            uint32_t i2c_ctl = I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk;
            switch (status) {
            case 0x60:  // Slave Receive Address ACK
            case 0x68:  // Slave Receive Arbitration Lost
                err = slave_callbacks->write_requested(slave_config);
                if (err < 0) {
                    i2c_ctl &= ~I2C_CTL0_AA_Msk;
                }
                break;
            case 0x80:  // Slave Receive Data ACK
                data_tran = I2C_GET_DATA(i2c_base);
                err = slave_callbacks->write_received(slave_config, data_tran);
                if (err < 0) {
                    i2c_ctl &= ~I2C_CTL0_AA_Msk;
                }
                break;
            case 0x88:  // Slave Receive Data NACK
                data_tran = I2C_GET_DATA(i2c_base);
                slave_callbacks->write_received(slave_config, data_tran);
                break;
            default:
                __ASSERT_NO_MSG(false);
            }
            I2C_SET_CONTROL_REG(i2c_base, i2c_ctl);
        } else {
            nu_int_i2c_disable_int(dev);
        }
        break;
    //case 0xA0:  // Slave Receive Repeat Start or Stop

    case 0xA0:  // Slave Transmit/Receive Repeat Start or Stop
        if (slave_callbacks) {
            slave_callbacks->stop(slave_config);
            I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
        }
        break;

    // GC mode
    //case 0xA0:  // GC mode Repeat Start or Stop
    case 0x90:  // GC mode Data ACK
    case 0x98:  // GC mode Data NACK
    case 0x70:  // GC mode Address ACK
    case 0x78:  // GC mode Arbitration Lost
        data->i2c.slaveaddr_state = WriteAddressed;
        if ((data->i2c.tran_ctrl & TRANCTRL_STARTED) && data->i2c.tran_pos) {
            if (data->i2c.tran_pos < data->i2c.tran_end) {
                if (status == 0x90 || status == 0x98) {
                    if (data->i2c.tran_ctrl & TRANCTRL_RECVDATA) {
                        *data->i2c.tran_pos ++ = I2C_GET_DATA(i2c_base);
                        data->i2c.tran_ctrl &= ~TRANCTRL_RECVDATA;
                    }
                }

                if (status == 0x98) {
                    data->i2c.slaveaddr_state = NoData;
                    nu_int_i2c_fsm_reset(dev, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
                } else if (data->i2c.tran_pos == data->i2c.tran_end) {
                    data->i2c.tran_ctrl &= ~TRANCTRL_STARTED;
                    nu_int_i2c_disable_int(dev);
                } else {
                    uint32_t i2c_ctl = I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk;
                    if ((data->i2c.tran_end - data->i2c.tran_pos) == 1 &&
                            data->i2c.tran_ctrl & TRANCTRL_NAKLASTDATA) {
                        // Last data
                        i2c_ctl &= ~I2C_CTL0_AA_Msk;
                    }
                    I2C_SET_CONTROL_REG(i2c_base, i2c_ctl);
                    data->i2c.tran_ctrl |= TRANCTRL_RECVDATA;
                }
            } else {
                data->i2c.tran_ctrl &= ~TRANCTRL_STARTED;
                nu_int_i2c_disable_int(dev);
                break;
            }
        } else {
            nu_int_i2c_disable_int(dev);
        }
        break;
#endif  /* CONFIG_I2C_SLAVE */

    case 0xF8:  // Bus Released
        break;

    default:
        nu_int_i2c_fsm_reset(dev, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
    }

    /* Signal active transfer has completed or stopped */
    if (act_tran_started && !(data->i2c.tran_ctrl & TRANCTRL_STARTED)) {
        k_sem_give(&data->xfer_sync);
    }
}

static int i2c_numaker_init(const struct device *dev)
{
    const struct i2c_numaker_config *config = dev->config;
    struct i2c_numaker_data *data = dev->data;
    int err = 0;

    /* Clean mutable context */
    memset(data, 0x00, sizeof(*data));

    k_sem_init(&data->lock, 1, 1);
    k_sem_init(&data->xfer_sync, 0, 1);

    SYS_UnlockReg();

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
    struct numaker_scc_subsys scc_subsys;

    memset(&scc_subsys, 0x00, sizeof(scc_subsys));
    scc_subsys.subsys_id        = NUMAKER_SCC_SUBSYS_ID_PCC;
    scc_subsys.pcc.clk_modidx   = config->clk_modidx;
    scc_subsys.pcc.clk_src      = config->clk_src;
    scc_subsys.pcc.clk_div      = config->clk_div;

    /* Equivalent to CLK_EnableModuleClock() */
    err = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t) &scc_subsys);
    if (err != 0) {
        goto cleanup;
    }
    /* Equivalent to CLK_SetModuleClock() */
    err = clock_control_configure(config->clkctrl_dev, (clock_control_subsys_t) &scc_subsys, NULL);
    if (err != 0) {
        goto cleanup;
    }
#else
    /* Enable module clock */
    CLK_EnableModuleClock(config->clk_modidx);

    /* Select module clock source/divider */
    CLK_SetModuleClock(config->clk_modidx, config->clk_src, config->clk_div);
#endif

    /* Configure pinmux (NuMaker's SYS MFP) */
#ifdef CONFIG_PINCTRL
    err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
    if (err != 0) {
        goto cleanup;
    }
#else
#error  "No separate pinmux function implementation. Enable CONFIG_PINCTRL instead."
#endif 

    SYS_ResetModule(config->id_rst);

    err = i2c_numaker_configure(dev, I2C_MODE_MASTER | i2c_map_dt_bitrate(config->bitrate));
    if (err != 0) {
        goto cleanup;
    }

    config->irq_config_func(dev);

cleanup:

    SYS_LockReg();
    return err;
}

static const struct i2c_driver_api i2c_numaker_driver_api = {
    .configure          = i2c_numaker_configure,
    .get_config         = i2c_numaker_get_config,
    .transfer           = i2c_numaker_transfer,
#ifdef CONFIG_I2C_SLAVE
    .slave_register     = i2c_numaker_slave_register,
    .slave_unregister   = i2c_numaker_slave_unregister,
#endif
    .recover_bus        = i2c_numaker_recover_bus,
};

#ifdef CONFIG_CLOCK_CONTROL_NUMAKER_SCC
#define NUMAKER_CLKCTRL_DEV_INIT(inst)          \
    .clkctrl_dev    = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),
#else
#define NUMAKER_CLKCTRL_DEV_INIT(inst)
#endif

#ifdef CONFIG_PINCTRL
#define NUMAKER_PINCTRL_DEFINE(inst)            \
    PINCTRL_DT_INST_DEFINE(inst);
#define NUMAKER_PINCTRL_INIT(inst)              \
    .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),
#else
#define NUMAKER_PINCTRL_DEFINE(inst)
#define NUMAKER_PINCTRL_INIT(inst)
#endif

#define I2C_NUMAKER_INIT(inst)                  \
    NUMAKER_PINCTRL_DEFINE(inst);               \
                                                \
    static void i2c_numaker_irq_config_func_ ## inst(const struct device *dev); \
                                                \
    static const struct i2c_numaker_config i2c_numaker_config_ ## inst = {      \
        .i2c_base   = (I2C_T *) DT_INST_REG_ADDR(inst),                 \
        .id_rst     = DT_INST_PROP(inst, reset),                        \
        .clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),    \
        .clk_src    = DT_INST_CLOCKS_CELL(inst, clock_source),          \
        .clk_div    = DT_INST_CLOCKS_CELL(inst, clock_divider),         \
        NUMAKER_CLKCTRL_DEV_INIT(inst)                                  \
        .irq_n      = DT_INST_IRQN(inst),                               \
        .irq_config_func    = i2c_numaker_irq_config_func_ ## inst,     \
        NUMAKER_PINCTRL_INIT(inst)                                      \
        .bitrate    = DT_INST_PROP(inst, clock_frequency),              \
    };                                          \
                                                \
    static struct i2c_numaker_data i2c_numaker_data_ ## inst;           \
                                                \
    I2C_DEVICE_DT_INST_DEFINE(inst,             \
            i2c_numaker_init,                   \
            NULL,                               \
            &i2c_numaker_data_ ## inst,         \
            &i2c_numaker_config_ ## inst,       \
            POST_KERNEL,                        \
            CONFIG_I2C_INIT_PRIORITY,           \
            &i2c_numaker_driver_api);           \
                                                \
    static void i2c_numaker_irq_config_func_ ## inst(const struct device *dev)  \
    {                                           \
        IRQ_CONNECT(DT_INST_IRQN(inst),         \
                    DT_INST_IRQ(inst, priority),\
                    i2c_numaker_isr,            \
                    DEVICE_DT_INST_GET(inst),   \
                    0);                         \
                                                \
        irq_enable(DT_INST_IRQN(inst));         \
    }

DT_INST_FOREACH_STATUS_OKAY(I2C_NUMAKER_INIT);

static void nu_int_i2c_fsm_reset(const struct device *dev, uint32_t i2c_ctl)
{
    const struct i2c_numaker_config *config = dev->config;
    struct i2c_numaker_data *data = dev->data;
    I2C_T *i2c_base = config->i2c_base;

    data->i2c.tran_ctrl = 0;

    I2C_SET_CONTROL_REG(i2c_base, i2c_ctl);
#ifdef CONFIG_I2C_SLAVE
    data->i2c.slaveaddr_state = NoData;
#endif
}

static void nu_int_i2c_fsm_tranfini(const struct device *dev, int lastdatanaked)
{
    struct i2c_numaker_data *data = dev->data;

    if (lastdatanaked) {
        data->i2c.tran_ctrl |= TRANCTRL_LASTDATANAKED;
    }

    data->i2c.tran_ctrl &= ~TRANCTRL_STARTED;
    nu_int_i2c_disable_int(dev);
}

static int nu_int_i2c_do_tran(const struct device *dev, char *buf, int length, int read, int naklastdata)
{
    if (! buf || ! length) {
        return 0;
    }

    struct i2c_numaker_data *data = dev->data;
    int tran_len = 0;

    nu_int_i2c_disable_int(dev);
    data->i2c.tran_ctrl = naklastdata ? (TRANCTRL_STARTED | TRANCTRL_NAKLASTDATA) : TRANCTRL_STARTED;
    data->i2c.tran_beg = buf;
    data->i2c.tran_pos = buf;
    data->i2c.tran_end = buf + length;

    /* Dummy take on xfer_sync sem to be sure that it is empty */
    k_sem_take(&data->xfer_sync, K_NO_WAIT);
    nu_int_i2c_enable_int(dev);

    if (nu_int_i2c_poll_tran_heatbeat_timeout(dev, NU_I2C_TIMEOUT_STAT_INT)) {
        // N/A
    } else {
        nu_int_i2c_disable_int(dev);
        tran_len = data->i2c.tran_pos - data->i2c.tran_beg;
        data->i2c.tran_beg = NULL;
        data->i2c.tran_pos = NULL;
        data->i2c.tran_end = NULL;
        nu_int_i2c_enable_int(dev);
    }

    return tran_len;
}

static int nu_int_i2c_do_trsn(const struct device *dev, uint32_t i2c_ctl, int sync)
{
    const struct i2c_numaker_config *config = dev->config;
    I2C_T *i2c_base = config->i2c_base;
    int err = 0;

    nu_int_i2c_disable_int(dev);

    if (nu_int_i2c_poll_status_timeout(dev, nu_int_i2c_is_trsn_done, NU_I2C_TIMEOUT_STAT_INT)) {
        err = -EIO;
    } else {
        // NOTE: Avoid duplicate Start/Stop. Otherwise, we may meet strange error.
        uint32_t status = I2C_GET_STATUS(i2c_base);

        switch (status) {
        case 0x08:  // Start
        case 0x10:  // Master Repeat Start
            if (i2c_ctl & I2C_CTL0_STA_Msk) {
                goto cleanup;
            } else {
                break;
            }
        case 0xF8:  // Bus Released
            if ((i2c_ctl & (I2C_CTL0_STA_Msk | I2C_CTL0_STO_Msk)) == I2C_CTL0_STO_Msk) {
                goto cleanup;
            } else {
                break;
            }
        }
        I2C_SET_CONTROL_REG(i2c_base, i2c_ctl);
        if (sync && nu_int_i2c_poll_status_timeout(dev, nu_int_i2c_is_trsn_done, NU_I2C_TIMEOUT_STAT_INT)) {
            err = -EIO;
        }
    }

cleanup:

    nu_int_i2c_enable_int(dev);

    return err;
}

static int nu_int_i2c_poll_status_timeout(const struct device *dev, int (*is_status)(const struct device *dev), uint32_t timeout)
{
    uint64_t t1, t2, elapsed = 0;
    int status_assert = 0;

    t1 = k_ticks_to_us_ceil64(k_uptime_ticks());        // ticks to us
    while (1) {
        status_assert = is_status(dev);
        if (status_assert) {
            break;
        }

        t2 = k_ticks_to_us_ceil64(k_uptime_ticks());    // ticks to us
        elapsed = t2 - t1;
        if (elapsed >= timeout) {
            break;
        }
    }

    return (elapsed >= timeout);
}

static int nu_int_i2c_poll_tran_heatbeat_timeout(const struct device *dev, uint32_t timeout)
{
    struct i2c_numaker_data *data = dev->data;
    int tran_started;
    char *tran_pos = NULL;
    char *tran_pos2 = NULL;

    nu_int_i2c_disable_int(dev);
    tran_pos = data->i2c.tran_pos;
    nu_int_i2c_enable_int(dev);
    while (1) {
        nu_int_i2c_disable_int(dev);
        tran_started = nu_int_i2c_is_tran_started(dev);
        nu_int_i2c_enable_int(dev);
        if (! tran_started) {    // Active transfer completed or stopped
            return 0;
        }

        /* Wait on active transfer completed or stopped
         * NOTE: K_USEC(timeout): us to k_timeout_t */
        int err = k_sem_take(&data->xfer_sync, K_USEC(timeout));
        if (err != 0) {
            nu_int_i2c_disable_int(dev);
            tran_pos2 = data->i2c.tran_pos;
            nu_int_i2c_enable_int(dev);
            if (tran_pos2 != tran_pos) {    // Active transfer on-going
                tran_pos = tran_pos2;
                continue;
            } else {                        // Active transfer idle
                return 1;
            }
        }
    }
}

static int nu_int_i2c_is_trsn_done(const struct device *dev)
{
    const struct i2c_numaker_config *config = dev->config;
    I2C_T *i2c_base = config->i2c_base;
    int i2c_int;
    uint32_t status;
    int inten_back;

    inten_back = nu_int_i2c_set_int(dev, 0);
    i2c_int = !! (i2c_base->CTL0 & I2C_CTL0_SI_Msk);
    status = I2C_GET_STATUS(i2c_base);
    nu_int_i2c_set_int(dev, inten_back);

    return (i2c_int || status == 0xF8);
}

static int nu_int_i2c_is_tran_started(const struct device *dev)
{
    struct i2c_numaker_data *data = dev->data;
    int started;
    int inten_back;

    inten_back = nu_int_i2c_set_int(dev, 0);
    started = !! (data->i2c.tran_ctrl & TRANCTRL_STARTED);
    nu_int_i2c_set_int(dev, inten_back);

    return started;
}

static int nu_int_i2c_addr2data(int address, int read)
{
    return read ? ((address << 1) | 1) : (address << 1);
}

#ifdef CONFIG_I2C_SLAVE
static int nu_int_i2c_addr2bspaddr(int address)
{
    return address;
}

static void nu_int_i2c_enable_slave_if_registered(const struct device *dev)
{
    const struct i2c_numaker_config *config = dev->config;
    I2C_T *i2c_base = config->i2c_base;

    if (nu_int_i2c_is_slave_registered(dev)) {
        /* Switch to not addressed mode. Own SLA will be recognized. */
        I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk | I2C_CTL0_AA_Msk);
    } else {
        /* Switch to not addressed mode. Own SLA will not be recognized. */
        I2C_SET_CONTROL_REG(i2c_base, I2C_CTL0_SI_Msk);
    }
}

static int nu_int_i2c_is_slave_registered(const struct device *dev)
{
    struct i2c_numaker_data *data = dev->data;

    if (data->slave_config) {
        return 1;
    } else {
        return 0;
    }
}

static int nu_int_i2c_is_slave_busy(const struct device *dev)
{
    struct i2c_numaker_data *data = dev->data;

    if (data->i2c.slaveaddr_state != NoData) {
        return 1;
    } else {
        return 0;
    }
}

#endif

static void nu_int_i2c_enable_int(const struct device *dev)
{
    const struct i2c_numaker_config *config = dev->config;
    struct i2c_numaker_data *data = dev->data;

    // Enable I2C interrupt
    NVIC_EnableIRQ(config->irq_n);
    data->i2c.inten = 1;
}

static void nu_int_i2c_disable_int(const struct device *dev)
{
    const struct i2c_numaker_config *config = dev->config;
    struct i2c_numaker_data *data = dev->data;

    // Disable I2C interrupt
    NVIC_DisableIRQ(config->irq_n);
    data->i2c.inten = 0;
}

static int nu_int_i2c_set_int(const struct device *dev, int inten)
{
    struct i2c_numaker_data *data = dev->data;
    int inten_back = data->i2c.inten;

    if (inten) {
        nu_int_i2c_enable_int(dev);
    } else {
        nu_int_i2c_disable_int(dev);
    }

    return inten_back;
}

static int nu_int_i2c_start(const struct device *dev)
{
    return nu_int_i2c_do_trsn(dev, I2C_CTL0_STA_Msk | I2C_CTL0_SI_Msk, 1);
}

static int nu_int_i2c_stop(const struct device *dev)
{
    return nu_int_i2c_do_trsn(dev, I2C_CTL0_STO_Msk | I2C_CTL0_SI_Msk, 1);
}

static int nu_int_i2c_reset(const struct device *dev)
{
    return nu_int_i2c_stop(dev);
}

__maybe_unused
static int nu_int_i2c_byte_read(const struct device *dev, int last)
{
    char data = 0;
    nu_int_i2c_do_tran(dev, &data, 1, 1, last);
    return data;
}

static int nu_int_i2c_byte_write(const struct device *dev, int data)
{
    char data_[1];
    data_[0] = data & 0xFF;

    if (nu_int_i2c_do_tran(dev, data_, 1, 0, 0) == 1 &&
            ! (((struct i2c_numaker_data *) dev->data)->i2c.tran_ctrl & TRANCTRL_LASTDATANAKED)) {
        return 1;
    } else {
        return 0;
    }
}
