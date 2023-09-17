/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief The ECC DBERR (Double Bit ERRor) and SBERR (Single Bit ERRor) signals from the ECC RAMs
 * inside the Intel SoC FPGA (Ex: Agilex5) HPS peripherals are all routed to the System Manager
 * which collects the data and combines them into a single scalar interrupt that is routed to the
 * GIC.
 */
#include <zephyr/kernel.h>
#include <zephyr/sip_svc/sip_svc.h>
#include <zephyr/drivers/sip_svc/sip_svc_agilex_smc.h>
#include <socfpga_system_manager.h>
#include "edac.h"
#include "hps_ecc.h"

#define DT_DRV_COMPAT intel_socfpga_hps_ecc

#define LOG_MODULE_NAME hps_ecc
#define LOG_LEVEL       CONFIG_HPS_ECC_LOG_LEVEL
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL);

/* Define HPS ECC wrapper instances register Offsets */
#define ECC_CTRL_OFST       (0x8)
#define ECC_CTRL_EN_MASK    BIT(0)
#define ECC_CTRL_INITA_MASK BIT(16)
#define ECC_CTRL_INITB_MASK BIT(24)

#define ECC_INITSTAT_OFST                (0x0C)
#define ECC_INITSTAT_INITCOMPLETEA_MASK  BIT(0)
#define ECC_INITSTAT_INITCOMPLETEB_MASK  BIT(8)
#define MEMORY_INIT_CHECK_MAX_RETRY_COUT (100)

#define ECC_ERRINTEN_OFST  (0x10)
#define ECC_ERRINTENS_OFST (0x14)
#define ECC_ERRINTENR_OFST (0x18)
#define ECC_SERRINTEN_MASK BIT(0)

#define ECC_INTMODE_OFST 0x1C
#define ECC_INTMODE_MASK BIT(0)

#define ECC_INTSTAT_OFST  (0x20)
#define ECC_SERRPENA_MASK BIT(0)
#define ECC_DERRPENA_MASK BIT(8)
#define ECC_ERRPENA_MASK  (ECC_SERRPENA_MASK | ECC_DERRPENA_MASK)
#define ECC_SERRPENB_MASK BIT(16)
#define ECC_DERRPENB_MASK BIT(24)
#define ECC_ERRPENB_MASK  (ECC_SERRPENB_MASK | ECC_DERRPENB_MASK)

#define ECC_INTTEST_OFST            (0x24)
#define ECC_MODSTAT_OFFSET          (0x28)
#define ECC_MODSTAT_RMW_DBERRA_MASK BIT(4)
#define ECC_MODSTAT_RMW_DBERRB_MASK BIT(5)
#define ECC_MODSTAT_RMW_DBERR_MASK  (ECC_MODSTAT_RMW_DBERRA_MASK | ECC_MODSTAT_RMW_DBERRB_MASK)
#define ECC_MODSTAT_RMW_SBERRA_MASK BIT(2)
#define ECC_MODSTAT_RMW_SBERRB_MASK BIT(3)
#define ECC_MODSTAT_RMW_SBERR_MASK  (ECC_MODSTAT_RMW_SBERRA_MASK | ECC_MODSTAT_RMW_SBERRB_MASK)
#define ECC_DBERRADDRA_OFFSET       (0x2C)
#define ECC_SBERRADDRA_OFFSET       (0x30)
#define ECC_DBERRADDRB_OFFSET       (0x34)
#define ECC_SBERRADDRB_OFFSET       (0x38)
#define ECC_SERRCNTREG_OFFSET       (0x3C)
#define ECC_ADDRESS_OFST            (0x40)
#define ECC_RDATA0_OFST             (0x44)
#define ECC_RDATA1_OFST             (0x48)
#define ECC_RDATA2_OFST             (0x4C)
#define ECC_RDATA3_OFST             (0x50)
#define ECC_WDATA0_OFST             (0x54)
#define ECC_WDATA1_OFST             (0x58)
#define ECC_WDATA2_OFST             (0x5C)
#define ECC_WDATA3_OFST             (0x60)
#define ECC_RECC0_OFST              (0x64)
#define ECC_RECC1_OFST              (0x68)
#define ECC_WECC0_OFST              (0x6C)
#define ECC_WECC1_OFST              (0x70)
#define ECC_DBYTECTRL_OFST          (0x74)
#define ECC_ACCCTRL_OFST            (0x78)
#define ECC_STARTACC_OFST           (0x7C)

#define ECC_SERRCNT_MAX_VAL (1u)
#define ECC_TSERRA          BIT(0)
#define ECC_TDERRA          BIT(8)
#define ECC_XACT_START      (0x10000)
#define ECC_WORD_WRITE      (0xFF)
#define ECC_WRITE_DOVR      (0x101)
#define ECC_WRITE_EDOVR     (0x103)
#define ECC_READ_EOVR       (0x2)
#define ECC_READ_EDOVR      (0x3)

#define SYSMNGR_ECC_INTMASK_SET            (0x94)
#define SYSMNGR_ECC_INTMASK_CLR            (0x98)
#define SYSMNGR_ECC_INTSTATUS_SBERR_OFFSET (0x9C)
#define SYSMNGR_ECC_INTSTATUS_DBERR_OFFSET (0xA0)

#define MAX_TIMEOUT_MSECS (1 * 1000UL)

enum smc_request {
	/* SMC request parameter a2 index*/
	SMC_REQUEST_A2_INDEX = 0x00,
	/* SMC request parameter a3 index */
	SMC_REQUEST_A3_INDEX = 0x01
};

/* SIP SVC response private data */
struct private_data_t {
	struct sip_svc_response response;
	struct k_sem smc_sem;
};

typedef void (*hps_ecc_config_irq_t)(const struct device *dev);
typedef void (*hps_ecc_enable_irq_t)(const struct device *dev, bool en);

