/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/device.h>
#include <inttypes.h>

/*
 * I3C Bus Discovery Application
 *
 * This sample discovers all I3C and I2C devices on the bus and displays
 * their properties according to MIPI I3C v1.1.1 specification.
 *
 * Features:
 * - Dynamic Address Assignment (DAA) for I3C devices
 * - Static device discovery from devicetree
 * - Full device property reporting (PID, BCR, DCR, addresses, limits)
 */

/* Device tree alias for I3C controller */
#define I3C_CONTROLLER_NODE DT_ALIAS(i3c0)

/* Fallback to DT_NODELABEL if alias not defined */
#if !DT_NODE_EXISTS(I3C_CONTROLLER_NODE)
#undef I3C_CONTROLLER_NODE
#define I3C_CONTROLLER_NODE DT_NODELABEL(i3c0)
#endif

/* Maximum dynamically tracked devices configured for this sample */
#define MAX_I3C_DEVICES CONFIG_I3C_NUM_OF_DESC_MEM_SLABS
#define MAX_I2C_DEVICES CONFIG_I3C_I2C_NUM_OF_DESC_MEM_SLABS

/**
 * @brief Decode BCR device role per MIPI I3C v1.1.1 Section 5.1.2.7
 *
 * @param bcr Bus Characteristics Register value
 * @return String representation of device role
 */
static const char *bcr_device_role_str(uint8_t bcr)
{
	uint8_t role = I3C_BCR_DEVICE_ROLE(bcr);

	switch (role) {
	case I3C_BCR_DEVICE_ROLE_I3C_TARGET:
		return "Target";
	case I3C_BCR_DEVICE_ROLE_I3C_CONTROLLER_CAPABLE:
		return "Controller Capable";
	default:
		return "Unknown";
	}
}

/**
 * @brief Decode device type based on DCR per MIPI I3C v1.1.1 Section 5.1.2.8
 *
 * @param dcr Device Characteristics Register value
 * @return String description of device type
 */
static const char *dcr_device_type_str(uint8_t dcr)
{
	/* Common DCR values from MIPI I3C specification */
	switch (dcr) {
	case 0x00:
		return "Generic Device";
	case 0x01:
		return "HID (Human Interface Device)";
	case 0x02:
		return "Sensor";
	case 0x03:
		return "Actuator";
	case 0x04:
		return "Data Converter";
	case 0x05:
		return "Audio";
	case 0x06:
		return "Camera/Imaging";
	case 0x07:
		return "Power";
	case 0x08:
		return "Memory";
	case 0x09:
		return "Smart Card";
	case 0x0A:
		return "Security";
	case 0x0B:
		return "Communication";
	case 0x0C:
		return "Health";
	case 0x3E:
		return "I3C Bridge";
	case 0x3F:
		return "I3C Hub";
	default:
		if (dcr >= 0x40 && dcr <= 0x7F) {
			return "Vendor Defined";
		}
		return "Unknown";
	}
}

/**
 * @brief Read a direct CCC payload from a single I3C target
 *
 * @param desc Pointer to I3C target descriptor
 * @param ccc_id Direct CCC identifier
 * @param buf Receive buffer
 * @param len Receive buffer length
 *
 * @return Number of bytes received or negative errno
 */
static int i3c_read_direct_ccc(struct i3c_device_desc *desc, uint8_t ccc_id,
			       uint8_t *buf, size_t len)
{
	struct i3c_ccc_target_payload target = {
		.addr = desc->dynamic_addr,
		.rnw = 1U,
		.data = buf,
		.data_len = len,
	};
	struct i3c_ccc_payload ccc = {
		.ccc.id = ccc_id,
		.targets.payloads = &target,
		.targets.num_targets = 1,
	};
	int ret;

	ret = i3c_do_ccc(desc->bus, &ccc);
	if (ret < 0) {
		return ret;
	}

	return (int)target.data_len;
}

/**
 * @brief Refresh target identity fields using direct CCCs
 *
 * @param desc Pointer to I3C target descriptor
 * @param pid Refreshed PID output
 * @param bcr Refreshed BCR output
 * @param dcr Refreshed DCR output
 */
