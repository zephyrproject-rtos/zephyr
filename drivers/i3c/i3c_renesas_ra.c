/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <r_i3c.h>
#include <rp_i3c.h>

#define DT_DRV_COMPAT renesas_ra_i3c

LOG_MODULE_REGISTER(i3c_ra, CONFIG_I3C_LOG_LEVEL);

#define I3C_RENESAS_RA_DATBAS_NUM                   (8U)
#define I3C_RENESAS_RA_BUS_OPEN                     (('I' << 16U) | ('3' << 8U) | ('C' << 0U))
#define I3C_RENESAS_RA_TYP_OD_RATE                  (1000000U)
#define I3C_RENESAS_RA_TYP_PP_RATE                  (4000000U)
#define I3C_RENESAS_RA_BUS_FREE_DETECTION_TIME      (7U)
#define I3C_RENESAS_RA_BUS_AVAILABLE_DETECTION_TIME (160U)
#define I3C_RENESAS_RA_BUS_IDLE_DETECTION_TIME      (160000U)
#define RESET_VALUE                                 (0U)
#define RSP_STT_SUCCESS                             (0x00)
#define RSP_STT_ABORTED                             (0x08)

/* SCL Specifications */
#define I3C_RENESAS_RA_TRANSFER_TIMEOUT K_MSEC(500U) /* Default timeout */
#define I3C_RENESAS_RA_OD_RISING_NS     (0U)         /* Open Drain Logic Rising Time (ns) */
#define I3C_RENESAS_RA_OD_FALLING_NS    (0U)         /* Open Drain Logic Falling Time (ns) */
#define I3C_RENESAS_RA_PP_RISING_NS     (0U)         /* Open Drain Logic Rising Time (ns) */
#define I3C_RENESAS_RA_PP_FALLING_NS    (0U)         /* Open Drain Logic Falling Time (ns) */
#define I3C_RENESAS_RA_OD_HIGH_NS       (167U)       /* Open Drain Logic High Time (ns) */
#define I3C_RENESAS_RA_PP_HIGH_NS       (50U)        /* Push Pull Logic High Time (ns) */
#define I3C_RENESAS_RA_EBRHP_MAX        (R_I3C0_EXTBR_EBRHP_Msk >> R_I3C0_EXTBR_EBRHP_Pos)
#define I3C_RENESAS_RA_EBRLP_MAX        (R_I3C0_EXTBR_EBRLP_Msk >> R_I3C0_EXTBR_EBRLP_Pos)
#define I3C_RENESAS_RA_EBRHO_MAX        (R_I3C0_EXTBR_EBRHO_Msk >> R_I3C0_EXTBR_EBRHO_Pos)
#define I3C_RENESAS_RA_EBRLO_MAX        (R_I3C0_EXTBR_EBRLO_Msk >> R_I3C0_EXTBR_EBRLO_Pos)
#define I3C_RENESAS_RA_SBRHP_MAX        (R_I3C0_STDBR_SBRHP_Msk >> R_I3C0_STDBR_SBRHP_Pos)
#define I3C_RENESAS_RA_SBRLP_MAX        (R_I3C0_STDBR_SBRLP_Msk >> R_I3C0_STDBR_SBRLP_Pos)
#define I3C_RENESAS_RA_SBRHO_MAX        (R_I3C0_STDBR_SBRHO_Msk >> R_I3C0_STDBR_SBRHO_Pos)
#define I3C_RENESAS_RA_SBRLO_MAX        (R_I3C0_STDBR_SBRLO_Msk >> R_I3C0_STDBR_SBRLO_Pos)

/* Specific data for clock settings */
typedef enum {
	RENESAS_RA_I3C_SCL_PUSHPULL,
	RENESAS_RA_I3C_SCL_OPENDRAIN,
} i3c_renesas_ra_i3c_scl_mode;

struct i3c_renesas_ra_scl_period {
	uint32_t bitrate;                 /* Desired bitrate value */
	uint8_t divider;                  /* Only meaning in standard opendrain */
	i3c_renesas_ra_i3c_scl_mode mode; /* SCL push-pull/opendrain mode */
	uint32_t t_high_ns;               /* SCL Logic High Time in nanoseconds */
	uint32_t t_low_ns;                /* SCL Logic High Time in nanoseconds */
	uint32_t t_rising_ns;             /* SCL Logic Rising Time in nanoseconds */
	uint32_t t_falling_ns;            /* SCL Logic Falling Time in nanoseconds */
	uint16_t high;                    /* Count value of the high-level period of SCL clock */
	uint16_t low;                     /* Count value of the low-level period of SCL clock */
	uint16_t h_max;                   /* max count value of the high level in register */
	uint16_t l_max;                   /* max count value of the low level in register */
};

struct i3c_renesas_ra_dev_info {
	uint8_t static_address;
	uint8_t dynamic_address;
	uint8_t active;
};

/* i3c device data and config*/
struct i3c_renesas_ra_data {
	struct i3c_driver_data common; /* I3C driver data */
	struct k_mutex bus_lock;       /* Used for bus protection */
	struct k_sem daa_end;
	struct k_sem ccc_end;
	struct k_sem xfer_end;
	uint32_t num_xfer;
	uint32_t cb_status;
	i3c_bitrate_mode_t i3c_mode;
	struct i3c_renesas_ra_dev_info *device_info;
	i3c_instance_ctrl_t *fsp_ctrl;         /* FSP control block */
	i3c_cfg_t *fsp_cfg;                    /* FSP configuration block  */
	i3c_device_cfg_t *fsp_master_cfg;      /* FSP master configuration */
	i3c_device_table_cfg_t *fsp_dev_table; /* DAT setting scheme */
	enum i3c_bus_mode mode;
	bool bus_configured;     /* true if bus had been configured */
	bool skip_address_phase; /* true to skip address phase handle */
	uint8_t address_phase_count;
};

struct i3c_renesas_ra_config {
	struct i3c_driver_config common;
	const struct pinctrl_dev_config *pin_cfg;       /* Pin control */
	const struct device *pclk_dev;                  /* Bus clock */
	const struct device *tclk_dev;                  /* Transfer clock */
	struct clock_control_ra_subsys_cfg pclk_subsys; /* Bus clock subsys */
	struct clock_control_ra_subsys_cfg tclk_subsys; /* Transfer clock subsys */
	void (*bus_enable_irq)(void);
};