struct ecc_block_data {
	DEVICE_MMIO_RAM;
	int sbe_count;
};

struct ecc_block_config {
	DEVICE_MMIO_ROM;
	mem_addr_t phy_addr;
	uint32_t reg_block_size;
	bool dual_port;
};

struct hps_ecc_config {
	hps_ecc_config_irq_t irq_config_fn;
	struct ecc_block_config ecc_blk_cfg[ECC_MODULE_MAX_INSTANCES];
};

/**
 * @brief System Manager ECC driver runtime data.
 *
 * @ecc_info_cb: Pointer to call back function. This call back function
 *               will be register by EDAC module. It will be invoked by
 *               system manager ecc driver When an ECC error interrupt occurs.
 * @cb_usr_data: Callback function user data pointer. It will be
 *               an argument when the callback function is invoked.
 */
struct hps_ecc_data {
	struct ecc_block_data ecc_blk_data[ECC_MODULE_MAX_INSTANCES];
	edac_callback_t ecc_info_cb;
	void *cb_usr_data;
	uint32_t hps_ecc_init_status;
};
char ecc_module_name[ECC_MODULE_MAX_INSTANCES][20] = {
	"Reserved", "OCRAM",    "USB0 RAM0", "USB1 RAM 0", "EMAC0 RX",  "EMAC0 TX",  "EMAC1 RX",
	"EMAC1 TX", "EMAC2 RX", "EMAC2 TX",  "DMA 0",      "USB1 RAM1", "USB1 RAM2", "NAND",
	"SDMMC A",  "SDMMC B",  " DDR 0",    "DDR 1",      "DMA 1"};

static uint32_t mailbox_client_token;
static struct sip_svc_controller *mailbox_smc_dev;

/**
 * @brief Initialize the SiP SVC client
 * It will get the controller and register the client.
 *
 * @return 0 on success or negative value on fail
 */
static int32_t hps_ecc_smc_init(void)
{
	mailbox_smc_dev = sip_svc_get_controller("smc");
	if (!mailbox_smc_dev) {
		LOG_ERR("Arm SiP service not found");
		return -ENODEV;
	}

	mailbox_client_token = sip_svc_register(mailbox_smc_dev, NULL);
	if (mailbox_client_token == SIP_SVC_ID_INVALID) {
		mailbox_smc_dev = NULL;
		LOG_ERR("Mailbox client register fail");
		return -EINVAL;
	}

	return 0;
}
/**
 * @brief Close the svc client
 *
 * @return 0 on success or negative value on fail
 */
static int32_t svc_client_close(void)
{
	int32_t err;
	uint32_t cmd_size = sizeof(uint32_t);
	struct sip_svc_request request;

	uint32_t *cmd_addr = (uint32_t *)k_malloc(cmd_size);

	if (!cmd_addr) {
		return -ENOMEM;
	}

	/* Fill the SiP SVC buffer with CANCEL request */
	*cmd_addr = MAILBOX_CANCEL_COMMAND;

	request.header = SIP_SVC_PROTO_HEADER(SIP_SVC_PROTO_CMD_ASYNC, 0);
	request.a0 = SMC_FUNC_ID_MAILBOX_SEND_COMMAND;
	request.a1 = 0;
	request.a2 = (uint64_t)cmd_addr;
	request.a3 = (uint64_t)cmd_size;
	request.a4 = 0;
	request.a5 = 0;
	request.a6 = 0;
	request.a7 = 0;
	request.resp_data_addr = (uint64_t)NULL;
	request.resp_data_size = 0;
	request.priv_data = NULL;

	err = sip_svc_close(mailbox_smc_dev, mailbox_client_token, &request);
	if (err) {
		k_free(cmd_addr);
		LOG_ERR("Mailbox client close fail (%d)", err);
	}

	return err;
}
/**
 * @brief Open SiP SVC client session
 *
 * @return 0 on success or negative value on failure
 */