static void refresh_i3c_device_info(struct i3c_device_desc *desc,
				    uint64_t *pid, uint8_t *bcr,
				    uint8_t *dcr)
{
	uint8_t pid_buf[6];
	uint8_t bcr_buf[1];
	uint8_t dcr_buf[1];
	int ret;

	*pid = desc->pid;
	*bcr = desc->bcr;
	*dcr = desc->dcr;

	ret = i3c_read_direct_ccc(desc, I3C_CCC_GETPID, pid_buf,
				  sizeof(pid_buf));
	if (ret == 6) {
		*pid = ((uint64_t)pid_buf[0] << 40) |
			((uint64_t)pid_buf[1] << 32) |
			((uint64_t)pid_buf[2] << 24) |
			((uint64_t)pid_buf[3] << 16) |
			((uint64_t)pid_buf[4] << 8) |
			(uint64_t)pid_buf[5];
	}

	ret = i3c_read_direct_ccc(desc, I3C_CCC_GETBCR, bcr_buf,
				  sizeof(bcr_buf));
	if (ret == 1) {
		*bcr = bcr_buf[0];
	}

	ret = i3c_read_direct_ccc(desc, I3C_CCC_GETDCR, dcr_buf,
				  sizeof(dcr_buf));
	if (ret == 1) {
		*dcr = dcr_buf[0];
	}

	(void)i3c_device_adv_info_get(desc);
}

/**
 * @brief Print detailed information about an I3C device
 *
 * Displays PID, BCR, DCR, addresses, and data length limits
 * per MIPI I3C v1.1.1 specification.
 *
 * @param desc Pointer to I3C device descriptor
 * @param index Device index for display
 * @param pid Provisioned ID value to print
 * @param bcr Bus characteristics register value to print
 * @param dcr Device characteristics register value to print
 */
static void print_i3c_device_info(struct i3c_device_desc *desc, int index,
				  uint64_t pid, uint8_t bcr, uint8_t dcr)
{
	uint8_t vendor_id = (pid >> 32) & 0xFF;
	uint32_t part_id = (pid >> 16) & 0xFFFF;
	uint16_t instance_id = pid & 0xFFFF;

	printk("========================================\n");
	printk("I3C Device #%d:\n", index);
	printk("  Device Name: %s\n", desc->dev->name);
	printk("  Transport Mode: I3C\n");
	printk("----------------------------------------\n");
	printk("  PID: 0x%012" PRIx64 "\n", pid);
	printk("    - Vendor ID: 0x%02x (%u)\n", vendor_id, vendor_id);
	printk("    - Part ID: 0x%04x (%u)\n", part_id, part_id);
	printk("    - Instance ID: 0x%04x (%u)\n", instance_id, instance_id);
	printk("  Device Type: %s (DCR=0x%02x)\n",
	       dcr_device_type_str(dcr), dcr);
	printk("  BCR: 0x%02x\n", bcr);
	printk("    - Role: %s\n", bcr_device_role_str(bcr));
	printk("    - Max Data Speed Limit: %s\n",
	       (bcr & I3C_BCR_MAX_DATA_SPEED_LIMIT) ? "Yes" : "No");
	printk("    - IBI Request Capable: %s\n",
	       (bcr & I3C_BCR_IBI_REQUEST_CAPABLE) ? "Yes" : "No");
	printk("    - IBI Payload: %s\n",
	       (bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE) ? "Yes" : "No");
	printk("    - Offline Capable: %s\n",
	       (bcr & I3C_BCR_OFFLINE_CAPABLE) ? "Yes" : "No");
	printk("    - Virtual Target: %s\n",
	       (bcr & I3C_BCR_VIRTUAL_TARGET) ? "Yes" : "No");
	printk("    - Advanced Capabilities: %s\n",
	       (bcr & I3C_BCR_ADV_CAPABILITIES) ? "Yes" : "No");
	printk("  Addresses:\n");
	printk("    - Dynamic Address: 0x%02x (%u)\n",
	       desc->dynamic_addr, desc->dynamic_addr);
	if (desc->static_addr != 0) {
		printk("    - Static Address: 0x%02x (%u)\n",
		       desc->static_addr, desc->static_addr);
	} else {
		printk("    - Static Address: None\n");
	}
	printk("----------------------------------------\n");
	printk("  Data Length Limits:\n");
	printk("    - Max Read Length (MRL): %u bytes\n", desc->data_length.mrl);
	printk("    - Max Write Length (MWL): %u bytes\n", desc->data_length.mwl);
	if (bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE) {
		printk("    - Max IBI Payload: %u bytes\n", desc->data_length.max_ibi);
	}
	printk("========================================\n\n");
}

