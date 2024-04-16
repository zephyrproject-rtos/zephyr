/*
 * Copyright (c) 2019, Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_ipc

#include <string.h>
#include <zephyr/drivers/ipm.h>
#include <nrfx_ipc.h>
#include "ipm_nrfx_ipc.h"

#define LOG_LEVEL CONFIG_IPM_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(ipm_nrfx_ipc);

struct ipm_nrf_data {
	ipm_callback_t callback;
	void *user_data;
};

static struct ipm_nrf_data nrfx_ipm_data;

static void gipm_init(void);
static void gipm_send(uint32_t id);

#if defined(CONFIG_IPM_NRF_SINGLE_INSTANCE)

static void nrfx_ipc_handler(uint8_t event_idx, void *p_context)
{
	if (nrfx_ipm_data.callback) {
		__ASSERT(event_idx < NRFX_IPC_ID_MAX_VALUE,
			 "Illegal event_idx: %d", event_idx);
		nrfx_ipm_data.callback(DEVICE_DT_INST_GET(0),
				       nrfx_ipm_data.user_data,
				       event_idx,
				       NULL);
	}
}

static int ipm_nrf_send(const struct device *dev, int wait, uint32_t id,
			const void *data, int size)
{
	if (id > NRFX_IPC_ID_MAX_VALUE) {
		return -EINVAL;
	}

	if (size > 0) {
		LOG_WRN("nRF driver does not support sending data over IPM");
	}

	gipm_send(id);
	return 0;
}

static int ipm_nrf_max_data_size_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static uint32_t ipm_nrf_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return NRFX_IPC_ID_MAX_VALUE;
}

static void ipm_nrf_register_callback(const struct device *dev,
				      ipm_callback_t cb,
				      void *user_data)
{
	nrfx_ipm_data.callback = cb;
	nrfx_ipm_data.user_data = user_data;
}

static int ipm_nrf_set_enabled(const struct device *dev, int enable)
{
	/* Enable configured channels */
	if (enable) {
		irq_enable(DT_INST_IRQN(0));
		nrfx_ipc_receive_event_group_enable((uint32_t)IPC_EVENT_BITS);
	} else {
		irq_disable(DT_INST_IRQN(0));
		nrfx_ipc_receive_event_group_disable((uint32_t)IPC_EVENT_BITS);
	}
	return 0;
}

static int ipm_nrf_init(const struct device *dev)
{
	gipm_init();
	return 0;
}

static const struct ipm_driver_api ipm_nrf_driver_api = {
	.send = ipm_nrf_send,
	.register_callback = ipm_nrf_register_callback,
	.max_data_size_get = ipm_nrf_max_data_size_get,
	.max_id_val_get = ipm_nrf_max_id_val_get,
	.set_enabled = ipm_nrf_set_enabled
};

DEVICE_DT_INST_DEFINE(0, ipm_nrf_init, NULL, NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &ipm_nrf_driver_api);

#else

struct vipm_nrf_data {
	ipm_callback_t callback[NRFX_IPC_ID_MAX_VALUE];
	void *user_data[NRFX_IPC_ID_MAX_VALUE];
	const struct device *ipm_device[NRFX_IPC_ID_MAX_VALUE];
	bool ipm_init;
};

static struct vipm_nrf_data nrfx_vipm_data;

static void vipm_dispatcher(uint8_t event_idx, void *p_context)
{
	__ASSERT(event_idx < NRFX_IPC_ID_MAX_VALUE,
		 "Illegal event_idx: %d", event_idx);
	if (nrfx_vipm_data.callback[event_idx] != NULL) {
		nrfx_vipm_data.callback[event_idx]
			(nrfx_vipm_data.ipm_device[event_idx],
			 nrfx_vipm_data.user_data[event_idx],
			 0,
			 NULL);
	}
}

static int vipm_nrf_max_data_size_get(const struct device *dev)
{
	return ipm_max_data_size_get(dev);
}

