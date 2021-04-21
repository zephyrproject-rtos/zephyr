/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_EMUL_H_
#define ZEPHYR_INCLUDE_EMUL_H_

/**
 * @brief Emulators used to test drivers and higher-level code that uses them
 * @defgroup io_emulators Emulator interface
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct device;
struct emul;

/**
 * Structure uniquely identifying a device to be emulated
 *
 * Currently this uses the device node label, but that will go away by 2.5.
 */
struct emul_link_for_bus {
	const char *label;
};

/** List of emulators attached to a bus */
struct emul_list_for_bus {
	/** Identifiers for children of the node */
	const struct emul_link_for_bus *children;
	/** Number of children of the node */
	unsigned int num_children;
};

/**
 * Standard callback for emulator initialisation providing the initialiser
 * record and the device that calls the emulator functions.
 *
 * @param emul Emulator to init
 * @param parent Parent device that is using the emulator
 */
typedef int (*emul_init_t)(const struct emul *emul,
			   const struct device *parent);

/** An emulator instance */
struct emul {
	/** function used to initialise the emulator state */
	emul_init_t init;
	/** handle to the device for which this provides low-level emulation */
	const char *dev_label;
	/** Emulator-specific configuration data */
	const void *cfg;
};

/**
 * Emulators are aggregated into an array at link time, from which emulating
 * devices can find the emulators that they are to use.
 */
extern const struct emul __emul_list_start[];
extern const struct emul __emul_list_end[];

/* Use the devicetree node identifier as a unique name. */
#define EMUL_REG_NAME(node_id) (_CONCAT(__emulreg_, node_id))

/**
 * Define a new emulator
 *
 * This adds a new struct emul to the linker list of emulations. This is
 * typically used in your emulator's DT_INST_FOREACH_STATUS_OKAY() clause.
 *
 * @param init_ptr function to call to initialise the emulator (see emul_init
 *	typedef)
 * @param node_id Node ID of the driver to emulate (e.g. DT_DRV_INST(n))
 * @param cfg_ptr emulator-specific configuration data
 */
#define EMUL_DEFINE(init_ptr, node_id, cfg_ptr)			\
	static struct emul EMUL_REG_NAME(node_id)		\
	__attribute__((__section__(".emulators"))) __used = {	\
		.init = (init_ptr),				\
		.dev_label = DT_LABEL(node_id),			\
		.cfg = (cfg_ptr),				\
	};

/**
 * Set up a list of emulators
 *
 * @param dev Device the emulators are attached to (e.g. an I2C controller)
 * @param list List of devices to set up
 * @return 0 if OK
 * @return negative value on error
 */
int emul_init_for_bus_from_list(const struct device *dev,
				const struct emul_list_for_bus *list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_EMUL_H_ */