/**
 * @brief Print information about an I2C device on I3C bus
 *
 * Displays address and Legacy Virtual Register (LVR) information.
 *
 * @param desc Pointer to I2C device descriptor
 * @param index Device index for display
 */
static void print_i2c_device_info(struct i3c_i2c_device_desc *desc, int index)
{
	uint8_t i2c_idx = I3C_LVR_I2C_DEV_IDX(desc->lvr);
	const char *idx_str;

	switch (i2c_idx) {
	case I3C_LVR_I2C_DEV_IDX_0:
		idx_str = "50ns spike filter (Fast)";
		break;
	case I3C_LVR_I2C_DEV_IDX_1:
		idx_str = "No spike filter (FM+ capable)";
		break;
	case I3C_LVR_I2C_DEV_IDX_2:
		idx_str = "No spike filter (FM only)";
		break;
	default:
		idx_str = "Unknown";
		break;
	}

	printk("========================================\n");
	printk("I2C Device #%d:\n", index);
	printk("  Transport Mode: Legacy I2C\n");
	printk("  Address: 0x%02x (%u)\n", desc->addr, desc->addr);
	printk("  LVR: 0x%02x\n", desc->lvr);
	printk("    - Device Index: %s\n", idx_str);
	printk("    - I2C Mode: %s\n",
	       I3C_LVR_I2C_MODE(desc->lvr) == I3C_LVR_I2C_FM_PLUS_MODE ?
			"FM+" : "FM");
	printk("========================================\n\n");
}

/**
 * @brief Print devicetree-style summary of discovered devices
 *
 * Generates an overlay-style summary that can be saved for reference
 * or added to an actual devicetree overlay.
 *
 * @param dev Pointer to I3C controller device
 * @param i3c_count Number of I3C devices found
 * @param i2c_count Number of I2C devices found
 */
static void print_dts_summary(const struct device *dev, int i3c_count, int i2c_count)
{
	struct i3c_device_desc *i3c_desc;
	struct i3c_i2c_device_desc *i2c_desc;
	int idx;

	printk("\n/* Device Tree Overlay - Discovered I3C/I2C Devices */\n");
	printk("/* Auto-generated by I3C Bus Scan Sample */\n");
	printk("&%s {\n", dev->name);
	printk("\tstatus = \"okay\";\n\n");

	/* I3C devices */
	if (i3c_count > 0) {
		printk("\t/* I3C Devices (discovered via DAA) */\n");
		idx = 0;
		I3C_BUS_FOR_EACH_I3CDEV(dev, i3c_desc) {
			uint64_t pid = i3c_desc->pid;
			uint32_t part_id = (pid >> 16) & 0xFFFF;

			printk("\tsensor_%04x: sensor@%02x {\n",
				part_id, i3c_desc->dynamic_addr);
			printk("\t\tcompatible = \"vendor,device\";\n");
			printk("\t\treg = <0x%02x>;\n", i3c_desc->dynamic_addr);
			printk("\t\tpid = <0x%012llx>;\n", pid);
			printk("\t\tbcr = <0x%02x>;\n", i3c_desc->bcr);
			printk("\t\tdcr = <0x%02x>;\n", i3c_desc->dcr);
			if (i3c_desc->static_addr != 0) {
				printk("\t\tstatic-address = <0x%02x>;\n",
					i3c_desc->static_addr);
			}
			printk("\t};\n\n");
			idx++;
		}
	}

	/* I2C devices */
	if (i2c_count > 0) {
		printk("\t/* I2C Legacy Devices */\n");
		idx = 0;
		I3C_BUS_FOR_EACH_I2CDEV(dev, i2c_desc) {
			printk("\ti2c_device_%02x: i2c-device@%02x {\n",
				i2c_desc->addr, i2c_desc->addr);
			printk("\t\tcompatible = \"i2c-device\";\n");
			printk("\t\treg = <0x%02x>;\n", i2c_desc->addr);
			printk("\t\tlvr = <0x%02x>;\n", i2c_desc->lvr);
			printk("\t};\n\n");
			idx++;
		}
	}

	printk("};\n");
}

/**
 * @brief Scan I3C bus and display all discovered devices
 *
 * Iterates through attached I3C and I2C devices and prints their
 * information. Devices are discovered via DAA (Dynamic Address
 * Assignment) during bus initialization.
 *
 * @param dev Pointer to I3C controller device
 * @return 0 on success, negative errno on failure
 */
