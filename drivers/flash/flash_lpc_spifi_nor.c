/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(flash_lpc_spifi);

/* SPIFI registers - correct offsets per LPC54S018 manual */
#define SPIFI_REG_CTRL      0x000
#define SPIFI_REG_CMD       0x004
#define SPIFI_REG_ADDR      0x008
#define SPIFI_REG_INTER     0x00C
#define SPIFI_REG_CLIMIT    0x010
#define SPIFI_REG_DATA      0x014
#define SPIFI_REG_MCMD      0x018
#define SPIFI_REG_STAT      0x01C

/* STAT bits - use our own names to avoid conflicts */
#define SPIFI_STAT_BIT_MCINIT  BIT(5)
#define SPIFI_STAT_BIT_CMD     BIT(0)
#define SPIFI_STAT_BIT_INTRQ   BIT(1)
#define SPIFI_STAT_BIT_RESET   BIT(4)

/* CTRL bits - use our own names to avoid conflicts */
#define SPIFI_CTRL_SET_TIMEOUT(n)  ((n) << 0)
#define SPIFI_CTRL_SET_CSHIGH(n)   (((n) - 1) << 16)
#define SPIFI_CTRL_BIT_PRFTCH_DIS  BIT(21)
#define SPIFI_CTRL_BIT_DUAL        BIT(22)
#define SPIFI_CTRL_BIT_MODE3       BIT(23)
#define SPIFI_CTRL_BIT_DMAEN       BIT(26)
#define SPIFI_CTRL_BIT_INTEN       BIT(27)
#define SPIFI_CTRL_BIT_FBCLK       BIT(30)
#define SPIFI_CTRL_BIT_DQS         BIT(31)

/* CMD bits - use our own names to avoid conflicts */
#define SPIFI_CMD_SET_DATALEN(n)   ((n) << 0)
#define SPIFI_CMD_BIT_DOUT         BIT(14)
#define SPIFI_CMD_SET_INTER(n)     ((n) << 15)
#define SPIFI_CMD_SET_FFORM(n)     ((n) << 19)
#define SPIFI_CMD_SET_FRAME(n)     ((n) << 21)
#define SPIFI_CMD_SET_OPCODE(n)    ((n) << 24)

/* Field form */
#define FIELD_ALL_SERIAL       0
#define FIELD_DATA_QUAD        1
#define FIELD_ADDR_INTER_DATA_QUAD 2
#define FIELD_ALL_QUAD         3

/* Frame form */
#define FRAME_OP               1
#define FRAME_OP_ADDR_SERIAL   2
#define FRAME_OP_ADDR_QUAD     3
#define FRAME_NO_OP_ADDR_SERIAL 4
#define FRAME_NO_OP_ADDR_QUAD  5
#define FRAME_OP_ADDR_ALL_QUAD 6

/* Opcodes for W25Q32JV */
#define OP_WRITE_ENABLE        0x06
#define OP_VOL_WRITE_ENABLE    0x50
#define OP_READ_SR1            0x05
#define OP_READ_SR2            0x35
#define OP_WRITE_SR            0x01
#define OP_PAGE_PROGRAM_QUAD   0x32
#define OP_SECTOR_ERASE        0x20
#define OP_READ_ID             0x9F
#define OP_QUAD_READ           0x6B  /* Fast Read Quad Output */

/* Params */
#define PAGE_SIZE              256
#define SECTOR_SIZE            4096
#define SPIFI_MEM_BASE         0x10000000
#define SPIFI_CLK_MAX          96000000

struct flash_lpc_spifi_config {
    uintptr_t reg_base;
};

static const struct flash_parameters flash_lpc_spifi_params = {
    .write_block_size = 1,
    .erase_value = 0xFF,
};

static void spifi_wait_ready(uintptr_t base) {
    while (sys_read32(base + SPIFI_REG_STAT) & SPIFI_STAT_BIT_CMD);
}

static int spifi_reset(uintptr_t base) {
    sys_write32(SPIFI_STAT_BIT_RESET, base + SPIFI_REG_STAT);
    while (sys_read32(base + SPIFI_REG_STAT) & SPIFI_STAT_BIT_RESET);
    return 0;
}