static int32_t svc_client_open(void)
{
	if ((!mailbox_smc_dev) || (mailbox_client_token == 0)) {
		LOG_ERR("Mailbox client is not registered");
		return -ENODEV;
	}

	if (sip_svc_open(mailbox_smc_dev, mailbox_client_token, K_MSEC(MAX_TIMEOUT_MSECS))) {
		LOG_ERR("Mailbox client open fail");
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief Call back function which we receive when we send the data
 * based on the current stage it will collect the data
 *
 * @param[in] c_token Token id for our svc services
 * @param[in] response Buffer will contain the response
 *
 * @return void
 */
static void smc_callback(uint32_t c_token, struct sip_svc_response *response)
{
	if (response == NULL) {
		return;
	}

	struct private_data_t *private_data = (struct private_data_t *)response->priv_data;

	LOG_DBG("SiP SVC callback");

	private_data->response.header = response->header;
	private_data->response.a0 = response->a0;
	private_data->response.a1 = response->a1;
	private_data->response.a2 = response->a2;
	private_data->response.a3 = response->a3;
	private_data->response.resp_data_size = response->resp_data_size;

	k_sem_give(&(private_data->smc_sem));
}

/**
 * @brief Send the data to sip_svc service layer
 *	based on the cmd_type further data will be send to SDM using mailbox
 *
 * @param[in] cmd_type Command type (Mailbox or Non-Mailbox)
 * @param[in] function_identifier Function identifier for each command type
 * @param[in] cmd_request
 * @param[in] private_data
 *
 * @return 0 on success or negative value on fail
 */
static int32_t smc_send(uint32_t cmd_type, uint64_t function_identifier, uint32_t *cmd_request,
			struct private_data_t *private_data)
{
	int32_t trans_id = 0;
	struct sip_svc_request request;

	if (!mailbox_smc_dev) {
		LOG_ERR("Mailbox client is not registered");
		return -ENODEV;
	}

	/* Fill SiP SVC request buffer */
	request.header = SIP_SVC_PROTO_HEADER(cmd_type, 0);
	request.a0 = function_identifier;
	request.a1 = 0;
	request.a2 = cmd_request[SMC_REQUEST_A2_INDEX];
	request.a3 = cmd_request[SMC_REQUEST_A3_INDEX];
	request.a4 = 0;
	request.a5 = 0;
	request.a6 = 0;
	request.a7 = 0;
	request.resp_data_addr = 0;
	request.resp_data_size = 0;
	request.priv_data = (void *)private_data;

	/* Send SiP SVC request */
	trans_id = sip_svc_send(mailbox_smc_dev, mailbox_client_token, &request, smc_callback);

	if (trans_id == SIP_SVC_ID_INVALID) {
		LOG_ERR("SiP SVC send request fail");
		return -EBUSY;
	}

	return 0;
}

static void smc_reg_write32(const struct device *dev, uint32_t data, mem_addr_t reg_addr)
{
	uint32_t smc_cmd[2] = {0};
	int32_t ret = 0;
	struct private_data_t priv_data;

	/* Initialize the semaphore */
	k_sem_init(&(priv_data.smc_sem), 0, 1);
	smc_cmd[SMC_REQUEST_A2_INDEX] = reg_addr;
	smc_cmd[SMC_REQUEST_A3_INDEX] = data;
	ret = smc_send(SIP_SVC_PROTO_CMD_SYNC, SMC_FUNC_ID_REG_WRITE, smc_cmd, &priv_data);
	if (ret) {
		LOG_ERR("%s : Failed to send the smc register write command", dev->name);
	}
	/* Wait for the SiP SVC callback */
	k_sem_take(&(priv_data.smc_sem), K_FOREVER);
	if (priv_data.response.a0) {
		LOG_DBG("%s : register write failed Addr: 0x%x Data: %d", dev->name,
			(uint32_t)reg_addr, data);
	}
}
static uint32_t smc_reg_read32(const struct device *dev, mem_addr_t reg_addr)
{
	uint32_t smc_cmd[2] = {0};
	int32_t ret = 0;
	struct private_data_t priv_data;
	uint32_t reg_val;

	/* Initialize the semaphore */
	k_sem_init(&(priv_data.smc_sem), 0, 1);
	smc_cmd[SMC_REQUEST_A2_INDEX] = reg_addr;
	ret = smc_send(SIP_SVC_PROTO_CMD_SYNC, SMC_FUNC_ID_REG_READ, smc_cmd, &priv_data);
	if (ret) {
		LOG_ERR("%s : Failed to send the smc register read Command", dev->name);
		return ret;
	}
	/* Wait for the SiP SVC callback */
	k_sem_take(&(priv_data.smc_sem), K_FOREVER);
	if (priv_data.response.a0) {
		LOG_DBG("%s : register read failed Addr: 0x%x", dev->name, (uint32_t)reg_addr);
	}
	reg_val = priv_data.response.a2;
	return reg_val;
}
/**
 * @brief	All the peripheral RAM ECC initialization must have completed in ATF.
 *			This function is to check the ECC wrapper initialization requirements
 *			fulfilled.
 * @param dev Pointer to the device structure for the driver.
 * @param ecc_modules_id ECC module ID
 * @return 0 initialisation requirements met.
 *         -EBUSY ECC block initialisation timeout.
 */
static int hps_ecc_instance_init(const struct device *dev, uint32_t ecc_modules_id)
{
	const struct hps_ecc_config *const config = dev->config;
	struct hps_ecc_data *data = dev->data;
	const struct ecc_block_config *ecc_blk_cfg = &config->ecc_blk_cfg[ecc_modules_id];
	struct ecc_block_data *ecc_blk_data = &data->ecc_blk_data[ecc_modules_id];
	uint32_t retry_count = MEMORY_INIT_CHECK_MAX_RETRY_COUT;

	device_map(&ecc_blk_data->_mmio, ecc_blk_cfg->phy_addr, ecc_blk_cfg->reg_block_size,
		   K_MEM_CACHE_NONE);

	/* Disable ECC Instance*/
	smc_reg_write32(dev, ECC_SERRINTEN_MASK, (ecc_blk_cfg->phy_addr + ECC_ERRINTENR_OFST));
	smc_reg_write32(dev,
			smc_reg_read32(dev, ecc_blk_cfg->phy_addr + ECC_CTRL_OFST) &
				(~ECC_CTRL_EN_MASK),
			ecc_blk_cfg->phy_addr + ECC_CTRL_OFST);

	smc_reg_write32(dev,
			smc_reg_read32(dev, ecc_blk_cfg->phy_addr + ECC_CTRL_OFST) |
				(ECC_CTRL_INITA_MASK),
			ecc_blk_cfg->phy_addr + ECC_CTRL_OFST);
	/* Wait for hardware memory initialization has completed on PORTA. */
	/* while (sys_read32(ecc_blk_data->_mmio + ECC_INITSTAT_OFST) & */
	while (!(sys_read32(ecc_blk_data->_mmio + ECC_INITSTAT_OFST) &
		 (ECC_INITSTAT_INITCOMPLETEA_MASK))) {
		retry_count--;
		if (retry_count == 0) {
			LOG_ERR("%s : %s ECC memory initialization timedout on PORTA", dev->name,
				ecc_module_name[ecc_modules_id]);
			return -EBUSY;
		}
	}
	/* Clear any pending ECC interrupts on PORT A */
	sys_write32(ECC_ERRPENA_MASK, ecc_blk_data->_mmio + ECC_INTSTAT_OFST);

	if (ecc_blk_cfg->dual_port) {
		smc_reg_write32(dev,
				smc_reg_read32(dev, ecc_blk_cfg->phy_addr + ECC_CTRL_OFST) |
					(ECC_CTRL_INITB_MASK),
				ecc_blk_cfg->phy_addr + ECC_CTRL_OFST);
		/* Wait for hardware memory initialization has completed on PORT B. */
		retry_count = MEMORY_INIT_CHECK_MAX_RETRY_COUT;
		while (!(sys_read32(ecc_blk_data->_mmio + ECC_INITSTAT_OFST) &
			 (ECC_INITSTAT_INITCOMPLETEB_MASK))) {
			retry_count--;
			if (retry_count == 0) {
				LOG_ERR("%s : %s ECC memory initialization timedout on PORTB",
					dev->name, ecc_module_name[ecc_modules_id]);
				return -EBUSY;
			}
		}
		/* Clear any pending ECC interrupts on PORT B */
		sys_write32(ECC_ERRPENB_MASK, ecc_blk_data->_mmio + ECC_INTSTAT_OFST);
	}
	sys_write32(ECC_SERRCNT_MAX_VAL, ecc_blk_data->_mmio + ECC_SERRCNTREG_OFFSET);
	smc_reg_write32(dev,
			smc_reg_read32(dev, ecc_blk_cfg->phy_addr + ECC_INTMODE_OFST) |
				(ECC_INTMODE_MASK),
			ecc_blk_cfg->phy_addr + ECC_INTMODE_OFST);
	smc_reg_write32(dev,
			smc_reg_read32(dev, ecc_blk_cfg->phy_addr + ECC_CTRL_OFST) |
				(ECC_CTRL_EN_MASK),
			ecc_blk_cfg->phy_addr + ECC_CTRL_OFST);
	smc_reg_write32(dev, ECC_SERRINTEN_MASK, (ecc_blk_cfg->phy_addr + ECC_ERRINTENS_OFST));

	data->hps_ecc_init_status |= (1 << ecc_modules_id);

	LOG_DBG("%s : %s ECC initialization success, init status = 0x%x", dev->name,
		ecc_module_name[ecc_modules_id], data->hps_ecc_init_status);

	return 0;
}

/**
 * @brief system manager ecc init function. it will check all the ecc module initialization
 * requirements satisfied.
 *
 * @param dev Pointer to the device structure for the driver.
 */
static int hps_ecc_init(const struct device *dev)
{
	struct hps_ecc_data *data = dev->data;
	const struct hps_ecc_config *const config = dev->config;
	int ret = 0;

	ret = hps_ecc_smc_init();
	if (ret) {
		return ret;
	}

	/* Opening SIP SVC session */
	ret = svc_client_open();
	if (ret) {
		LOG_ERR("Client open Failed!");
		return ret;
	}

	/* Disable ECC interrupts*/
	smc_reg_write32(dev, SYSMNGR_ECC_MODULE_INSTANCES_MSK,
			SOCFPGA_SYSMGR_REG_BASE + SYSMNGR_ECC_INTMASK_SET);

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(ocram_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_OCRAM);
	if (ret) {
		LOG_ERR("%s : OCRAM ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(usb0_ram0_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_USB0_RAM0);
	if (ret) {
		LOG_ERR("%s : USB0 RAM0 ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(usb1_ram0_ecc), okay)
	/* usb1_rx_ecc block protects USB1 RAM0 */
	ret = hps_ecc_instance_init(dev, ECC_USB1_RAM0);
	if (ret) {
		LOG_ERR("%s : USB1 RAM0 ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac0_rx_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_EMAC0_RX);
	if (ret) {
		LOG_ERR("%s : EMAC0 RX FIFO ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac0_tx_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_EMAC0_TX);
	if (ret) {
		LOG_ERR("%s : EMAC0 TX FIFO ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac1_rx_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_EMAC1_RX);
	if (ret) {
		LOG_ERR("%s : EMAC1 RX FIFO ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac1_tx_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_EMAC1_TX);
	if (ret) {
		LOG_ERR("%s : EMAC1 TX FIFO ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac2_rx_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_EMAC2_RX);
	if (ret) {
		LOG_ERR("%s : EMAC2 RX FIFO ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac2_tx_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_EMAC2_TX);
	if (ret) {
		LOG_ERR("%s : EMAC2 TX FIFO ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(dma0_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_DMA0);
	if (ret) {
		LOG_ERR("%s : DMA0 ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(usb1_ram1_ecc), okay)
	/* usb1_tx_ecc block protects USB1 RAM1 */
	ret = hps_ecc_instance_init(dev, ECC_USB1_RAM1);
	if (ret) {
		LOG_ERR("%s : USB1 RAM1 ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(usb1_ram2_ecc), okay)
	/* usb1_cache_ecc block protects USB1 RAM2 */
	ret = hps_ecc_instance_init(dev, ECC_USB1_RAM2);
	if (ret) {
		LOG_ERR("%s : USB1 RAM2 ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(nand_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_NAND);
	if (ret) {
		LOG_ERR("%s : NAND ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(sdmmca_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_SDMMCA);
	if (ret) {
		LOG_ERR("%s : SDMMCA ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(sdmmcb_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_SDMMCB);
	if (ret) {
		LOG_ERR("%s : SDMMCB ECC not initialized or disabled", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(dma1_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_DMA1);
	if (ret) {
		LOG_ERR("%s : DMA1 ECC not initialized or disabled ", dev->name);
	}
#endif

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(dma1_ecc), okay)
	ret = hps_ecc_instance_init(dev, ECC_QSPI);
	if (ret) {
		LOG_ERR("%s : QSPI ECC not initialized or disabled ", dev->name);
	}
#endif

	/* Enable ECC interrupts in system Manager*/
	smc_reg_write32(dev, data->hps_ecc_init_status,
			SOCFPGA_SYSMGR_REG_BASE + SYSMNGR_ECC_INTMASK_CLR);
	/* Configure system manager ECC interrupt in GIC */
	(config->irq_config_fn)(dev);

	/* Ignoring the return value to return bridge reset status */
	if (svc_client_close()) {
		LOG_ERR("Unregistering & Closing failed");
	}

	return 0;
}
#if defined(CONFIG_EDAC_ERROR_INJECT)
/**
 * @brief To inject ecc error in a HPS peripheral RAM specified by ecc_modules_id
 *
 * @param dev Pointer to the device structure for the driver.
 * @param ecc_modules_id ECC module ID
 * @param error_type error type DBE/SBE
 * @return 0 Error injection is success
 *			-EINVAL in case of invalid error_type parameter
 *			-ENODEV in case of ECC is not initialised or disabled
 */
static int hps_ecc_instance_inject_ecc_err(const struct device *dev, uint32_t ecc_modules_id,
					   uint32_t error_type)
{
	struct hps_ecc_data *data = dev->data;
	struct ecc_block_data *ecc_blk_data = &data->ecc_blk_data[ecc_modules_id];

	if (data->hps_ecc_init_status & (1 << ecc_modules_id)) {

		switch (error_type) {
		case INJECT_DBE:
			/* trigger uncorrectable error on PORT A */
			sys_write32(ECC_TDERRA, ecc_blk_data->_mmio + ECC_INTTEST_OFST);
			LOG_DBG("%s : %s double bit ECC error injection success", dev->name,
				ecc_module_name[ecc_modules_id]);
			break;

		case INJECT_SBE:
			/* trigger correctable error on PORT A */
			sys_write16(ECC_TSERRA, ecc_blk_data->_mmio + ECC_INTTEST_OFST);
			LOG_DBG("%s : %s Single bit ECC error injection success", dev->name,
				ecc_module_name[ecc_modules_id]);
			break;

		default:
			LOG_DBG("%s : %s ECC error injection failed", dev->name,
				ecc_module_name[ecc_modules_id]);
			return -EINVAL;
		}

	} else {
		LOG_DBG("%s : %s ECC not initialized or disabled", dev->name,
			ecc_module_name[ecc_modules_id]);
		return -ENODEV;
	}

	return 0;
}
#endif /* CONFIG_EDAC_ERROR_INJECT */

/**
 * @brief to get the single bit error count
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param id HPS peripheral ECC module ID or EMIF interface ID
 * @return  >= 0 Get SB error count success and count value equals to return value
 *			-ENODEV in case of ECC is not initialised or disabled
 */
int hps_get_sbe_ecc_error_cnt(const struct device *dev, uint32_t ecc_modules_id)
{
	struct hps_ecc_data *data = dev->data;
	const struct ecc_block_data *ecc_blk_data = &data->ecc_blk_data[ecc_modules_id];

	if (!(data->hps_ecc_init_status & (1 << ecc_modules_id))) {
		LOG_DBG("%s : %s ECC not initialized or disabled", dev->name,
			ecc_module_name[ecc_modules_id]);
		return -ENODEV;
	} else {
		return ecc_blk_data->sbe_count;
	}
}

/**
 * @brief To process the single bit error occurred in a ECC modules specified by module ID
 *
 * @param dev Pointer to the device structure for the driver.
 * @param ecc_modules_id ECC module ID
 */
static void hps_ecc_instance_process_sberr(const struct device *dev, uint32_t ecc_modules_id)
{
	struct hps_ecc_data *data = dev->data;
	struct ecc_block_data *ecc_blk_data = &data->ecc_blk_data[ecc_modules_id];
	const struct hps_ecc_config *const config = dev->config;
	const struct ecc_block_config *ecc_blk_cfg = &config->ecc_blk_cfg[ecc_modules_id];
	uint32_t sberr_address;
	uint32_t ecc_int_status;

	ecc_int_status = sys_read32(ecc_blk_data->_mmio + ECC_INTSTAT_OFST);
	sys_write32(ECC_SERRPENA_MASK, (ecc_blk_data->_mmio + ECC_INTSTAT_OFST));

	/*
	 * Check if that a RMW access due to a subword access generated a SBERR on RAM PORT A
	 */
	if (ecc_int_status & ECC_SERRPENA_MASK) {
		/* read the recent single-bit error address on RAM PORT A */
		sberr_address = sys_read32(ecc_blk_data->_mmio + ECC_SBERRADDRA_OFFSET);
		LOG_DBG("%s : %s Single bit error on RAM PORTA address = 0x%x", dev->name,
			ecc_module_name[ecc_modules_id], sberr_address);

		ecc_blk_data->sbe_count++;
	}

	if (ecc_blk_cfg->dual_port) {
		/*
		 * Check if that a RMW access due to a subword access generated a SBERR on RAM PORTB
		 */
		if (ecc_int_status & ECC_SERRPENB_MASK) {
			sys_write32(ECC_SERRPENB_MASK, (ecc_blk_data->_mmio + ECC_INTSTAT_OFST));
			/* read the recent single-bit error address on RAM PORT B*/
			sberr_address = sys_read32(ecc_blk_data->_mmio + ECC_SBERRADDRB_OFFSET);
			LOG_DBG("%s : %s Single bit error RAM on PORTB address = 0x%x", dev->name,
				ecc_module_name[ecc_modules_id], sberr_address);
			ecc_blk_data->sbe_count++;
		}
	}
}

/**
 * @brief To process the double bit error occurred in a ECC modules specified by module ID
 *
 * @param dev Pointer to the device structure for the driver.
 * @param ecc_modules_id ECC module ID
 */
void hps_ecc_instance_process_dberr(const struct device *dev, uint32_t ecc_modules_id)
{
	struct hps_ecc_data *data = dev->data;
	struct ecc_block_data *ecc_blk_data = &data->ecc_blk_data[ecc_modules_id];
	const struct hps_ecc_config *const config = dev->config;
	const struct ecc_block_config *ecc_blk_cfg = &config->ecc_blk_cfg[ecc_modules_id];
	uint32_t dberr_address;
	uint32_t ecc_int_status;

	ecc_int_status = sys_read32(ecc_blk_data->_mmio + ECC_INTSTAT_OFST);
	sys_write32(ECC_DERRPENA_MASK, (ecc_blk_data->_mmio + ECC_INTSTAT_OFST));

	/*
	 * Check if that a RMW access due to a subword access generated a DBERR on RAM
	 * PORT A
	 */
	if (ecc_int_status & ECC_DERRPENA_MASK) {
		/* read the recent double-bit error address on RAM PORT A */
		dberr_address = sys_read32(ecc_blk_data->_mmio + ECC_DBERRADDRA_OFFSET);
		LOG_DBG("%s : %s Double bit error on RAM PORTA address = 0x%x", dev->name,
			ecc_module_name[ecc_modules_id], dberr_address);
	}

	if (ecc_blk_cfg->dual_port) {
		/*
		 * Check if that a RMW access due to a subword access generated a DBERR on RAM
		 * PORTB
		 */
		if (ecc_int_status & ECC_DERRPENB_MASK) {
			sys_write32(ECC_DERRPENB_MASK, (ecc_blk_data->_mmio + ECC_INTSTAT_OFST));
			/* read the recent double-bit error address on RAM PORT B*/
			dberr_address = sys_read32(ecc_blk_data->_mmio + ECC_DBERRADDRB_OFFSET);
			LOG_DBG("%s : %s Double bit error RAM on PORTB address = 0x%x", dev->name,
				ecc_module_name[ecc_modules_id], dberr_address);
		}
	}
}

/** @brief Function to set a call back function for reporting ECC error.
 *  this call back will be called from system manager ECC ISR if an ECC error occurs.
 *
 * @param dev Pointer to the device structure for the driver.
 * @param cb Callback function
 * @param user_data Pointer to user data.
 *
 * @return	0 if callback function set is success
 *			-EINVAL if invalid value passed for input parameter 'cb'.
 */
static int hps_set_ecc_error_cb(const struct device *dev, edac_callback_t cb, void *user_data)
{
	struct hps_ecc_data *data = dev->data;
	int ret = 0;

	if (cb != NULL) {
		data->ecc_info_cb = cb;
		data->cb_usr_data = user_data;
	} else {
		ret = -EINVAL;
	}
	return ret;
}

static void hps_ecc_sberr_isr(const struct device *dev)
{
	struct hps_ecc_data *data = dev->data;
	uint32_t sberr_status;
	bool dbe = false, sbe = false;

	LOG_DBG("%s : Global ECC error detected", dev->name);

	/* Read single Bit Error status */
	sberr_status = sys_read32(SOCFPGA_SYSMGR_REG_BASE + SYSMNGR_ECC_INTSTATUS_SBERR_OFFSET) &
		       SYSMNGR_ECC_MODULE_INSTANCES_MSK;
	if (sberr_status != 0) {
		LOG_ERR("%s : Single bit errors detected SBERR status = 0x%x ", dev->name,
			sberr_status);
		sbe = true;
		for (uint32_t i = 0; (i < ECC_MODULE_SYSMNGR_MAX_INSTANCES); i++) {
			if (sberr_status & BIT(i)) {
				/* For each ECC module of i, process its recent single-bit error */
				hps_ecc_instance_process_sberr(dev, i);
			}
		}
	}

	/* Report the single bit error status to EDAC module*/
	if (data->ecc_info_cb != NULL) {
		data->ecc_info_cb(dev, dbe, sbe, data->cb_usr_data);
	} else {
		LOG_DBG("%s : Invalid EDAC callback function", dev->name);
	}
}

void process_serror_for_hps_dbe(const struct device *dev)
{
	struct hps_ecc_data *data = dev->data;
	uint32_t dberr_status;
	bool dbe = false, sbe = false;

	/* Read Double Bit Error status */
	dberr_status = sys_read32(SOCFPGA_SYSMGR_REG_BASE + SYSMNGR_ECC_INTSTATUS_DBERR_OFFSET) &
		       SYSMNGR_ECC_MODULE_INSTANCES_MSK;

	if (dberr_status != 0) {
		LOG_ERR("%s : Double bit errors detected DBERR status = 0x%x ", dev->name,
			dberr_status);
		dbe = true;
		for (uint32_t i = 0; (i < ECC_MODULE_SYSMNGR_MAX_INSTANCES); i++) {
			if (dberr_status & BIT(i)) {
				/* For each ECC module of i, process its recent double bit error */
				hps_ecc_instance_process_dberr(dev, i);
			}
		}
	}

	/* Report the double bit error status to EDAC module*/
	if (data->ecc_info_cb != NULL) {
		data->ecc_info_cb(dev, dbe, sbe, data->cb_usr_data);
	} else {
		LOG_DBG("%s : Invalid EDAC callback function", dev->name);
	}
}

#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(qspi_ecc), okay)
static void hps_qspi_ecc_sberr_isr(const struct device *dev)
{
	struct hps_ecc_data *data = dev->data;

	hps_ecc_instance_process_sberr(dev, ECC_QSPI);

	/* Report the double and single bit error status to EDAC module*/
	if (data->ecc_info_cb != NULL) {
		data->ecc_info_cb(dev, false, true, data->cb_usr_data);
	} else {
		LOG_DBG("%s : Invalid EDAC callback function", dev->name);
	}
}

static void hps_qspi_ecc_dberr_isr(const struct device *dev)
{
	struct hps_ecc_data *data = dev->data;

	hps_ecc_instance_process_dberr(dev, ECC_QSPI);

	/* Report the double and single bit error status to EDAC module*/
	if (data->ecc_info_cb != NULL) {
		data->ecc_info_cb(dev, true, false, data->cb_usr_data);
	} else {
		LOG_DBG("%s : Invalid EDAC callback function", dev->name);
	}
}
#endif

/* System manager ECC interrupt configuration and enable functions */
static void hps_ecc_irq_config(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 0, irq), DT_INST_IRQ_BY_IDX(0, 0, priority),
		    hps_ecc_sberr_isr, DEVICE_DT_INST_GET(0), 0);
	LOG_DBG("%s : Configured HPS ECC global interrupt IRQ No: %d", dev->name,
		DT_INST_IRQ_BY_IDX(0, 0, irq));
	irq_enable(DT_INST_IRQ_BY_IDX(0, 0, irq));
	LOG_DBG("%s : Enabled HPS ECC golbal interrupt IRQ No: %d", dev->name,
		DT_INST_IRQ_BY_IDX(0, 0, irq));
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(qspi_ecc), okay)
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 1, irq), DT_INST_IRQ_BY_IDX(0, 1, priority),
		    hps_qspi_ecc_sberr_isr, DEVICE_DT_INST_GET(0), 0);
	LOG_DBG("%s : Configured QSPI ECC SBE interrupt IRQ No: %d", dev->name,
		DT_INST_IRQ_BY_IDX(0, 1, irq));
	irq_enable(DT_INST_IRQ_BY_IDX(0, 1, irq));
	LOG_DBG("%s : Enabled HPS QSPI SBE interrupt IRQ No: %d", dev->name,
		DT_INST_IRQ_BY_IDX(0, 1, irq));
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 2, irq), DT_INST_IRQ_BY_IDX(0, 2, priority),
		    hps_qspi_ecc_dberr_isr, DEVICE_DT_INST_GET(0), 0);
	LOG_DBG("%s : Configured HPS QSPI DBE interrupt IRQ No: %d", dev->name,
		DT_INST_IRQ_BY_IDX(0, 2, irq));
	irq_enable(DT_INST_IRQ_BY_IDX(0, 2, irq));
	LOG_DBG("%s : Enabled HPS QSPI DBE interrupt IRQ No: %d", dev->name,
		DT_INST_IRQ_BY_IDX(0, 2, irq));
#endif /* DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(ocram_ecc), okay) */
}

static const struct edac_ecc_driver_api hps_ecc_driver_api = {
#if defined(CONFIG_EDAC_ERROR_INJECT)
	.inject_ecc_error = hps_ecc_instance_inject_ecc_err,
#endif
	.set_ecc_error_cb = hps_set_ecc_error_cb,
	.get_sbe_ecc_err_cnt = hps_get_sbe_ecc_error_cnt,
};

static const struct hps_ecc_config hps_ecc_dev_config = {
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(ocram_ecc), okay)
	.ecc_blk_cfg[ECC_OCRAM].phy_addr = DT_REG_ADDR(DT_NODELABEL(ocram_ecc)),
	.ecc_blk_cfg[ECC_OCRAM].reg_block_size = DT_REG_SIZE(DT_NODELABEL(ocram_ecc)),
	.ecc_blk_cfg[ECC_OCRAM].dual_port = DT_PROP(DT_NODELABEL(ocram_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(usb0_ram0_ecc), okay)
	.ecc_blk_cfg[ECC_USB0_RAM0].phy_addr = DT_REG_ADDR(DT_NODELABEL(usb0_ram0_ecc)),
	.ecc_blk_cfg[ECC_USB0_RAM0].reg_block_size = DT_REG_SIZE(DT_NODELABEL(usb0_ram0_ecc)),
	.ecc_blk_cfg[ECC_USB0_RAM0].dual_port = DT_PROP(DT_NODELABEL(usb0_ram0_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(usb1_ram0_ecc), okay)
	.ecc_blk_cfg[ECC_USB1_RAM0].phy_addr = DT_REG_ADDR(DT_NODELABEL(usb1_ram0_ecc)),
	.ecc_blk_cfg[ECC_USB1_RAM0].reg_block_size = DT_REG_SIZE(DT_NODELABEL(usb1_ram0_ecc)),
	.ecc_blk_cfg[ECC_USB1_RAM0].dual_port = DT_PROP(DT_NODELABEL(usb1_ram0_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac0_rx_ecc), okay)
	.ecc_blk_cfg[ECC_EMAC0_RX].phy_addr = DT_REG_ADDR(DT_NODELABEL(emac0_rx_ecc)),
	.ecc_blk_cfg[ECC_EMAC0_RX].reg_block_size = DT_REG_SIZE(DT_NODELABEL(emac0_rx_ecc)),
	.ecc_blk_cfg[ECC_EMAC0_RX].dual_port = DT_PROP(DT_NODELABEL(emac0_rx_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac0_tx_ecc), okay)
	.ecc_blk_cfg[ECC_EMAC0_TX].phy_addr = DT_REG_ADDR(DT_NODELABEL(emac0_tx_ecc)),
	.ecc_blk_cfg[ECC_EMAC0_TX].reg_block_size = DT_REG_SIZE(DT_NODELABEL(emac0_tx_ecc)),
	.ecc_blk_cfg[ECC_EMAC0_TX].dual_port = DT_PROP(DT_NODELABEL(emac0_tx_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac1_rx_ecc), okay)
	.ecc_blk_cfg[ECC_EMAC1_RX].phy_addr = DT_REG_ADDR(DT_NODELABEL(emac1_rx_ecc)),
	.ecc_blk_cfg[ECC_EMAC1_RX].reg_block_size = DT_REG_SIZE(DT_NODELABEL(emac1_rx_ecc)),
	.ecc_blk_cfg[ECC_EMAC1_RX].dual_port = DT_PROP(DT_NODELABEL(emac1_rx_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac1_tx_ecc), okay)
	.ecc_blk_cfg[ECC_EMAC1_TX].phy_addr = DT_REG_ADDR(DT_NODELABEL(emac1_tx_ecc)),
	.ecc_blk_cfg[ECC_EMAC1_TX].reg_block_size = DT_REG_SIZE(DT_NODELABEL(emac1_tx_ecc)),
	.ecc_blk_cfg[ECC_EMAC1_TX].dual_port = DT_PROP(DT_NODELABEL(emac1_tx_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac2_rx_ecc), okay)
	.ecc_blk_cfg[ECC_EMAC2_RX].phy_addr = DT_REG_ADDR(DT_NODELABEL(emac2_rx_ecc)),
	.ecc_blk_cfg[ECC_EMAC2_RX].reg_block_size = DT_REG_SIZE(DT_NODELABEL(emac2_rx_ecc)),
	.ecc_blk_cfg[ECC_EMAC2_RX].dual_port = DT_PROP(DT_NODELABEL(emac2_rx_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(emac2_tx_ecc), okay)
	.ecc_blk_cfg[ECC_EMAC2_TX].phy_addr = DT_REG_ADDR(DT_NODELABEL(emac2_tx_ecc)),
	.ecc_blk_cfg[ECC_EMAC2_TX].reg_block_size = DT_REG_SIZE(DT_NODELABEL(emac2_tx_ecc)),
	.ecc_blk_cfg[ECC_EMAC2_TX].dual_port = DT_PROP(DT_NODELABEL(emac2_tx_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(dma0_ecc), okay)
	.ecc_blk_cfg[ECC_DMA0].phy_addr = DT_REG_ADDR(DT_NODELABEL(dma0_ecc)),
	.ecc_blk_cfg[ECC_DMA0].reg_block_size = DT_REG_SIZE(DT_NODELABEL(dma0_ecc)),
	.ecc_blk_cfg[ECC_DMA0].dual_port = DT_PROP(DT_NODELABEL(dma0_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(usb1_ram1_ecc), okay)
	.ecc_blk_cfg[ECC_USB1_RAM1].phy_addr = DT_REG_ADDR(DT_NODELABEL(usb1_ram1_ecc)),
	.ecc_blk_cfg[ECC_USB1_RAM1].reg_block_size = DT_REG_SIZE(DT_NODELABEL(usb1_ram1_ecc)),
	.ecc_blk_cfg[ECC_USB1_RAM1].dual_port = DT_PROP(DT_NODELABEL(usb1_ram1_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(usb1_ram2_ecc), okay)
	.ecc_blk_cfg[ECC_USB1_RAM2].phy_addr = DT_REG_ADDR(DT_NODELABEL(usb1_ram2_ecc)),
	.ecc_blk_cfg[ECC_USB1_RAM2].reg_block_size = DT_REG_SIZE(DT_NODELABEL(usb1_ram2_ecc)),
	.ecc_blk_cfg[ECC_USB1_RAM2].dual_port = DT_PROP(DT_NODELABEL(usb1_ram2_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(nand_ecc), okay)
	.ecc_blk_cfg[ECC_NAND].phy_addr = DT_REG_ADDR(DT_NODELABEL(nand_ecc)),
	.ecc_blk_cfg[ECC_NAND].reg_block_size = DT_REG_SIZE(DT_NODELABEL(nand_ecc)),
	.ecc_blk_cfg[ECC_NAND].dual_port = DT_PROP(DT_NODELABEL(nand_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(sdmmca_ecc), okay)
	.ecc_blk_cfg[ECC_SDMMCA].phy_addr = DT_REG_ADDR(DT_NODELABEL(sdmmca_ecc)),
	.ecc_blk_cfg[ECC_SDMMCA].reg_block_size = DT_REG_SIZE(DT_NODELABEL(sdmmca_ecc)),
	.ecc_blk_cfg[ECC_SDMMCA].dual_port = DT_PROP(DT_NODELABEL(sdmmca_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(sdmmcb_ecc), okay)
	.ecc_blk_cfg[ECC_SDMMCB].phy_addr = DT_REG_ADDR(DT_NODELABEL(sdmmcb_ecc)),
	.ecc_blk_cfg[ECC_SDMMCB].reg_block_size = DT_REG_SIZE(DT_NODELABEL(sdmmcb_ecc)),
	.ecc_blk_cfg[ECC_SDMMCB].dual_port = DT_PROP(DT_NODELABEL(sdmmcb_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(dma1_ecc), okay)
	.ecc_blk_cfg[ECC_DMA1].phy_addr = DT_REG_ADDR(DT_NODELABEL(dma1_ecc)),
	.ecc_blk_cfg[ECC_DMA1].reg_block_size = DT_REG_SIZE(DT_NODELABEL(dma1_ecc)),
	.ecc_blk_cfg[ECC_DMA1].dual_port = DT_PROP(DT_NODELABEL(dma1_ecc), dual_port),
#endif
#if DT_NODE_HAS_STATUS_INTERNAL(DT_NODELABEL(dma1_ecc), okay)
	.ecc_blk_cfg[ECC_QSPI].phy_addr = DT_REG_ADDR(DT_NODELABEL(qspi_ecc)),
	.ecc_blk_cfg[ECC_QSPI].reg_block_size = DT_REG_SIZE(DT_NODELABEL(qspi_ecc)),
	.ecc_blk_cfg[ECC_QSPI].dual_port = DT_PROP(DT_NODELABEL(qspi_ecc), dual_port),
#endif
	.irq_config_fn = hps_ecc_irq_config,
};

static struct hps_ecc_data hps_ecc_dev_data;

DEVICE_DT_DEFINE(DT_NODELABEL(hps_ecc), hps_ecc_init, NULL, &hps_ecc_dev_data, &hps_ecc_dev_config,
		 POST_KERNEL, CONFIG_HPS_ECC_INIT_PRIORITY, &hps_ecc_driver_api);
