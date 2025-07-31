/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <fsl_device_registers.h>

LOG_MODULE_REGISTER(puf_lpc54s018, CONFIG_ENTROPY_LOG_LEVEL);

/* PUF peripheral base from baremetal */
#define PUF_BASE 0x4003B000U
#define PUF ((PUF_Type *)PUF_BASE)

/* PUF IRQ from baremetal */
#define PUF_IRQ PUF_IRQn
#define PUF_IRQ_PRIORITY 3

/* PUF register structure based on LPC54S018M.h */
typedef struct {
	__IO uint32_t CTRL;          /* Control register */
	__IO uint32_t KEYINDEX;      /* Key index register */
	__IO uint32_t KEYSIZE;       /* Key size register */
	__I  uint32_t STAT;          /* Status register */
	__I  uint32_t ALLOW;         /* Allow register */
	__O  uint32_t KEYINPUT;      /* Key input register */
	__O  uint32_t CODEINPUT;     /* Code input register */
	__I  uint32_t KEYOUTPUT;     /* Key output register */
	__I  uint32_t CODEOUTPUT;    /* Code output register */
	__I  uint32_t KEYMASK[4];    /* Key mask registers */
	__IO uint32_t IDXBLK[2];     /* Index block registers */
	__IO uint32_t SHIFT;         /* Shift register */
	__IO uint32_t INTEN;         /* Interrupt enable */
	__IO uint32_t INTSTAT;       /* Interrupt status */
	__IO uint32_t PWRCTRL;       /* Power control */
	__IO uint32_t CFG;           /* Configuration */
} PUF_Type;

/* PUF Control bits */
#define PUF_CTRL_ENROLL      (1U << 0)
#define PUF_CTRL_START       (1U << 1)
#define PUF_CTRL_GENERATEKEY (1U << 2)
#define PUF_CTRL_SETKEY      (1U << 3)
#define PUF_CTRL_GETKEY      (1U << 4)

/* PUF Status bits */
#define PUF_STAT_BUSY        (1U << 0)
#define PUF_STAT_SUCCESS     (1U << 1)
#define PUF_STAT_ERROR       (1U << 2)
#define PUF_STAT_KEYVALID    (1U << 3)
#define PUF_STAT_ENROLLED    (1U << 4)

/* PUF key sizes */
enum puf_key_size {
	PUF_KEY_SIZE_64 = 64,
	PUF_KEY_SIZE_128 = 128,
	PUF_KEY_SIZE_192 = 192,
	PUF_KEY_SIZE_256 = 256
};

struct puf_lpc54s018_data {
	uint32_t activation_code[192]; /* Maximum activation code size */
	bool enrolled;
	bool busy;
	struct k_sem sync_sem;
};

struct puf_lpc54s018_config {
	uint32_t base;
	void (*irq_config_func)(const struct device *dev);
};

static void puf_lpc54s018_isr(const struct device *dev)
{
	struct puf_lpc54s018_data *data = dev->data;
	uint32_t status = PUF->INTSTAT;

	/* Clear interrupts */
	PUF->INTSTAT = status;

	if (status & PUF_STAT_SUCCESS) {
		data->busy = false;
		k_sem_give(&data->sync_sem);
	} else if (status & PUF_STAT_ERROR) {
		LOG_ERR("PUF operation error");
		data->busy = false;
		k_sem_give(&data->sync_sem);
	}
}