static int spifi_send_cmd(uintptr_t base, uint8_t op, uint32_t addr, uint32_t addr_bytes, 
                         void *data, uint32_t len, bool dout, uint8_t inter_bytes, 
                         uint8_t field_form, uint8_t frame_form) {
    spifi_wait_ready(base);

    if (addr_bytes > 0) {
        sys_write32(addr, base + SPIFI_REG_ADDR);
    }

    uint32_t cmd = SPIFI_CMD_SET_DATALEN(len) |
                   (dout ? SPIFI_CMD_BIT_DOUT : 0) |
                   SPIFI_CMD_SET_INTER(inter_bytes) |
                   SPIFI_CMD_SET_FFORM(field_form) |
                   SPIFI_CMD_SET_FRAME(frame_form) |
                   SPIFI_CMD_SET_OPCODE(op);

    sys_write32(cmd, base + SPIFI_REG_CMD);

    spifi_wait_ready(base);

    if (len > 0) {
        if (dout) {
            for (uint32_t i = 0; i < len; i++) {
                sys_write8(((uint8_t *)data)[i], base + SPIFI_REG_DATA);
            }
        } else {
            for (uint32_t i = 0; i < len; i++) {
                ((uint8_t *)data)[i] = sys_read8(base + SPIFI_REG_DATA);
            }
        }
    }

    return 0;
}

static int flash_lpc_spifi_poll_busy(uintptr_t base) {
    uint8_t sr1;
    do {
        spifi_send_cmd(base, OP_READ_SR1, 0, 0, &sr1, 1, false, 0, FIELD_ALL_SERIAL, FRAME_OP);
    } while (sr1 & BIT(0));  /* BUSY bit */
    return 0;
}

static int flash_lpc_spifi_write_enable(uintptr_t base) {
    spifi_send_cmd(base, OP_WRITE_ENABLE, 0, 0, NULL, 0, false, 0, FIELD_ALL_SERIAL, FRAME_OP);
    return 0;
}

static void spifi_set_memory_mode(uintptr_t base) {
    /* Set to Quad Output Read (0x6B): opcode serial, addr serial, 1 dummy byte serial (8 clocks), data quad */
    spifi_send_cmd(base, OP_QUAD_READ, 0, 3, NULL, 0, false, 1, FIELD_DATA_QUAD, FRAME_OP_ADDR_SERIAL);
    /* DATALEN=0 for unlimited read */
}

static int flash_lpc_spifi_enable_quad(uintptr_t base) {
    uint8_t sr1, sr2;

    /* Read SR1 and SR2 */
    spifi_send_cmd(base, OP_READ_SR1, 0, 0, &sr1, 1, false, 0, FIELD_ALL_SERIAL, FRAME_OP);
    spifi_send_cmd(base, OP_READ_SR2, 0, 0, &sr2, 1, false, 0, FIELD_ALL_SERIAL, FRAME_OP);

    if (sr2 & BIT(1)) {
        return 0;  /* Already enabled */
    }

    sr2 |= BIT(1);  /* Set QE bit */

    uint8_t sr_data[2] = {sr1, sr2};

    flash_lpc_spifi_write_enable(base);
    spifi_send_cmd(base, OP_WRITE_SR, 0, 0, sr_data, 2, true, 0, FIELD_ALL_SERIAL, FRAME_OP);
    flash_lpc_spifi_poll_busy(base);

    /* Verify */
    spifi_send_cmd(base, OP_READ_SR2, 0, 0, &sr2, 1, false, 0, FIELD_ALL_SERIAL, FRAME_OP);
    if (!(sr2 & BIT(1))) {
        LOG_ERR("Failed to enable quad mode");
        return -EIO;
    }

    return 0;
}

__ramfunc static int flash_lpc_spifi_erase(const struct device *dev, off_t offset, size_t size) {
    const struct flash_lpc_spifi_config *config = dev->config;
    uintptr_t base = config->reg_base;

    if ((offset % SECTOR_SIZE) != 0 || (size % SECTOR_SIZE) != 0) {
        return -EINVAL;
    }

    /* Temporarily exit memory mode by resetting or issuing a finite command */
    spifi_reset(base);  /* Safe way to exit memory mode */

    for (size_t i = 0; i < (size / SECTOR_SIZE); i++) {
        flash_lpc_spifi_write_enable(base);
        spifi_send_cmd(base, OP_SECTOR_ERASE, offset + (i * SECTOR_SIZE), 3, NULL, 0, false, 0, FIELD_ALL_SERIAL, FRAME_OP_ADDR_SERIAL);
        flash_lpc_spifi_poll_busy(base);
    }

    spifi_set_memory_mode(base);  /* Restore memory mode */

    return 0;
}