static int scan_i3c_bus(const struct device *dev)
{
	struct i3c_device_desc *i3c_desc;
	struct i3c_i2c_device_desc *i2c_desc;
	int i3c_count = 0;
	int i2c_count = 0;
	uint64_t pid;
	uint8_t bcr;
	uint8_t dcr;

	printk("\n\n*** I3C Bus Scan Starting ***\n\n");

	if (!device_is_ready(dev)) {
		printk("Error: I3C controller not ready\n");
		return -ENODEV;
	}

	/* Scan I3C devices */
	printk("--- Scanning I3C Devices ---\n");
	I3C_BUS_FOR_EACH_I3CDEV(dev, i3c_desc) {
		refresh_i3c_device_info(i3c_desc, &pid, &bcr, &dcr);
		i3c_count++;
		print_i3c_device_info(i3c_desc, i3c_count, pid, bcr, dcr);
	}
	if (i3c_count == 0) {
		printk("No I3C devices found on the bus.\n");
	}

	/* Scan I2C devices */
	printk("--- Scanning I2C/Legacy Devices ---\n");
	I3C_BUS_FOR_EACH_I2CDEV(dev, i2c_desc) {
		i2c_count++;
		print_i2c_device_info(i2c_desc, i2c_count);
	}
	if (i2c_count == 0) {
		printk("No I2C devices found on the bus.\n");
	}

	/* Summary */
	printk("\n*** Scan Complete ***\n");
	printk("Total I3C devices found: %d\n", i3c_count);
	printk("Total I2C devices found: %d\n", i2c_count);
	printk("Total devices: %d\n", i3c_count + i2c_count);

	/* DTS-style summary */
	print_dts_summary(dev, i3c_count, i2c_count);

	if (i3c_count == 0 && i2c_count == 0) {
		printk("\nNote: No devices found. Ensure:\n");
		printk("  1. I3C/I2C devices are connected to the bus\n");
		printk("  2. Devices have power and proper pull-ups\n");
		printk("  3. DAA (Dynamic Address Assignment) was performed\n");
		printk("  4. Devices are defined in devicetree or discovered via DAA\n");
		printk("  5. I2C devices need static definition in devicetree\n");
	}

	return 0;
}

/**
 * @brief Main entry point
 *
 * Gets the I3C controller from devicetree and scans all devices
 * that were discovered during bus initialization.
 *
 * Bus initialization (i3c_bus_init) runs automatically at driver init
 * and performs the full MIPI I3C discovery sequence:
 *   1. OD slow-speed first broadcast (disables I2C spike filters)
 *   2. RSTACT (optional, CONFIG_I3C_INIT_RSTACT)
 *   3. RSTDAA (reset all dynamic addresses)
 *   4. DISEC (disable events during DAA)
 *   5. SETDASA (for DT-known devices with static addresses)
 *   6. SETAASA (for DT-known devices that support it)
 *   7. ENTDAA (Dynamic Address Assignment for all remaining targets)
 *   8. ENEC (re-enable Hot-Join events)
 *
 * Both DT-known and unknown I3C targets are discovered and attached.
 * Unknown targets are allocated from CONFIG_I3C_NUM_OF_DESC_MEM_SLABS.
 *
 * @return 0 on success, negative errno on failure
 */
int main(void)
{
	const struct device *i3c_dev;
	int ret;

	/* Get I3C controller device */
	i3c_dev = DEVICE_DT_GET(I3C_CONTROLLER_NODE);

	if (!device_is_ready(i3c_dev)) {
		printk("Error: I3C device %s is not ready\n", i3c_dev->name);
		return -ENODEV;
	}

	printk("*** I3C Bus Discovery Sample ***\n");
	printk("Controller: %s\n", i3c_dev->name);
	printk("MIPI I3C Specification v1.1.1 Compliant\n");
	printk("Max descriptor pool: I3C=%d, I2C=%d\n",
		MAX_I3C_DEVICES, MAX_I2C_DEVICES);
	printk("Bus init already completed (DAA at driver init)\n");

	/* Perform comprehensive scan of all attached devices */
	ret = scan_i3c_bus(i3c_dev);
	if (ret < 0) {
		printk("Error: Scan failed: %d\n", ret);
		return ret;
	}

	return 0;
}