/* HAL isr */
extern void i3c_resp_isr(void);
extern void i3c_rx_isr(void);
extern void i3c_tx_isr(void);
extern void i3c_rcv_isr(void);
extern void i3c_eei_isr(void);

static enum i3c_bus_mode i3c_renesas_ra_get_bus_mode(const struct i3c_dev_list *dev_list)
{
	enum i3c_bus_mode mode = I3C_BUS_MODE_PURE;

	for (int i = 0; i < dev_list->num_i2c; i++) {
		switch (I3C_LVR_I2C_DEV_IDX(dev_list->i2c[i].lvr)) {
		case I3C_LVR_I2C_DEV_IDX_0:
			if (mode < I3C_BUS_MODE_MIXED_FAST) {
				mode = I3C_BUS_MODE_MIXED_FAST;
			}
			break;
		case I3C_LVR_I2C_DEV_IDX_1:
			if (mode < I3C_BUS_MODE_MIXED_LIMITED) {
				mode = I3C_BUS_MODE_MIXED_LIMITED;
			}
			break;
		case I3C_LVR_I2C_DEV_IDX_2:
			if (mode < I3C_BUS_MODE_MIXED_SLOW) {
				mode = I3C_BUS_MODE_MIXED_SLOW;
			}
			break;
		default:
			mode = I3C_BUS_MODE_INVALID;
			break;
		}
	}
	return mode;
}

static int i3c_renesas_ra_address_slots_init(const struct device *dev)
{
	const struct i3c_renesas_ra_config *config = dev->config;
	struct i3c_renesas_ra_data *data = dev->data;
	uint8_t controller_da;
	int ret = 0;

	ret = i3c_addr_slots_init(dev);
	if (ret) {
		LOG_ERR("Apply i3c_addr_slots_init() fail %d", ret);
		return ret;
	}

	if (config->common.primary_controller_da) {
		if (!i3c_addr_slots_is_free(&data->common.attached_dev.addr_slots,
					    config->common.primary_controller_da)) {
			controller_da = i3c_addr_slots_next_free_find(
				&data->common.attached_dev.addr_slots, 0);
			LOG_WRN("%s: 0x%02x DA selected for controller as 0x%02x is unavailable",
				dev->name, controller_da, config->common.primary_controller_da);
		} else {
			controller_da = config->common.primary_controller_da;
		}
	} else {
		controller_da =
			i3c_addr_slots_next_free_find(&data->common.attached_dev.addr_slots, 0);
	}
	if (controller_da == 0) {
		return -ENOSPC;
	}
	/* Set master address before configuring bus */
	data->fsp_master_cfg->dynamic_address = controller_da;
	/* Mark the address as I3C device */
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, controller_da);

	LOG_DBG("Controller address: 0x%02X", controller_da);
	return 0;
}

static int i3c_renesas_ra_device_index_find(const struct device *dev, uint8_t addr, bool i2c_dev)
{
	struct i3c_renesas_ra_data *data = dev->data;
	int index = -1;

	/* Find device index */
	if (i2c_dev) {
		for (int i = I3C_RENESAS_RA_DATBAS_NUM - 1; i >= 0; i--) {
			if (data->device_info[i].static_address == addr) {
				index = i;
				break;
			}
		}
	} else {
		for (int i = 0; i < I3C_RENESAS_RA_DATBAS_NUM; i++) {
			if (data->device_info[i].dynamic_address == addr) {
				index = i;
				break;
			}
		}
	}

	return index;
}

static int i3c_renesas_ra_device_index_request(const struct device *dev, uint8_t addr, bool i2c_dev)
{
	struct i3c_renesas_ra_data *data = dev->data;
	int index = -1;

	/* Find device index */
	index = i3c_renesas_ra_device_index_find(dev, addr, i2c_dev);

	/* Device not found, register new index */
	if (index < 0) {
		if (i2c_dev) {
			for (int i = I3C_RENESAS_RA_DATBAS_NUM - 1; i >= 0; i--) {
				if (data->device_info[i].active == 0) {
					index = i;
					data->device_info[i].static_address = addr;
					data->device_info[i].active = 1;
					break;
				}
			}
		} else {
			for (int i = 0; i < I3C_RENESAS_RA_DATBAS_NUM; i++) {
				if (data->device_info[i].active == 0) {
					index = i;
					data->device_info[i].dynamic_address = addr;
					data->device_info[i].active = 1;
					break;
				}
			}
		}
	}

	return index;
}

static void i3c_renesas_ra_handle_address_phase(const struct device *dev,
						i3c_slave_info_t const *daa_rx)
{
	const struct i3c_renesas_ra_config *config = dev->config;
	struct i3c_renesas_ra_data *data = dev->data;
	struct i3c_device_desc *target;
	int target_index = -1;
	uint8_t dyn_addr = 0;
	int ret = 0;
	i3c_device_table_cfg_t device_table = {0};

	uint64_t pid = sys_get_be48(&daa_rx->pid[0]);

	fsp_err_t fsp_err = FSP_SUCCESS;

	/* Find device in the device list, assign a dynamic address */
	ret = i3c_dev_list_daa_addr_helper(&data->common.attached_dev.addr_slots,
					   &config->common.dev_list, pid, false, false, &target,
					   &dyn_addr);
	if (ret) {
		LOG_DBG("Assign new DA error");
		goto add_phase_exit;
	}

	/* Update target descriptor */
	target->dynamic_addr = dyn_addr;
	target->bcr = daa_rx->bcr;
	target->dcr = daa_rx->dcr;

	/* Request index for this target */
	target_index = i3c_renesas_ra_device_index_request(dev, dyn_addr, false);
	if (target_index < 0) {
		ret = -ENODEV;
		goto add_phase_exit;
	}

	/* Mark the address as used */
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, dyn_addr);

	/* Mark the static address as free */
	if ((target != NULL) && (target->static_addr != 0U) && (dyn_addr != target->static_addr)) {
		i3c_addr_slots_mark_free(&data->common.attached_dev.addr_slots,
					 target->static_addr);
	}

	/* Update device map */
	data->device_info[target_index].dynamic_address = dyn_addr;
	data->device_info[target_index].active = 1;

	/* Prepare device table before launching DAA */
	device_table.dynamic_address = dyn_addr;
	device_table.static_address = 0x00;
	device_table.device_protocol = I3C_DEVICE_PROTOCOL_I3C;
	device_table.ibi_accept = false;
	device_table.ibi_payload = false;
	device_table.master_request_accept = false;

	/* Add this device to DAT */
	fsp_err = R_I3C_MasterDeviceTableSet(data->fsp_ctrl, target_index, &device_table);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto add_phase_exit;
	}