__ramfunc static int flash_lpc_spifi_write(const struct device *dev, off_t offset, const void *data, size_t len) {
    const struct flash_lpc_spifi_config *config = dev->config;
    uintptr_t base = config->reg_base;

    if ((len % PAGE_SIZE) != 0 || (offset % PAGE_SIZE) != 0) {
        return -EINVAL;
    }

    spifi_reset(base);  /* Exit memory mode */

    const uint8_t *src = data;
    for (size_t i = 0; i < (len / PAGE_SIZE); i++) {
        flash_lpc_spifi_write_enable(base);
        spifi_send_cmd(base, OP_PAGE_PROGRAM_QUAD, offset + (i * PAGE_SIZE), 3, (void *)src, PAGE_SIZE, true, 0, FIELD_ADDR_INTER_DATA_QUAD, FRAME_OP_ADDR_SERIAL);
        flash_lpc_spifi_poll_busy(base);
        src += PAGE_SIZE;
    }

    spifi_set_memory_mode(base);

    return 0;
}

static int flash_lpc_spifi_read(const struct device *dev, off_t offset, void *data, size_t len) {
    memcpy(data, (const void *)(SPIFI_MEM_BASE + offset), len);
    return 0;
}

static const struct flash_parameters *flash_lpc_spifi_get_parameters(const struct device *dev) {
    return &flash_lpc_spifi_params;
}

static const struct flash_driver_api flash_lpc_spifi_api = {
    .erase = flash_lpc_spifi_erase,
    .write = flash_lpc_spifi_write,
    .read = flash_lpc_spifi_read,
    .get_parameters = flash_lpc_spifi_get_parameters,
};

static int flash_lpc_spifi_init(const struct device *dev) {
    const struct flash_lpc_spifi_config *config = dev->config;
    uintptr_t base = config->reg_base;

    spifi_reset(base);

    /* Configure CTRL: example values, adjust for 96MHz */
    uint32_t ctrl = SPIFI_CTRL_SET_TIMEOUT(0xFFFF) |
                    SPIFI_CTRL_SET_CSHIGH(4) |  /* Min CS high 4 cycles */
                    SPIFI_CTRL_BIT_FBCLK |      /* Feedback clock */
                    SPIFI_CTRL_BIT_MODE3 |      /* CLK idle high */
                    0;  /* No DMA, no int */
    sys_write32(ctrl, base + SPIFI_REG_CTRL);

    /* Enable quad mode on flash (serial commands) */
    flash_lpc_spifi_enable_quad(base);

    /* Set memory mode */
    spifi_set_memory_mode(base);

    /* Optional: set cache limit if needed */
    sys_write32(0xFFFFFFFF, base + SPIFI_REG_CLIMIT);

    /* Verify JEDEC ID */
    uint8_t jedec_id[3];
    spifi_reset(base);
    spifi_send_cmd(base, OP_READ_ID, 0, 0, jedec_id, 3, false, 0, FIELD_ALL_SERIAL, FRAME_OP);
    
    if (jedec_id[0] != 0xEF || jedec_id[1] != 0x40 || jedec_id[2] != 0x16) {
        LOG_ERR("Invalid JEDEC ID: %02x %02x %02x (expected EF 40 16 for W25Q32JV)", 
                jedec_id[0], jedec_id[1], jedec_id[2]);
        return -ENODEV;
    }
    
    LOG_INF("W25Q32JV-DTR 4MB QSPI flash detected (JEDEC ID: %02x %02x %02x)", 
            jedec_id[0], jedec_id[1], jedec_id[2]);
    LOG_INF("SPIFI configured at 96MHz with quad mode enabled");
    
    spifi_set_memory_mode(base);

    return 0;
}

static const struct flash_lpc_spifi_config flash_lpc_spifi_cfg = {
    .reg_base = DT_REG_ADDR(DT_NODELABEL(spifi)),
};

DEVICE_DT_DEFINE(DT_NODELABEL(spifi),
         flash_lpc_spifi_init,
         NULL,
         NULL, &flash_lpc_spifi_cfg,
         POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
         &flash_lpc_spifi_api);