static int puf_lpc54s018_enroll(const struct device *dev)
{
	struct puf_lpc54s018_data *data = dev->data;
	int ret = 0;

	if (data->enrolled) {
		LOG_WRN("PUF already enrolled");
		return -EALREADY;
	}

	/* Check if enrollment is blocked by OTP */
	struct secure_boot_config sb_config;
	if (secure_boot_read_config(&sb_config) == 0) {
		if (sb_config.otp_value & OTPC_BOOTROM_BLOCK_PUF_ENROLL_KEY_CODE_MASK) {
			LOG_ERR("PUF enrollment blocked by OTP");
			return -EACCES;
		}
	}

	LOG_INF("Starting PUF enrollment...");

	data->busy = true;
	k_sem_reset(&data->sync_sem);

	/* Start enrollment */
	PUF->CTRL = PUF_CTRL_ENROLL | PUF_CTRL_START;

	/* Wait for completion */
	ret = k_sem_take(&data->sync_sem, K_MSEC(1000));
	if (ret != 0) {
		LOG_ERR("PUF enrollment timeout");
		return -ETIMEDOUT;
	}

	/* Check result */
	if (PUF->STAT & PUF_STAT_ERROR) {
		LOG_ERR("PUF enrollment failed");
		return -EIO;
	}

	/* Store activation code */
	for (int i = 0; i < ARRAY_SIZE(data->activation_code); i++) {
		data->activation_code[i] = PUF->CODEOUTPUT;
	}

	data->enrolled = true;
	LOG_INF("PUF enrollment complete");

	return 0;
}

static int puf_lpc54s018_get_entropy(const struct device *dev,
				     uint8_t *buffer, uint16_t length)
{
	struct puf_lpc54s018_data *data = dev->data;
	int ret;

	if (!data->enrolled) {
		LOG_ERR("PUF not enrolled");
		return -EINVAL;
	}

	if (length > PUF_KEY_SIZE_256 / 8) {
		LOG_ERR("Requested length %u exceeds maximum", length);
		return -EINVAL;
	}

	data->busy = true;
	k_sem_reset(&data->sync_sem);

	/* Set key size */
	PUF->KEYSIZE = length * 8;

	/* Generate key */
	PUF->CTRL = PUF_CTRL_GENERATEKEY | PUF_CTRL_START;

	/* Wait for completion */
	ret = k_sem_take(&data->sync_sem, K_MSEC(100));
	if (ret != 0) {
		LOG_ERR("PUF key generation timeout");
		return -ETIMEDOUT;
	}

	/* Check result */
	if (PUF->STAT & PUF_STAT_ERROR) {
		LOG_ERR("PUF key generation failed");
		return -EIO;
	}

	/* Read key output */
	uint32_t *out32 = (uint32_t *)buffer;
	for (int i = 0; i < (length + 3) / 4; i++) {
		out32[i] = PUF->KEYOUTPUT;
	}

	return 0;
}

static int puf_lpc54s018_init(const struct device *dev)
{
	const struct puf_lpc54s018_config *config = dev->config;
	struct puf_lpc54s018_data *data = dev->data;

	LOG_INF("Initializing PUF");

	k_sem_init(&data->sync_sem, 0, 1);

	/* Enable PUF clock */
	/* TODO: Enable clock via clock control driver */

	/* Configure interrupts */
	config->irq_config_func(dev);

	/* Enable PUF interrupts */
	PUF->INTEN = PUF_STAT_SUCCESS | PUF_STAT_ERROR;

	/* Check if already enrolled */
	if (PUF->STAT & PUF_STAT_ENROLLED) {
		data->enrolled = true;
		LOG_INF("PUF already enrolled");
	} else {
		LOG_INF("PUF not enrolled - enrollment required");
	}

	LOG_INF("PUF initialized");

	return 0;
}

/* Generate and store key in hardware key slot */
int lpc_puf_generate_key(uint8_t key_index, uint8_t key_size)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct puf_lpc54s018_data *data = dev->data;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (!data->enrolled) {
		LOG_ERR("PUF not enrolled");
		return -EINVAL;
	}

	/* Validate key index (0-3 for user keys) */
	if (key_index > 3) {
		LOG_ERR("Invalid key index: %u", key_index);
		return -EINVAL;
	}

	/* Validate key size */
	if (key_size != 16 && key_size != 24 && key_size != 32) {
		LOG_ERR("Invalid key size: %u", key_size);
		return -EINVAL;
	}

	data->busy = true;
	k_sem_reset(&data->sync_sem);

	/* Set key parameters */
	PUF->KEYINDEX = key_index;
	PUF->KEYSIZE = key_size * 8;

	/* Generate and store key */
	PUF->CTRL = PUF_CTRL_GENERATEKEY | PUF_CTRL_SETKEY | PUF_CTRL_START;

	/* Wait for completion */
	ret = k_sem_take(&data->sync_sem, K_MSEC(500));
	if (ret != 0) {
		LOG_ERR("Key generation timeout");
		return -ETIMEDOUT;
	}

	if (PUF->STAT & PUF_STAT_ERROR) {
		LOG_ERR("Key generation failed");
		return -EIO;
	}

	LOG_INF("Generated key at index %u, size %u bytes", key_index, key_size);
	return 0;
}