add_phase_exit:
	if (ret == 0) {
		LOG_DBG("Attach PID[0x%016llX] DA[0x%02X] SA[0x%02X] to DAT%d", target->pid,
			target->dynamic_addr, target->static_addr, target_index);
	} else {
		LOG_DBG("DAA address phase error");
	}
}

static void i3c_renesas_ra_hal_callback(i3c_callback_args_t const *const p_args)
{
	const struct device *dev = (const struct device *)p_args->p_context;
	struct i3c_renesas_ra_data *data = dev->data;

	data->cb_status = p_args->event_status;

	switch (p_args->event) {
	case I3C_EVENT_ENTDAA_ADDRESS_PHASE:
		if (!data->skip_address_phase) {
			i3c_renesas_ra_handle_address_phase(dev, p_args->p_slave_info);
			data->address_phase_count++;
		}
		break;

	case I3C_EVENT_ADDRESS_ASSIGNMENT_COMPLETE:
		k_sem_give(&data->daa_end);
		break;

	case I3C_EVENT_READ_COMPLETE:
		__fallthrough;
	case I3C_EVENT_WRITE_COMPLETE:
		data->num_xfer = p_args->transfer_size;
		k_sem_give(&data->xfer_end);
		break;

	case I3C_EVENT_COMMAND_COMPLETE:
		data->num_xfer = p_args->transfer_size;
		k_sem_give(&data->ccc_end);
		break;

	default:
		break;
	}
}

/* Specific functions */
static int calculate_period(uint32_t tclk_rate, struct i3c_renesas_ra_scl_period *period)
{
	double divider = period->divider;
	double t_rising_ns = period->t_rising_ns;
	double t_falling_ns = period->t_falling_ns;
	double t_high_ns = period->t_high_ns;
	double bitrate = period->bitrate;
	int mode = period->mode;

	uint32_t scl_cnt_high = (double)tclk_rate * t_high_ns / ((double)1e9 * divider);
	double actual_t_high_ns = (double)scl_cnt_high * (double)1e9 * divider / (double)tclk_rate;
	double t_low_ns = ((double)1e9 / bitrate) - actual_t_high_ns - t_rising_ns - t_falling_ns;

	if (mode == RENESAS_RA_I3C_SCL_OPENDRAIN && t_low_ns < 200.0) {
		LOG_DBG("SCL Low period must be greater than or equal to 200 nanoseconds.");
	}
	if (mode == RENESAS_RA_I3C_SCL_PUSHPULL && t_low_ns < 24.0) {
		LOG_DBG("SCL Low period must be greater than or equal to 24 nanoseconds.");
	}
	uint32_t scl_cnt_low = t_low_ns * (double)tclk_rate / ((double)1e9 * divider);

	if (scl_cnt_high > period->h_max || scl_cnt_low > period->l_max || scl_cnt_high == 0 ||
	    scl_cnt_low == 0) {
		return -EINVAL;
	}
	period->t_high_ns = actual_t_high_ns;
	period->t_low_ns = t_low_ns;
	period->high = scl_cnt_high;
	period->low = scl_cnt_low;
	period->bitrate = tclk_rate / ((scl_cnt_high + scl_cnt_low) * divider);

	return 0;
}