static uint32_t vipm_nrf_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int vipm_nrf_init(const struct device *dev)
{
	if (!nrfx_vipm_data.ipm_init) {
		gipm_init();
		nrfx_vipm_data.ipm_init = true;
	}
	return 0;
}

#define VIPM_DEVICE_1(_idx)						\
static int vipm_nrf_##_idx##_send(const struct device *dev, int wait,	\
				  uint32_t id, const void *data, int size)	\
{									\
	if (!IS_ENABLED(CONFIG_IPM_MSG_CH_##_idx##_TX)) {		\
		LOG_ERR("IPM_" #_idx " is RX message channel");		\
		return -EINVAL;						\
	}								\
									\
	if (id > NRFX_IPC_ID_MAX_VALUE) {				\
		return -EINVAL;						\
	}								\
									\
	if (id != 0) {							\
		LOG_WRN("Passing message ID to IPM with"		\
			"predefined message ID");			\
	}								\
									\
	if (size > 0) {							\
		LOG_WRN("nRF driver does not support"			\
			"sending data over IPM");			\
	}								\
									\
	gipm_send(_idx);						\
	return 0;							\
}									\
									\
static void vipm_nrf_##_idx##_register_callback(const struct device *dev, \
						ipm_callback_t cb,	\
						void *user_data)	\
{									\
	if (IS_ENABLED(CONFIG_IPM_MSG_CH_##_idx##_RX)) {		\
		nrfx_vipm_data.callback[_idx] = cb;			\
		nrfx_vipm_data.user_data[_idx] = user_data;		\
		nrfx_vipm_data.ipm_device[_idx] = dev;			\
	} else {							\
		LOG_WRN("Trying to register a callback"			\
			"for TX channel IPM_" #_idx);			\
	}								\
}									\
									\
static int vipm_nrf_##_idx##_set_enabled(const struct device *dev, int enable)\
{									\
	if (!IS_ENABLED(CONFIG_IPM_MSG_CH_##_idx##_RX)) {		\
		LOG_ERR("IPM_" #_idx " is TX message channel");		\
		return -EINVAL;						\
	} else if (enable) {						\
		irq_enable(DT_INST_IRQN(0));		\
		nrfx_ipc_receive_event_enable(_idx);			\
	} else if (!enable) {						\
		nrfx_ipc_receive_event_disable(_idx);			\
	}								\
	return 0;							\
}									\
									\
static const struct ipm_driver_api vipm_nrf_##_idx##_driver_api = {	\
	.send = vipm_nrf_##_idx##_send,					\
	.register_callback = vipm_nrf_##_idx##_register_callback,	\
	.max_data_size_get = vipm_nrf_max_data_size_get,		\
	.max_id_val_get = vipm_nrf_max_id_val_get,			\
	.set_enabled = vipm_nrf_##_idx##_set_enabled			\
};									\
									\
DEVICE_DEFINE(vipm_nrf_##_idx, "IPM_"#_idx,				\
		    vipm_nrf_init, NULL, NULL, NULL,			\
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
		    &vipm_nrf_##_idx##_driver_api)

#define VIPM_DEVICE(_idx, _)						\
	IF_ENABLED(CONFIG_IPM_MSG_CH_##_idx##_ENABLE, (VIPM_DEVICE_1(_idx)))

LISTIFY(NRFX_IPC_ID_MAX_VALUE, VIPM_DEVICE, (;), _);

#endif

static void gipm_init(void)
{
	/* Init IPC */
#if defined(CONFIG_IPM_NRF_SINGLE_INSTANCE)
	nrfx_ipc_init(0, nrfx_ipc_handler, (void *)&nrfx_ipm_data);
#else
	nrfx_ipc_init(0, vipm_dispatcher, (void *)&nrfx_ipm_data);
#endif
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    nrfx_isr, nrfx_ipc_irq_handler, 0);

	/* Set up signals and channels */
	nrfx_ipc_config_load(&ipc_cfg);
}

static void gipm_send(uint32_t id)
{
	nrfx_ipc_signal(id);
}