/* Load key from hardware slot to AES engine */
int lpc_puf_get_key(uint8_t key_index)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct puf_lpc54s018_data *data = dev->data;
	int ret;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (!data->enrolled) {
		LOG_ERR("PUF not enrolled");
		return -EINVAL;
	}

	if (key_index > 3) {
		LOG_ERR("Invalid key index: %u", key_index);
		return -EINVAL;
	}

	data->busy = true;
	k_sem_reset(&data->sync_sem);

	/* Set key index */
	PUF->KEYINDEX = key_index;

	/* Get key (loads into AES) */
	PUF->CTRL = PUF_CTRL_GETKEY | PUF_CTRL_START;

	/* Wait for completion */
	ret = k_sem_take(&data->sync_sem, K_MSEC(100));
	if (ret != 0) {
		LOG_ERR("Get key timeout");
		return -ETIMEDOUT;
	}

	if (PUF->STAT & PUF_STAT_ERROR) {
		LOG_ERR("Get key failed");
		return -EIO;
	}

	LOG_DBG("Loaded key from index %u", key_index);
	return 0;
}

/* Public enrollment function */
int lpc_puf_enroll(void)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	return puf_lpc54s018_enroll(dev);
}

static void puf_lpc54s018_irq_config(const struct device *dev)
{
	IRQ_CONNECT(PUF_IRQ, PUF_IRQ_PRIORITY,
		    puf_lpc54s018_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(PUF_IRQ);
}

static const struct entropy_driver_api puf_lpc54s018_api = {
	.get_entropy = puf_lpc54s018_get_entropy,
};

static struct puf_lpc54s018_data puf_lpc54s018_data_0;

static const struct puf_lpc54s018_config puf_lpc54s018_config_0 = {
	.base = PUF_BASE,
	.irq_config_func = puf_lpc54s018_irq_config,
};

DEVICE_DT_INST_DEFINE(0, puf_lpc54s018_init, NULL,
		      &puf_lpc54s018_data_0, &puf_lpc54s018_config_0,
		      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,
		      &puf_lpc54s018_api);

/**
 * PUF enrollment command (for development/provisioning only)
 */
#ifdef CONFIG_LPC54S018_PUF_SHELL
#include <zephyr/shell/shell.h>

static int cmd_puf_enroll(const struct shell *shell, size_t argc, char **argv)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	int ret;

	shell_print(shell, "Starting PUF enrollment...");
	ret = puf_lpc54s018_enroll(dev);
	if (ret == 0) {
		shell_print(shell, "PUF enrollment successful");
	} else {
		shell_error(shell, "PUF enrollment failed: %d", ret);
	}

	return ret;
}

static int cmd_puf_status(const struct shell *shell, size_t argc, char **argv)
{
	struct puf_lpc54s018_data *data = puf_lpc54s018_data_0;
	uint32_t status = PUF->STAT;

	shell_print(shell, "PUF Status: 0x%08X", status);
	shell_print(shell, "  Enrolled: %s", (status & PUF_STAT_ENROLLED) ? "YES" : "NO");
	shell_print(shell, "  Busy: %s", (status & PUF_STAT_BUSY) ? "YES" : "NO");
	shell_print(shell, "  Key Valid: %s", (status & PUF_STAT_KEYVALID) ? "YES" : "NO");
	shell_print(shell, "  Driver Enrolled: %s", data->enrolled ? "YES" : "NO");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(puf_cmds,
	SHELL_CMD(enroll, NULL, "Enroll PUF (one-time operation)", cmd_puf_enroll),
	SHELL_CMD(status, NULL, "Show PUF status", cmd_puf_status),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(puf, &puf_cmds, "PUF commands", NULL);
#endif /* CONFIG_LPC54S018_PUF_SHELL */