static int i3c_renesas_ra_bitrate_setup(const struct device *dev)
{
	const struct i3c_renesas_ra_config *config = dev->config;
	struct i3c_renesas_ra_data *data = dev->data;
	i3c_extended_cfg_t *p_extend = (i3c_extended_cfg_t *)data->fsp_cfg->p_extend;
	i3c_bitrate_settings_t *bitrate_setting = &p_extend->bitrate_settings;
	uint32_t i3c_bitrate = data->common.ctrl_config.scl.i3c;
	uint32_t i2c_bitrate = data->common.ctrl_config.scl.i2c;
	uint8_t dsbrpo = 0;
	uint32_t tclk_rate;
	uint32_t pclk_rate;
	int ret = 0;

	struct i3c_renesas_ra_scl_period std_opendrain;
	struct i3c_renesas_ra_scl_period std_pushpull;
	struct i3c_renesas_ra_scl_period ext_opendrain;
	struct i3c_renesas_ra_scl_period ext_pushpull;

	if (i3c_bitrate < i2c_bitrate) {
		return -EINVAL;
	}

	/* Use STDBR for I2C and I3C transfers */
	std_opendrain.bitrate = i2c_bitrate;
	std_opendrain.divider = 1;
	std_opendrain.mode = RENESAS_RA_I3C_SCL_OPENDRAIN;
	std_opendrain.t_high_ns = I3C_RENESAS_RA_OD_HIGH_NS;
	std_opendrain.t_rising_ns = I3C_RENESAS_RA_OD_RISING_NS;
	std_opendrain.t_falling_ns = I3C_RENESAS_RA_OD_FALLING_NS;
	std_opendrain.h_max = I3C_RENESAS_RA_SBRHO_MAX;
	std_opendrain.l_max = I3C_RENESAS_RA_SBRLO_MAX;

	std_pushpull.bitrate = i3c_bitrate;
	std_pushpull.divider = 1;
	std_pushpull.mode = RENESAS_RA_I3C_SCL_PUSHPULL;
	std_pushpull.t_high_ns = I3C_RENESAS_RA_PP_HIGH_NS;
	std_pushpull.t_rising_ns = I3C_RENESAS_RA_PP_RISING_NS;
	std_pushpull.t_falling_ns = I3C_RENESAS_RA_PP_FALLING_NS;
	std_pushpull.h_max = I3C_RENESAS_RA_SBRHP_MAX;
	std_pushpull.l_max = I3C_RENESAS_RA_SBRLP_MAX;

	/* Set EXTBR */
	ext_opendrain.bitrate = I3C_RENESAS_RA_TYP_OD_RATE;
	ext_opendrain.divider = 1;
	ext_opendrain.mode = RENESAS_RA_I3C_SCL_OPENDRAIN;
	ext_opendrain.t_high_ns = I3C_RENESAS_RA_OD_HIGH_NS;
	ext_opendrain.t_rising_ns = I3C_RENESAS_RA_OD_RISING_NS;
	ext_opendrain.t_falling_ns = I3C_RENESAS_RA_OD_FALLING_NS;
	ext_opendrain.h_max = I3C_RENESAS_RA_EBRHO_MAX;
	ext_opendrain.l_max = I3C_RENESAS_RA_EBRLO_MAX;

	ext_pushpull.bitrate = I3C_RENESAS_RA_TYP_PP_RATE;
	ext_pushpull.divider = 1;
	ext_pushpull.mode = RENESAS_RA_I3C_SCL_PUSHPULL;
	ext_pushpull.t_high_ns = I3C_RENESAS_RA_PP_HIGH_NS;
	ext_pushpull.t_rising_ns = I3C_RENESAS_RA_PP_RISING_NS;
	ext_pushpull.t_falling_ns = I3C_RENESAS_RA_PP_FALLING_NS;
	ext_pushpull.h_max = I3C_RENESAS_RA_EBRHP_MAX;
	ext_pushpull.l_max = I3C_RENESAS_RA_EBRLP_MAX;

	/* Save bitrate mode */
	data->i3c_mode = I3C_BITRATE_MODE_I3C_SDR0_STDBR;

	clock_control_get_rate(config->tclk_dev, (clock_control_subsys_t)&config->tclk_subsys,
			       &tclk_rate);
	clock_control_get_rate(config->pclk_dev, (clock_control_subsys_t)&config->pclk_subsys,
			       &pclk_rate);
	LOG_DBG("Clock rate I3CCLK %d PCLK %d", tclk_rate, pclk_rate);

	/*  Relation between the bus clock (PCLK) and transfer clock(TCLK) */
	if (pclk_rate > tclk_rate || pclk_rate < tclk_rate / 2) {
		ret = -EINVAL;
		goto bitrate_exit;
	}

	/* Calculate period setting for scl in standard opendrain modes */
	ret = calculate_period(tclk_rate, &std_opendrain);
	if (ret) {
		/*
		 * Try resolve with dsbrpro=1,
		 * double scl bitrate for standard opendrain mode
		 */
		dsbrpo = 1;
		std_opendrain.divider = 2;
		ret = calculate_period(tclk_rate, &std_opendrain);
	}
	if (ret) {
		goto bitrate_exit;
	}

	/* Calculate period setting for scl in standard pushpull modes */
	ret = calculate_period(tclk_rate, &std_pushpull);
	if (ret) {
		goto bitrate_exit;
	}

	/* Calculate period setting for scl in extexnded opendrain modes */
	ret = calculate_period(tclk_rate, &ext_opendrain);
	if (ret) {
		goto bitrate_exit;
	}

	/* Calculate period setting for scl in extexnded pushpull modes */
	ret = calculate_period(tclk_rate, &ext_pushpull);
	if (ret) {
		goto bitrate_exit;
	}

	LOG_DBG("actual I2C speed: %d Mbps", std_opendrain.bitrate);
	LOG_DBG("actual I3C speed: OD %d Mbps, PP %d Mbps", std_opendrain.bitrate,
		std_pushpull.bitrate);

	bitrate_setting->stdbr = ((std_opendrain.high << R_I3C0_STDBR_SBRHO_Pos) |
				  (std_opendrain.low << R_I3C0_STDBR_SBRLO_Pos)) |
				 ((std_pushpull.high << R_I3C0_STDBR_SBRHP_Pos) |
				  (std_pushpull.low << R_I3C0_STDBR_SBRLP_Pos)) |
				 (dsbrpo << R_I3C0_STDBR_DSBRPO_Pos);
	bitrate_setting->extbr = ((ext_opendrain.high << R_I3C0_EXTBR_EBRHO_Pos) |
				  (ext_opendrain.low << R_I3C0_EXTBR_EBRLO_Pos)) |
				 ((ext_pushpull.high << R_I3C0_EXTBR_EBRHP_Pos) |
				  (ext_pushpull.low << R_I3C0_EXTBR_EBRLP_Pos));

bitrate_exit:
	return ret;
}

/* i3c interface */
static int i3c_renesas_ra_configure(const struct device *dev, enum i3c_config_type type,
				    void *bus_config)
{
	struct i3c_renesas_ra_data *data = dev->data;
	struct i3c_config_controller *ctrler_cfg;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;
	i3c_device_table_cfg_t device[I3C_RENESAS_RA_DATBAS_NUM];

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	switch (type) {
	case I3C_CONFIG_CONTROLLER:
		ctrler_cfg = bus_config;
		/* Unsupported mode */
		if (ctrler_cfg->is_secondary || ctrler_cfg->supported_hdr) {
			ret = -ENOTSUP;
			goto configure_exit;
		}

		if ((ctrler_cfg->scl.i2c == 0U) || (ctrler_cfg->scl.i3c == 0U)) {
			ret = -EINVAL;
			goto configure_exit;
		}
		/* Save bitrate setting to device data */
		data->common.ctrl_config.scl.i3c = ctrler_cfg->scl.i3c;
		data->common.ctrl_config.scl.i2c = ctrler_cfg->scl.i2c;

		/* Bitrate settings */
		ret = i3c_renesas_ra_bitrate_setup(dev);
		if (ret) {
			LOG_ERR("Failed to resolve bitrate settings");
			goto configure_exit;
		}

		/* retain DAT */
		for (int i = 0; i < I3C_RENESAS_RA_DATBAS_NUM; i++) {
			if (!data->device_info[i].active) {
				continue;
			}
			fsp_err =
				R_I3C_MasterDeviceTableGet(data->fsp_ctrl, (uint32_t)i, &device[i]);
			if (fsp_err != FSP_SUCCESS) {
				LOG_DBG("retain DAT failed, err=%d", ret);
				ret = -EIO;
				goto configure_exit;
			}
		}

		/* Close bus*/
		if (data->fsp_ctrl->open == I3C_RENESAS_RA_BUS_OPEN) {
			fsp_err = R_I3C_Close(data->fsp_ctrl);
			if (fsp_err != FSP_SUCCESS) {
				LOG_ERR("Failed to init i3c bus, err=%d", fsp_err);
				ret = -EIO;
				goto configure_exit;
			}
		}

		/* Open I3C bus */
		data->fsp_cfg->device_type = I3C_DEVICE_TYPE_MAIN_MASTER;
		fsp_err = R_I3C_Open(data->fsp_ctrl, data->fsp_cfg);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Failed to init i3c bus, err=%d", fsp_err);
			ret = -EIO;
			goto configure_exit;
		}

		/* reload DAT */
		for (int i = 0; i < I3C_RENESAS_RA_DATBAS_NUM; i++) {
			if (!data->device_info[i].active) {
				continue;
			}
			fsp_err =
				R_I3C_MasterDeviceTableSet(data->fsp_ctrl, (uint32_t)i, &device[i]);
			if (fsp_err != FSP_SUCCESS) {
				LOG_DBG("reload DAT failed %d, err=%d", i, ret);
				ret = -EIO;
				goto configure_exit;
			}
		}

		/* Set this device as master role */
		fsp_err = R_I3C_DeviceCfgSet(data->fsp_ctrl, data->fsp_master_cfg);
		if (ret) {
			LOG_ERR("Failed to init i3c controller, err=%d", ret);
			ret = -EIO;
			goto configure_exit;
		}

		/* Enable bus to apply bitrate setting */
		fsp_err = R_I3C_Enable(data->fsp_ctrl);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Failed to enable bus, err=%d", fsp_err);
			ret = -EIO;
			goto configure_exit;
		}
		break;

	case I3C_CONFIG_TARGET:
		/* TODO: target mode */
		ret = -ENOTSUP;
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

configure_exit:
	k_mutex_unlock(&data->bus_lock);

	return ret;
}

static int i3c_renesas_ra_config_get(const struct device *dev, enum i3c_config_type type,
				     void *bus_config)
{
	struct i3c_renesas_ra_data *data = dev->data;

	if (type == I3C_CONFIG_CONTROLLER) {
#ifdef CONFIG_I3C_CONTROLLER
		(void)memcpy(bus_config, &data->common.ctrl_config,
			     sizeof(data->common.ctrl_config));
#else
		return -ENOTSUP;
#endif /* CONFIG_I3C_CONTROLLER */
	} else {
		return -EINVAL;
	}

	return 0;
}

static int i3c_renesas_ra_attach_i3c_device(const struct device *dev,
					    struct i3c_device_desc *target)
{
	struct i3c_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int target_index = -1;
	int ret = 0;
	i3c_device_table_cfg_t device_table = {0};

	if (target->dynamic_addr == 0 && target->static_addr == 0) {
		/*
		 * Do notthing.
		 * This case called from address slots init process.
		 */
		return 0;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	/* Create scheme for saving device in DAT */
	device_table.dynamic_address = target->dynamic_addr ? target->dynamic_addr : 0x00;
	device_table.static_address = target->static_addr ? target->static_addr : 0x00;
	device_table.device_protocol = I3C_DEVICE_PROTOCOL_I3C;
	device_table.ibi_accept = false;
	device_table.ibi_payload = false;
	device_table.master_request_accept = false;

	target_index = i3c_renesas_ra_device_index_request(
		dev, (target->dynamic_addr) ? target->dynamic_addr : target->static_addr, false);
	if (target_index < 0) {
		ret = -ERANGE;
		goto attach_i3c_exit;
	}

	fsp_err = R_I3C_MasterDeviceTableSet(data->fsp_ctrl, target_index, &device_table);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto attach_i3c_exit;
	}

attach_i3c_exit:
	k_mutex_unlock(&data->bus_lock);

	if (ret == 0) {
		LOG_DBG("Attach PID[0x%016llX] DA[0x%02X] SA[0x%02X] to DAT%d", target->pid,
			target->dynamic_addr, target->static_addr, target_index);
	}

	return ret;
}

static int i3c_renesas_ra_reattach_i3c_device(const struct device *dev,
					      struct i3c_device_desc *target, uint8_t old_dyn_addr)
{
	struct i3c_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int target_index = -1;
	int ret = 0;
	i3c_device_table_cfg_t device_table = {0};

	if (target->dynamic_addr == 0 && target->static_addr == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	target_index = i3c_renesas_ra_device_index_find(dev, old_dyn_addr, false);
	if (target_index < 0) {
		ret = -ENODEV;
		goto reattach_i3c_exit;
	}

	fsp_err = R_I3C_MasterDeviceTableGet(data->fsp_ctrl, target_index, &device_table);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto reattach_i3c_exit;
	}
	device_table.dynamic_address = target->dynamic_addr ? target->dynamic_addr : 0x00;
	device_table.static_address = target->static_addr ? target->static_addr : 0x00;

	fsp_err = R_I3C_MasterDeviceTableSet(data->fsp_ctrl, target_index, &device_table);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto reattach_i3c_exit;
	}

reattach_i3c_exit:
	k_mutex_unlock(&data->bus_lock);

	if (ret == 0) {
		LOG_DBG("Reattach PID[0x%016llX] DA[0x%02X] SA[0x%02X] to DAT%d", target->pid,
			target->dynamic_addr, target->static_addr, target_index);
	}
	return ret;
}

static int i3c_renesas_ra_detach_i3c_device(const struct device *dev,
					    struct i3c_device_desc *target)
{
	struct i3c_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;
	int target_index = -1;

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	target_index = i3c_renesas_ra_device_index_find(
		dev, (target->dynamic_addr) ? target->dynamic_addr : target->static_addr, false);
	if (target_index < 0) {
		ret = -ERANGE;
		goto detach_i3c_exit;
	}

	fsp_err = R_I3C_MasterDeviceTableReset(data->fsp_ctrl, target_index);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto detach_i3c_exit;
	}

detach_i3c_exit:
	k_mutex_unlock(&data->bus_lock);

	if (ret == 0) {
		LOG_DBG("Detach PID[0x%016llX] from Device Table", target->pid);
	}

	return ret;
}

static int i3c_renesas_ra_do_daa(const struct device *dev)
{
	const struct i3c_renesas_ra_config *config = dev->config;
	struct i3c_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	/* If num_i3c is 0, set num_dev to 1 for handling daa called from hot-join IBI */
	uint32_t num_dev = (config->common.dev_list.num_i3c) ? config->common.dev_list.num_i3c : 1;
	uint32_t start_index = 0;

	/* Start DAA without address asignment to get device info */
	data->address_phase_count = 0;
	data->skip_address_phase = false;
	fsp_err = R_I3C_DynamicAddressAssignmentStart(data->fsp_ctrl, I3C_CCC_ENTDAA, start_index,
						      num_dev);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto daa_exit;
	}

	ret = k_sem_take(&data->daa_end, I3C_RENESAS_RA_TRANSFER_TIMEOUT);
	if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
		goto daa_exit;
	}

	if (data->address_phase_count == 0) {
		/* No device apply DA */
		goto daa_exit;
	}

	/* Start DAA again to apply new addresses */
	data->skip_address_phase = true;
	fsp_err = R_I3C_DynamicAddressAssignmentStart(data->fsp_ctrl, I3C_CCC_ENTDAA, start_index,
						      num_dev);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto daa_exit;
	}

	ret = k_sem_take(&data->daa_end, I3C_RENESAS_RA_TRANSFER_TIMEOUT);
	if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
		goto daa_exit;
	}

	if (data->cb_status != RSP_STT_SUCCESS) {
		ret = -EIO;
		goto daa_exit;
	}

	goto daa_exit;

daa_exit:
	k_mutex_unlock(&data->bus_lock);

	LOG_DBG("DAA %s", ret ? "failed" : "complete");
	return ret;
}

static int i3c_renesas_ra_do_dasa(const struct device *dev)
{
	struct i3c_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	fsp_err = R_I3C_DynamicAddressAssignmentStart(data->fsp_ctrl, I3C_CCC_SETDASA, 0, 1);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto dasa_exit;
	}

	ret = k_sem_take(&data->daa_end, I3C_RENESAS_RA_TRANSFER_TIMEOUT);
	if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
		goto dasa_exit;
	}

	if (data->cb_status != RSP_STT_SUCCESS) {
		ret = -EIO;
		goto dasa_exit;
	}

	goto dasa_exit;

dasa_exit:
	k_mutex_unlock(&data->bus_lock);

	LOG_DBG("DASA %s", ret ? "failed" : "complete");
	return ret;
}

static int i3c_renesas_ra_broadcast_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	struct i3c_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;
	i3c_command_descriptor_t cmd = {0};

	cmd.command_code = payload->ccc.id;
	cmd.restart = 0;
	cmd.rnw = 0; /* Broadcast is always write */
	cmd.p_buffer = (payload->ccc.data_len) ? payload->ccc.data : NULL;
	cmd.length = (uint32_t)payload->ccc.data_len;

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	/* Select bitrate mode, ignore target index */
	fsp_err = R_I3C_DeviceSelect(data->fsp_ctrl, 0x00, data->i3c_mode);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto bc_ccc_exit;
	}

	fsp_err = R_I3C_CommandSend(data->fsp_ctrl, &cmd);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto bc_ccc_exit;
	}

	ret = k_sem_take(&data->ccc_end, I3C_RENESAS_RA_TRANSFER_TIMEOUT);
	if (ret == -EAGAIN) {
		ret = -ETIMEDOUT;
		goto bc_ccc_exit;
	}

	if (data->cb_status != RSP_STT_SUCCESS) {
		ret = -EIO;
		goto bc_ccc_exit;
	}

	payload->ccc.num_xfer = data->num_xfer;

bc_ccc_exit:
	k_mutex_unlock(&data->bus_lock);

	LOG_DBG("broadcast CCC[0x%02X] %s", payload->ccc.id, ret ? "failed" : "complete");
	return ret;
}

static int i3c_renesas_ra_direct_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	struct i3c_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int ret = 0;
	int num_targets = payload->targets.num_targets;
	int target_index = -1;
	i3c_command_descriptor_t cmd = {0};

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	for (int i = 0; i < num_targets; i++) {
		struct i3c_ccc_target_payload *tg_payload = &payload->targets.payloads[i];

		cmd.command_code = payload->ccc.id;
		cmd.restart = (i == num_targets - 1) ? 0 : 1;
		cmd.rnw = tg_payload->rnw;
		cmd.p_buffer = tg_payload->data;
		cmd.length = (uint32_t)tg_payload->data_len;

		target_index = i3c_renesas_ra_device_index_find(dev, tg_payload->addr, false);
		if (target_index < 0) {
			ret = -ENODEV;
			goto dr_ccc_exit;
		}

		/* Select target index and bitrate mode */
		fsp_err = R_I3C_DeviceSelect(data->fsp_ctrl, target_index, data->i3c_mode);
		if (fsp_err != FSP_SUCCESS) {
			ret = -EIO;
			goto dr_ccc_exit;
		}

		payload->ccc.num_xfer = 0;

		fsp_err = R_I3C_CommandSend(data->fsp_ctrl, &cmd);
		if (fsp_err != FSP_SUCCESS) {
			ret = -EIO;
			goto dr_ccc_exit;
		}

		ret = k_sem_take(&data->ccc_end, I3C_RENESAS_RA_TRANSFER_TIMEOUT);
		if (ret == -EAGAIN) {
			ret = -ETIMEDOUT;
			goto dr_ccc_exit;
		}

		if (data->cb_status != RSP_STT_SUCCESS) {
			ret = -EIO;
			goto dr_ccc_exit;
		}

		tg_payload->num_xfer = data->num_xfer;
	}

dr_ccc_exit:
	k_mutex_unlock(&data->bus_lock);

	LOG_DBG("direct CCC[0x%02X] %s", payload->ccc.id, ret ? "failed" : "complete");
	return ret;
}

/* Common command code Method */
static int i3c_renesas_ra_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	if (dev == NULL || payload == NULL ||
	    (payload->ccc.data_len > 0 && payload->ccc.data == NULL)) {
		return -EINVAL;
	}

	if (payload->ccc.id == I3C_CCC_SETDASA) {
		/* SETDASA CCC is not implemented as normal CCC */
		return i3c_renesas_ra_do_dasa(dev);
	}

	if (i3c_ccc_is_payload_broadcast(payload)) {
		return i3c_renesas_ra_broadcast_ccc(dev, payload);
	} else {
		return i3c_renesas_ra_direct_ccc(dev, payload);
	}
}

static int i3c_renesas_ra_i3c_transfer(const struct device *dev, struct i3c_device_desc *target,
				       struct i3c_msg *msgs, uint8_t num_msgs)
{
	struct i3c_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int target_index = -1;
	int ret = 0;

	if (msgs == NULL || target->dynamic_addr == 0U) {
		return -EINVAL;
	}

	/* Verify all messages */
	for (size_t i = 0; i < num_msgs; i++) {
		if (msgs[i].buf == NULL) {
			return -EINVAL;
		}
		if ((msgs[i].flags & I3C_MSG_HDR) && (msgs[i].hdr_mode != 0)) {
			return -ENOTSUP;
		}
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);

	target_index = i3c_renesas_ra_device_index_find(
		dev, (target->dynamic_addr) ? target->dynamic_addr : target->static_addr, false);
	if (target_index < 0) {
		return -ENODEV;
	}

	/* Select target index and bitrate mode */
	fsp_err = R_I3C_DeviceSelect(data->fsp_ctrl, target_index, data->i3c_mode);
	if (fsp_err != FSP_SUCCESS) {
		ret = -EIO;
		goto i3c_xfer_exit;
	}

	for (int i = 0; i < num_msgs; i++) {
		bool msg_rst = (msgs[i].flags & I3C_MSG_STOP) ? false : true;

		if (msgs[i].flags & I3C_MSG_READ) {
			fsp_err = R_I3C_Read(data->fsp_ctrl, msgs[i].buf, msgs[i].len, msg_rst);
		} else {
			fsp_err = R_I3C_Write(data->fsp_ctrl, msgs[i].buf, msgs[i].len, msg_rst);
		}
		msgs[i].num_xfer = data->num_xfer;
		if (fsp_err != FSP_SUCCESS) {
			ret = -EIO;
			goto i3c_xfer_exit;
		}

		ret = k_sem_take(&data->xfer_end, I3C_RENESAS_RA_TRANSFER_TIMEOUT);
		if (ret == -EAGAIN) {
			ret = -ETIMEDOUT;
			goto i3c_xfer_exit;
		}

		if (data->cb_status != RSP_STT_SUCCESS && data->cb_status != RSP_STT_ABORTED) {
			ret = -EIO;
			goto i3c_xfer_exit;
		}
	}
	goto i3c_xfer_exit;

i3c_xfer_exit:
	k_mutex_unlock(&data->bus_lock);

	LOG_DBG("xfer I3C[0x%02X] %s", target->dynamic_addr, ret ? "failed" : "complete");
	return ret;
}

static struct i3c_device_desc *i3c_renesas_ra_device_find(const struct device *dev,
							  const struct i3c_device_id *id)
{
	const struct i3c_renesas_ra_config *config = dev->config;

	return i3c_dev_list_find(&config->common.dev_list, id);
}

static int i3c_renesas_ra_init(const struct device *dev)
{
	const struct i3c_renesas_ra_config *config = dev->config;
	struct i3c_renesas_ra_data *data = dev->data;
	int ret = 0;

	k_sem_init(&data->daa_end, 0, 1);
	k_sem_init(&data->ccc_end, 0, 1);
	k_sem_init(&data->xfer_end, 0, 1);
	k_mutex_init(&data->bus_lock);

	ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Apply pinctrl fail %d", ret);
		return ret;
	}

	ret = clock_control_on(config->pclk_dev, (clock_control_subsys_t)&config->pclk_subsys);
	if (ret) {
		LOG_ERR("Failed to start i3c bus clock, err=%d", ret);
		return ret;
	}

	config->bus_enable_irq();

#ifdef CONFIG_I3C_CONTROLLER
	data->mode = i3c_renesas_ra_get_bus_mode(&config->common.dev_list);

	/* Clear bus internal device info */
	memset(data->device_info, 0x00,
	       sizeof(struct i3c_renesas_ra_dev_info) * I3C_RENESAS_RA_DATBAS_NUM);

	/* Init address slots */
	ret = i3c_renesas_ra_address_slots_init(dev);
	if (ret) {
		LOG_ERR("Failed to set controller address, err=%d", ret);
		return ret;
	}

	/* Configure bus */
	if (i3c_configure(dev, I3C_CONFIG_CONTROLLER, &data->common.ctrl_config)) {
		LOG_ERR("Failed to configure bus");
		return ret;
	}

	/* Check I3C is controller mode and target device exist in device tree */
	if (config->common.dev_list.num_i3c > 0) {
		/* Perform bus initialization */
		ret = i3c_bus_init(dev, &config->common.dev_list);
		if (ret) {
			LOG_ERR("Apply i3c_bus_init() fail %d", ret);
			return ret;
		}
	}
#endif /* CONFIG_I3C_CONTROLLER */
	return 0;
}

/* i3c device API */
static DEVICE_API(i3c, i3c_renesas_ra_api) = {
	.configure = i3c_renesas_ra_configure,
	.config_get = i3c_renesas_ra_config_get,
	.attach_i3c_device = i3c_renesas_ra_attach_i3c_device,
	.reattach_i3c_device = i3c_renesas_ra_reattach_i3c_device,
	.detach_i3c_device = i3c_renesas_ra_detach_i3c_device,
	.do_daa = i3c_renesas_ra_do_daa,
	.do_ccc = i3c_renesas_ra_do_ccc,
	.i3c_xfers = i3c_renesas_ra_i3c_transfer,
	.i3c_device_find = i3c_renesas_ra_device_find,
#ifdef CONFIG_I3C_RTIO
	.iodev_submit = i3c_iodev_submit_fallback,
#endif /* CONFIG_I3C_RTIO */
};

#define I3C_RENESAS_RA_IRQ_EN(index, isr_name, isr_func, event_name)                               \
	R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, isr_name, irq)] = BSP_PRV_IELS_ENUM(event_name);   \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, isr_name, irq),                                     \
		    DT_INST_IRQ_BY_NAME(index, isr_name, priority), isr_func,                      \
		    DEVICE_DT_INST_GET(index), 0);                                                 \
	irq_enable(DT_INST_IRQ_BY_NAME(index, isr_name, irq));

/* HAL Configurations */
#define I3C_RENESAS_RA_HAL_INIT(index)                                                             \
	static i3c_instance_ctrl_t i3c##index##_ctrl;                                              \
	static i3c_extended_cfg_t i3c##index##_cfg_extend = {                                      \
		.bitrate_settings.stdbr = RESET_VALUE,                                             \
		.bitrate_settings.extbr = RESET_VALUE,                                             \
		.bitrate_settings.clock_stalling.assigned_address_phase_enable = RESET_VALUE,      \
		.bitrate_settings.clock_stalling.transition_phase_enable = RESET_VALUE,            \
		.bitrate_settings.clock_stalling.parity_phase_enable = RESET_VALUE,                \
		.bitrate_settings.clock_stalling.ack_phase_enable = RESET_VALUE,                   \
		.bitrate_settings.clock_stalling.clock_stalling_time = RESET_VALUE,                \
		.ibi_control.hot_join_acknowledge = RESET_VALUE,                                   \
		.ibi_control.notify_rejected_hot_join_requests = RESET_VALUE,                      \
		.ibi_control.notify_rejected_mastership_requests = RESET_VALUE,                    \
		.ibi_control.notify_rejected_interrupt_requests = RESET_VALUE,                     \
		.bus_free_detection_time = I3C_RENESAS_RA_BUS_FREE_DETECTION_TIME,                 \
		.bus_available_detection_time = I3C_RENESAS_RA_BUS_AVAILABLE_DETECTION_TIME,       \
		.bus_idle_detection_time = I3C_RENESAS_RA_BUS_IDLE_DETECTION_TIME,                 \
		.timeout_detection_enable = true,                                                  \
		.slave_command_response_info = {0},                                                \
		.resp_irq = DT_INST_IRQ_BY_NAME(index, resp, irq),                                 \
		.rx_irq = DT_INST_IRQ_BY_NAME(index, rx, irq),                                     \
		.tx_irq = DT_INST_IRQ_BY_NAME(index, tx, irq),                                     \
		.rcv_irq = DT_INST_IRQ_BY_NAME(index, rcv, irq),                                   \
		.ibi_irq = DT_INST_IRQ_BY_NAME(index, ibi, irq),                                   \
		.eei_irq = DT_INST_IRQ_BY_NAME(index, eei, irq),                                   \
	};                                                                                         \
	static i3c_cfg_t i3c##index##_cfg = {                                                      \
		.channel = DT_INST_PROP(index, channel),                                           \
		.p_callback = &i3c_renesas_ra_hal_callback,                                        \
		.p_context = (void *)DEVICE_DT_INST_GET(index),                                    \
		.p_extend = &i3c##index##_cfg_extend,                                              \
	};                                                                                         \
	static i3c_device_cfg_t i3c##index##_master_cfg = {0};                                     \
	static struct i3c_renesas_ra_dev_info i3c##index##_dev_inf[I3C_RENESAS_RA_DATBAS_NUM];

/* Device Initialize */
#define I3C_RENESAS_RA_INIT(index)                                                                 \
	I3C_RENESAS_RA_HAL_INIT(index);                                                            \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static struct i3c_device_desc i3c##index##_renesas_ra_i3c_dev_list[] =                     \
		I3C_DEVICE_ARRAY_DT_INST(index);                                                   \
	static struct i3c_i2c_device_desc i3c##index##_renesas_ra_i2c_dev_list[] =                 \
		I3C_I2C_DEVICE_ARRAY_DT_INST(index);                                               \
                                                                                                   \
	static void i3c##index##_renesas_ra_enable_irq(void)                                       \
	{                                                                                          \
		I3C_RENESAS_RA_IRQ_EN(index, resp, i3c_resp_isr, EVENT_I3C##index##_RESPONSE)      \
		I3C_RENESAS_RA_IRQ_EN(index, rx, i3c_rx_isr, EVENT_I3C##index##_RX)                \
		I3C_RENESAS_RA_IRQ_EN(index, tx, i3c_tx_isr, EVENT_I3C##index##_TX)                \
		I3C_RENESAS_RA_IRQ_EN(index, rcv, i3c_rcv_isr, EVENT_I3C##index##_RCV_STATUS)      \
		I3C_RENESAS_RA_IRQ_EN(index, eei, i3c_eei_isr, EVENT_I3C##index##_EEI)             \
	}                                                                                          \
                                                                                                   \
	static struct i3c_renesas_ra_data i3c##index##_renesas_ra_data = {                         \
		.common.ctrl_config.scl.i3c =                                                      \
			DT_INST_PROP_OR(index, i3c_scl_hz, I3C_RENESAS_RA_TYP_PP_RATE),            \
		.common.ctrl_config.scl.i2c =                                                      \
			DT_INST_PROP_OR(index, i2c_scl_hz, I3C_RENESAS_RA_TYP_OD_RATE),            \
		.fsp_ctrl = &i3c##index##_ctrl,                                                    \
		.fsp_cfg = &i3c##index##_cfg,                                                      \
		.fsp_master_cfg = &i3c##index##_master_cfg,                                        \
		.device_info = &i3c##index##_dev_inf[0],                                           \
		.skip_address_phase = true,                                                        \
	};                                                                                         \
                                                                                                   \
	static const struct i3c_renesas_ra_config i3c##index##_renesas_ra_config = {               \
		.common.dev_list.i3c = i3c##index##_renesas_ra_i3c_dev_list,                       \
		.common.dev_list.num_i3c = ARRAY_SIZE(i3c##index##_renesas_ra_i3c_dev_list),       \
		.common.dev_list.i2c = i3c##index##_renesas_ra_i2c_dev_list,                       \
		.common.dev_list.num_i2c = ARRAY_SIZE(i3c##index##_renesas_ra_i2c_dev_list),       \
		.common.primary_controller_da = DT_INST_PROP_OR(index, primary_controller_da, 0),  \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                  \
		.pclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(index, pclk)),               \
		.tclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(index, tclk)),               \
		.pclk_subsys.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(index, pclk, mstp),      \
		.pclk_subsys.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(index, pclk, stop_bit),        \
		.tclk_subsys.mstp = (uint32_t)DT_INST_CLOCKS_CELL_BY_NAME(index, tclk, mstp),      \
		.tclk_subsys.stop_bit = DT_INST_CLOCKS_CELL_BY_NAME(index, tclk, stop_bit),        \
		.bus_enable_irq = &i3c##index##_renesas_ra_enable_irq,                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &i3c_renesas_ra_init, NULL, &i3c##index##_renesas_ra_data,    \
			      &i3c##index##_renesas_ra_config, POST_KERNEL,                        \
			      CONFIG_I3C_CONTROLLER_INIT_PRIORITY, &i3c_renesas_ra_api)

DT_INST_FOREACH_STATUS_OKAY(I3C_RENESAS_RA_INIT)
