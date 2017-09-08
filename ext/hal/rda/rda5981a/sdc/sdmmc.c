#include "global_macros.h"
#include "sdmmc_reg.h"
#include "sdmmc.h"
#include "hal_sdmmc.h"

/* zephyr */
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

#include "soc_cfg.h"

#define RDA5981A_PIN(_port, _pin) \
	((_port << 5) | _pin)

extern unsigned int AHBBusClock;
extern int printk(const char *fmt, ...);
extern void k_busy_wait(uint32_t usec_to_wait);
extern uint32_t _timer_cycle_get_32(void);

#define SDMMC_DEBUG	0

#if SDMMC_DEBUG
#define sdmmc_dbg(fmt,args...) \
	printk("DBG: %s " fmt "\r",__PRETTY_FUNCTION__,##args)
#else
#define sdmmc_dbg(fmt,args...)
#endif

#define sdmmc_err(fmt,args...) \
	printk("ERR: %s " fmt "\r",__PRETTY_FUNCTION__,##args)

/* Command 8 things: cf spec Vers.2 section 4.3.13 */
#define MCD_CMD8_CHECK_PATTERN	0xaa
#define MCD_CMD8_VOLTAGE_SEL	(0x1<<8)
#define MCD_OCR_HCS	(1<<30)
#define MCD_OCR_CCS_MASK	(1<<30)

#define WAIT_USEC (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)/(1000*1000)
#define WAIT_MSEC	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)/(1000)
#define WAIT_SECOND	CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

/* Timeouts for V1 */
#define MCD_CMD_TIMEOUT_V1	(2*WAIT_SECOND)
#define MCD_RESP_TIMEOUT_V1	(2*WAIT_SECOND)
#define MCD_TRAN_TIMEOUT_V1	(2*WAIT_SECOND)
#define MCD_READ_TIMEOUT_V1	(5* WAIT_SECOND)
#define MCD_WRITE_TIMEOUT_V1	(5*WAIT_SECOND)

/* Timeouts for V2 */
#define MCD_CMD_TIMEOUT_V2	(2*WAIT_SECOND)
#define MCD_RESP_TIMEOUT_V2	( 2*WAIT_SECOND)
#define MCD_TRAN_TIMEOUT_V2	(2*WAIT_SECOND)
#define MCD_READ_TIMEOUT_V2	(5*WAIT_SECOND)
#define MCD_WRITE_TIMEOUT_V2	(5*WAIT_SECOND)
/* the card is supposed to answer within 1s */
#define MCD_SDMMC_OCR_TIMEOUT	(1*WAIT_SECOND)

/* Spec Ver2 p96 */
#define MCD_SDMMC_OCR_VOLT_PROF_MASK	0xff8000

static uint32_t mcd_ocr_reg = MCD_SDMMC_OCR_VOLT_PROF_MASK;
/*rca address is high 16 bits, low 16 bits are stuff.You can set rca 0 or other digitals*/
static uint32_t mcd_rca_reg;
static uint32_t mcd_block_len = 512;
static bool mcd_card_is_sdhc = false;
static mcd_status_t mcd_status = MCD_STATUS_NOTOPEN_PRESENT;

#define MMC_MAX_CLK 20000000	/* 20 MHz */

/* Current in-progress transfer, if any. */
static struct hal_sdmmc_transfer mcd_sdmmc_trans = {
	.sys_mem_addr = 0,
	.sdcard_addr = 0,
	.block_num = 0,
	.block_size = 0,
	.direction = HAL_SDMMC_DIRECTION_WRITE,
};

static mcd_card_ver_t mcd_ver = MCD_CARD_V2;
static uint8_t hal_sdmmc_read, hal_sdmmc_datovr;
static struct mcd_csd_reg mcd_csd;
static volatile struct hwp_sdmmc *hwp_sdmmc = hwp_sdmmc1;

static inline void mdelay(uint32_t sleep_in_ms)
{
	k_busy_wait(1000 * (sleep_in_ms));
}

static inline void udelay(uint32_t sleep_in_us)
{
	k_busy_wait(sleep_in_us);
}

static uint32_t hal_get_uptime(void)
{
	return _timer_cycle_get_32();
}

static void hal_sdmmc_open(uint8_t clk_adj)
{
	hwp_sdmmc->sdmmc_int_mask = 0;
	hwp_sdmmc->sdmmc_mclk_adjust = clk_adj;
}

static void hal_sdmmc_close(void)
{
	hwp_sdmmc->sdmmc_config = 0;
}

static void hal_sdmmc_send_cmd(uint32_t cmd, uint32_t arg, bool suspend)
{
	uint32_t config_reg = 0;

	hwp_sdmmc->sdmmc_config = 0;

	switch (cmd) {
	case HAL_SDMMC_CMD_GO_IDLE_STATE:
		config_reg = SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_ALL_SEND_CID:
		config_reg =
		    SDMMC_RSP_SEL_R2 | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SEND_RELATIVE_ADDR:
		config_reg =
		    SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SEND_IF_COND:
		config_reg =
		    SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SET_DSR:
		config_reg = SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SELECT_CARD:
		config_reg =
		    SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SEND_CSD:
		config_reg =
		    SDMMC_RSP_SEL_R2 | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_STOP_TRANSMISSION:
		config_reg =
		    SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SEND_STATUS:
		config_reg =
		    SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SET_BLOCKLEN:
		config_reg =
		    SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_READ_SINGLE_BLOCK:
		config_reg = SDMMC_RD_WT_SEL_READ
		    | SDMMC_RD_WT_EN
		    | SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_READ_MULT_BLOCK:
		config_reg = SDMMC_S_M_SEL_MULTIPLE
		    | SDMMC_RD_WT_SEL_READ
		    | SDMMC_RD_WT_EN
		    | SDMMC_RSP_SEL_OTHER
		    | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD | SDMMC_AUTO_CMD_EN;
		break;

	case HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK:
		config_reg = SDMMC_RD_WT_SEL_WRITE
		    | SDMMC_RD_WT_EN
		    | SDMMC_RSP_SEL_OTHER
		    | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD | 0xA000;
		break;

	case HAL_SDMMC_CMD_WRITE_MULT_BLOCK:
		config_reg = SDMMC_S_M_SEL_MULTIPLE
		    | SDMMC_RD_WT_SEL_WRITE
		    | SDMMC_RD_WT_EN
		    | SDMMC_RSP_SEL_OTHER
		    | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD | SDMMC_AUTO_CMD_EN;
		break;

	case HAL_SDMMC_CMD_APP_CMD:
		config_reg =
		    SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SET_BUS_WIDTH:
	case HAL_SDMMC_CMD_SWITCH:
		config_reg =
		    SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SEND_NUM_WR_BLOCKS:
		config_reg = SDMMC_RD_WT_SEL_READ
		    | SDMMC_RD_WT_EN
		    | SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_SET_WR_BLK_COUNT:
		config_reg =
		    SDMMC_RSP_SEL_OTHER | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	case HAL_SDMMC_CMD_MMC_SEND_OP_COND:
	case HAL_SDMMC_CMD_SEND_OP_COND:
		config_reg =
		    SDMMC_RSP_SEL_R3 | SDMMC_RSP_EN | SDMMC_SDMMC_SENDCMD;
		break;

	default:
		break;
	}

	hwp_sdmmc->sdmmc_cmd_index = SDMMC_COMMAND(cmd);
	hwp_sdmmc->sdmmc_cmd_arg = SDMMC_ARGUMENT(arg);
	hwp_sdmmc->sdmmc_config = config_reg;
	sdmmc_dbg("cmd:%d, arg:0x%x, config_reg:0x%x, sdmmc_config:0x%x\n",
		  SDMMC_COMMAND(cmd), SDMMC_ARGUMENT(arg), config_reg,
		  hwp_sdmmc->sdmmc_config);
}

static bool hal_sdmmc_need_response(HAL_SDMMC_CMD_T cmd)
{
	switch (cmd) {
	case HAL_SDMMC_CMD_GO_IDLE_STATE:
	case HAL_SDMMC_CMD_SET_DSR:
	case HAL_SDMMC_CMD_STOP_TRANSMISSION:
		return false;
	case HAL_SDMMC_CMD_ALL_SEND_CID:
	case HAL_SDMMC_CMD_SEND_RELATIVE_ADDR:
	case HAL_SDMMC_CMD_SEND_IF_COND:
	case HAL_SDMMC_CMD_SELECT_CARD:
	case HAL_SDMMC_CMD_SEND_CSD:
	case HAL_SDMMC_CMD_SEND_STATUS:
	case HAL_SDMMC_CMD_SET_BLOCKLEN:
	case HAL_SDMMC_CMD_READ_SINGLE_BLOCK:
	case HAL_SDMMC_CMD_READ_MULT_BLOCK:
	case HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK:
	case HAL_SDMMC_CMD_WRITE_MULT_BLOCK:
	case HAL_SDMMC_CMD_APP_CMD:
	case HAL_SDMMC_CMD_SET_BUS_WIDTH:
	case HAL_SDMMC_CMD_SEND_NUM_WR_BLOCKS:
	case HAL_SDMMC_CMD_SET_WR_BLK_COUNT:
	case HAL_SDMMC_CMD_MMC_SEND_OP_COND:
	case HAL_SDMMC_CMD_SEND_OP_COND:
	case HAL_SDMMC_CMD_SWITCH:
		return true;

	default:
		return false;
	}
}

static bool hal_sdmmc_cmd_done(void)
{
	return (!(hwp_sdmmc->sdmmc_status & SDMMC_NOT_SDMMC_OVER)) ? true :
	    false;
}

static void hal_sdmmc_set_clk(uint32_t clock)
{
	uint32_t div;
	uint32_t abc = AHBBusClock;

	/* Update the divider takes care of the registers configuration */
	if (clock > MMC_MAX_CLK)
		clock = MMC_MAX_CLK;

	div = ((abc / clock) >> 1) - 1;

	/* mclk   div  mclk  @ sys_clk=26MHz
	 * 400K : 32   394Khz
	 * 6.5M :  1   6.5Mhz
	 */
	hwp_sdmmc->sdmmc_trans_speed = SDMMC_SDMMC_TRANS_SPEED(div);
	sdmmc_dbg("sdmmc_trans_speed: 0x%x\n", hwp_sdmmc->sdmmc_trans_speed);
}

static hal_sdmmc_op_status_t hal_sdmmc_get_op_status(void)
{
	return ((hal_sdmmc_op_status_t) hwp_sdmmc->sdmmc_status);
}

static void hal_sdmmc_get_resp(uint32_t cmd, uint32_t * arg, bool suspend)
{
	/* TODO Check in the spec for all the commands response types */
	switch (cmd) {
		/*
		 * If they require a response, it is cargoed
		 * with a 32 bit argument.
		 */
	case HAL_SDMMC_CMD_GO_IDLE_STATE:
	case HAL_SDMMC_CMD_SEND_RELATIVE_ADDR:
	case HAL_SDMMC_CMD_SEND_IF_COND:
	case HAL_SDMMC_CMD_SET_DSR:
	case HAL_SDMMC_CMD_SELECT_CARD:
	case HAL_SDMMC_CMD_STOP_TRANSMISSION:
	case HAL_SDMMC_CMD_SEND_STATUS:
	case HAL_SDMMC_CMD_SET_BLOCKLEN:
	case HAL_SDMMC_CMD_READ_SINGLE_BLOCK:
	case HAL_SDMMC_CMD_READ_MULT_BLOCK:
	case HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK:
	case HAL_SDMMC_CMD_WRITE_MULT_BLOCK:
	case HAL_SDMMC_CMD_APP_CMD:
	case HAL_SDMMC_CMD_SET_BUS_WIDTH:
	case HAL_SDMMC_CMD_SEND_NUM_WR_BLOCKS:
	case HAL_SDMMC_CMD_SET_WR_BLK_COUNT:
	case HAL_SDMMC_CMD_MMC_SEND_OP_COND:
	case HAL_SDMMC_CMD_SEND_OP_COND:
	case HAL_SDMMC_CMD_SWITCH:
		arg[0] = hwp_sdmmc->sdmmc_resp_arg3;
		arg[1] = 0;
		arg[2] = 0;
		arg[3] = 0;
		break;

		/* Those response arguments are 128 bits */
	case HAL_SDMMC_CMD_ALL_SEND_CID:
	case HAL_SDMMC_CMD_SEND_CSD:
		arg[0] = hwp_sdmmc->sdmmc_resp_arg0 << 1;
		arg[1] = hwp_sdmmc->sdmmc_resp_arg1;
		arg[2] = hwp_sdmmc->sdmmc_resp_arg2;
		arg[3] = hwp_sdmmc->sdmmc_resp_arg3;
		break;

	default:
		break;
	}
}

static void hal_sdmmc_enter_trans_mode(void)
{
	hwp_sdmmc->sdmmc_config |= SDMMC_RD_WT_EN;
}

static void hal_sdmmc_set_data_width(uint32_t width)
{
	switch (width) {
	case HAL_SDMMC_DATA_BUS_WIDTH_1:
		hwp_sdmmc->sdmmc_data_width = 1;
		break;

	case HAL_SDMMC_DATA_BUS_WIDTH_4:
		hwp_sdmmc->sdmmc_data_width = 4;
		break;

	default:
		break;
	}
}

static int hal_sdmmc_set_tans_info(struct hal_sdmmc_transfer *transfer)
{
	uint32_t length = 0;
	uint32_t length_exp = 0;

	length = transfer->block_size;

	/* The block size register */
	while (length != 1) {
		length >>= 1;
		length_exp++;
	}

	sdmmc_dbg("size=%d count num=%d\n", length_exp, transfer->block_num);

	/* Configure amount of data */
	hwp_sdmmc->sdmmc_block_cnt = SDMMC_SDMMC_BLOCK_CNT(transfer->block_num);
	hwp_sdmmc->sdmmc_block_size = SDMMC_SDMMC_BLOCK_SIZE(length_exp);

	/* Configure Bytes reordering */
	hwp_sdmmc->apbi_ctrl_sdmmc = SDMMC_SOFT_RST_L | SDMMC_L_ENDIAN(1);
	sdmmc_dbg("size=%d bytes\n",
		  transfer->block_num * transfer->block_size);

	/* Record Channel used. */
	if (transfer->direction == HAL_SDMMC_DIRECTION_READ
	    || transfer->direction == HAL_SDMMC_DIRECTION_WRITE)
		hal_sdmmc_read = 1;

	return 0;
}

static bool hal_sdmmc_trans_done(int type)
{
	/*
	 * The link is not full duplex. We check both the
	 * direction, but only one can be in progress at a time.
	 */

	if (hal_sdmmc_read > 0) {
		/* Operation in progress is a read. The EMMC module itself can know it has finished */
		if ((0 == hal_sdmmc_datovr)
		    && (hwp_sdmmc->sdmmc_int_status & SDMMC_DAT_OVER_INT)) {
			hal_sdmmc_datovr = 1;
		}

		if ((1 == hal_sdmmc_datovr) && hal_sdmmc_cmd_done()) {
			/* Transfer is over and clear the interrupt source. */
			hwp_sdmmc->sdmmc_int_clear = SDMMC_DAT_OVER_CL;

			hal_sdmmc_read = 0;
			hal_sdmmc_datovr = 0;

			/* Clear the FIFO. */
			hwp_sdmmc->apbi_ctrl_sdmmc = 0 | SDMMC_L_ENDIAN(1);
			hwp_sdmmc->apbi_ctrl_sdmmc =
			    SDMMC_SOFT_RST_L | SDMMC_L_ENDIAN(1);

			return true;
		}
	}

	/* there's still data running through a pipe (or no transfer in progress ...) */
	return false;
}

static void hal_sdmmc_stop_trans(struct hal_sdmmc_transfer *transfer)
{
	/* Configure amount of data */
	hwp_sdmmc->sdmmc_block_cnt = SDMMC_SDMMC_BLOCK_CNT(0);
	hwp_sdmmc->sdmmc_block_size = SDMMC_SDMMC_BLOCK_SIZE(0);

	hal_sdmmc_read = 0;

	/*  Put the FIFO in reset state. */
	hwp_sdmmc->apbi_ctrl_sdmmc = 0 | SDMMC_L_ENDIAN(1);
	hwp_sdmmc->apbi_ctrl_sdmmc = SDMMC_SOFT_RST_L | SDMMC_L_ENDIAN(1);
}

static bool hal_sdmmc_is_busy(void)
{
	if ((hwp_sdmmc->sdmmc_status &
	     (SDMMC_NOT_SDMMC_OVER | SDMMC_BUSY | SDMMC_DL_BUSY)) != 0) {
		/* SDMMc is busy doing something. */
		return true;
	} else {
		return false;
	}
}

/* Wait for a command to be done or timeouts */
static uint32_t mcd_sdmmc_wait_cmd_over(void)
{
	uint64_t start_time = hal_get_uptime();
	uint64_t time_out;

	time_out =
	    (MCD_CARD_V1 == mcd_ver) ? MCD_CMD_TIMEOUT_V1 : MCD_CMD_TIMEOUT_V2;

	while (hal_get_uptime() - start_time < time_out
	       && !hal_sdmmc_cmd_done()) ;

	if (!hal_sdmmc_cmd_done()) {
		sdmmc_err("timeout\n");
		return MCD_ERR_CARD_TIMEOUT;
	} else {
		return MCD_ERR_NO;
	}
}

static int mcd_sdmmc_wait_resp(void)
{
	hal_sdmmc_op_status_t status = hal_sdmmc_get_op_status();
	uint64_t start_time = hal_get_uptime();
	uint64_t rsp_time_out;

	rsp_time_out =
	    (MCD_CARD_V1 ==
	     mcd_ver) ? MCD_RESP_TIMEOUT_V1 : MCD_RESP_TIMEOUT_V2;

	while (hal_get_uptime() - start_time < rsp_time_out &&
	       status.fields.noresponse_received) {
		status = hal_sdmmc_get_op_status();
	}

	if (status.fields.noresponse_received) {
		sdmmc_err("No Response\n");
		return MCD_ERR_CARD_NO_RESPONSE;
	}

	if (status.fields.response_crc_error) {
		sdmmc_err("Response CRC\n");
		return MCD_ERR_CARD_RESPONSE_BAD_CRC;
	}

	return MCD_ERR_NO;
}

static int mcd_sdmmc_read_check_crc(void)
{
	hal_sdmmc_op_status_t operation_status;
	operation_status = hal_sdmmc_get_op_status();

	/* 0 means no CRC error during transmission */
	if (operation_status.fields.data_error != 0) {
		sdmmc_err("Bad CRC, 0x%08X\n", operation_status.reg);
		return MCD_ERR_CARD_RESPONSE_BAD_CRC;
	} else {
		return MCD_ERR_NO;
	}
}

static int mcd_sdmmc_send_cmd(int cmd, uint32_t arg, uint32_t * resp,
			      bool suspend)
{
	int err_status = MCD_ERR_NO;
	mcd_card_status_t card_status = { 0 };
	uint32_t cmd55_response[4] = { 0, 0, 0, 0 };

	if (cmd & HAL_SDMMC_ACMD_SEL) {
		/* This is an application specific command,
		 * we send the CMD55 first, response r1
		 */
		hal_sdmmc_send_cmd(HAL_SDMMC_CMD_APP_CMD, mcd_rca_reg, false);

		/* Wait for command over */
		if (MCD_ERR_CARD_TIMEOUT == mcd_sdmmc_wait_cmd_over()) {
			sdmmc_err("cmd55 timeout\n");
			return MCD_ERR_CARD_TIMEOUT;
		}
		/* Wait for response */
		if (hal_sdmmc_need_response(HAL_SDMMC_CMD_APP_CMD)) {
			err_status = mcd_sdmmc_wait_resp();
		}

		if (MCD_ERR_NO != err_status) {
			sdmmc_err("cmd55 err=%d\n", err_status);
			return err_status;
		}
		/* Fetch response */
		hal_sdmmc_get_resp(HAL_SDMMC_CMD_APP_CMD, cmd55_response,
				   false);

		card_status.reg = cmd55_response[0];
		if (HAL_SDMMC_CMD_SEND_OP_COND == cmd) {
			if (!(card_status.fields.app_cmd)) {
				sdmmc_err("cmd55(acmd41) status\n");
				return MCD_ERR;
			}
		} else {
			if (!(card_status.fields.ready_for_data) ||
			    !(card_status.fields.app_cmd)) {
				sdmmc_err("cmd55 status\n");
				return MCD_ERR;
			}
		}
	}
	/* Send proper command. If it was an ACMD, the CMD55 have just been sent. */
	hal_sdmmc_send_cmd(cmd, arg, suspend);

	/* Wait for command to be sent */
	err_status = mcd_sdmmc_wait_cmd_over();

	if (MCD_ERR_CARD_TIMEOUT == err_status) {
		sdmmc_err("%s send timeout\n",
			  (cmd & HAL_SDMMC_ACMD_SEL) ? "ACMD" : "CMD");
		return MCD_ERR_CARD_TIMEOUT;
	}
	/* Wait for response and get its argument */
	if (hal_sdmmc_need_response(cmd)) {
		err_status = mcd_sdmmc_wait_resp();
	}

	if (MCD_ERR_NO != err_status) {
		sdmmc_err("%s Response err=%d\n",
			  (cmd & HAL_SDMMC_ACMD_SEL) ? "ACMD" : "CMD",
			  err_status);
		return err_status;
	}
	/* Fetch response */
	hal_sdmmc_get_resp(cmd, resp, false);

	return MCD_ERR_NO;
}

#define MCD_CSD_VERSION_1       0
#define MCD_CSD_VERSION_2       1

static int mcd_sdmmc_init_csd(struct mcd_csd_reg *csd, uint32_t * csd_raw)
{
	/* CF SD spec version2, CSD version 1 */
	csd->csd_structure = (uint8_t) ((csd_raw[3] & (0x3 << 30)) >> 30);

	/* Byte 47 to 75 are different depending on the version
	 * of the CSD srtucture.
	 */
	csd->spec_vers = (uint8_t) ((csd_raw[3] & (0xf < 26)) >> 26);
	csd->taac = (uint8_t) ((csd_raw[3] & (0xff << 16)) >> 16);
	csd->nsac = (uint8_t) ((csd_raw[3] & (0xff << 8)) >> 8);
	csd->tran_speed = (uint8_t) (csd_raw[3] & 0xff);

	csd->ccc = (csd_raw[2] & (0xfff << 20)) >> 20;
	csd->read_bl_len = (uint8_t) ((csd_raw[2] & (0xf << 16)) >> 16);
	csd->read_bl_partial =
	    (((csd_raw[2] & (0x1 << 15)) >> 15) != 0) ? true : false;
	csd->write_blk_misalign =
	    (((csd_raw[2] & (0x1 << 14)) >> 14) != 0) ? true : false;
	csd->read_blk_misalign =
	    (((csd_raw[2] & (0x1 << 13)) >> 13) != 0U) ? true : false;
	csd->dsr_imp = (((csd_raw[2] & (0x1 << 12)) >> 12) != 0) ? true : false;

	if (csd->csd_structure == MCD_CSD_VERSION_1) {
		csd->c_size = (csd_raw[2] & 0x3ff) << 2;
		csd->c_size = csd->c_size | ((csd_raw[1] & (0x3 << 30)) >> 30);
		csd->vdd_r_curr_min =
		    (uint8_t) ((csd_raw[1] & (0x7 << 27)) >> 27);
		csd->vdd_r_curr_max =
		    (uint8_t) ((csd_raw[1] & (0x7 << 24)) >> 24);
		csd->vdd_w_curr_min =
		    (uint8_t) ((csd_raw[1] & (0x7 << 21)) >> 21);
		csd->vdd_w_curr_max =
		    (uint8_t) ((csd_raw[1] & (0x7 << 18)) >> 18);
		csd->c_size_mult = (uint8_t) ((csd_raw[1] & (0x7 << 15)) >> 15);

		/* Block number: cf Spec Version 2 page 103 (116). */
		csd->block_number = (csd->c_size + 1) << (csd->c_size_mult + 2);
	} else {
		/* csd->csdStructure == MCD_CSD_VERSION_2 */
		csd->c_size =
		    ((csd_raw[2] & 0x3f)) | ((csd_raw[1] & (0xffff << 16)) >>
					     16);

		/* Other fields are undefined --> zeroed */
		csd->vdd_r_curr_min = 0;
		csd->vdd_r_curr_max = 0;
		csd->vdd_w_curr_min = 0;
		csd->vdd_w_curr_max = 0;
		csd->c_size_mult = 0;

		/* Block number: cf Spec Version 2 page 109 (122). */
		csd->block_number = (csd->c_size + 1) * 1024;
		/* should check incompatible size and return MCD_ERR_UNUSABLE_CARD; */
	}

	csd->erase_blk_enable = (uint8_t) ((csd_raw[1] & (0x1 << 14)) >> 14);
	csd->sector_size = (uint8_t) ((csd_raw[1] & (0x7f << 7)) >> 7);
	csd->wp_grp_size = (uint8_t) (csd_raw[1] & 0x7f);

	csd->wp_grp_enable =
	    (((csd_raw[0] & (0x1 << 31)) >> 31) != 0) ? true : false;
	csd->r2w_factor = (uint8_t) ((csd_raw[0] & (0x7 << 26)) >> 26);
	csd->write_bl_len = (uint8_t) ((csd_raw[0] & (0xf << 22)) >> 22);
	csd->write_bl_partial =
	    (((csd_raw[0] & (0x1 << 21)) >> 21) != 0) ? true : false;
	csd->file_format_grp =
	    (((csd_raw[0] & (0x1 << 15)) >> 15) != 0) ? true : false;
	csd->copy = (((csd_raw[0] & (0x1 << 14)) >> 14) != 0) ? true : false;
	csd->perm_write_protect =
	    (((csd_raw[0] & (0x1 << 13)) >> 13) != 0) ? true : false;
	csd->tmp_write_protect =
	    (((csd_raw[0] & (0x1 << 12)) >> 12) != 0) ? true : false;
	csd->file_format = (uint8_t) ((csd_raw[0] & (0x3 << 10)) >> 10);
	csd->crc = (uint8_t) ((csd_raw[0] & (0x7f << 1)) >> 1);

	return MCD_ERR_NO;
}

static int mcd_read_csd(struct mcd_csd_reg *mcd_csd)
{
	int err_status = MCD_ERR_NO;
	uint32_t response[4] = { 0 };
	uint32_t mcd_nb_block = 0;

	/* Get card CSD */
	err_status =
	    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_SEND_CSD, mcd_rca_reg, response,
			       false);
	if (err_status == MCD_ERR_NO) {
		err_status = mcd_sdmmc_init_csd(mcd_csd, response);
	}
	/*
	 * Store it localy
	 * FIXME: Is this real ? cf Physical Spec version 2
	 * page 59 (72) about CMD16 : default block length
	 * is 512 bytes. Isn't 512 bytes a good block
	 * length ? And I quote : "In both cases, if block length is set larger
	 * than 512Bytes, the card sets the BLOCK_LEN_ERROR bit."
	 */

	mcd_block_len = (1 << mcd_csd->read_bl_len);
	if (mcd_block_len > 512) {
		mcd_block_len = 512;
	}
	mcd_nb_block =
	    mcd_csd->block_number * ((1 << mcd_csd->read_bl_len) /
				     mcd_block_len);

	return err_status;
}

static int mcd_sdmmc_tran_state(uint32_t iter)
{
	uint32_t card_response[4] = { 0 };
	int err_status = MCD_ERR_NO;
	mcd_card_status_t card_status_tran_state;
	uint64_t start_time = hal_get_uptime();
	uint64_t time_out;

	/* Using reg to clear all bit of the bitfields that are not
	 * explicitly set.
	 */
	card_status_tran_state.reg = 0;
	card_status_tran_state.fields.ready_for_data = 1;
	card_status_tran_state.fields.current_state = MCD_CARD_STATE_TRAN;

	while (1) {
		/* sdmmc_dbg("CMD13, Set Trans State\n"); */
		err_status =
		    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_SEND_STATUS, mcd_rca_reg,
				       card_response, false);
		if (err_status != MCD_ERR_NO) {
			sdmmc_err("CMD13, Fail(%d)\n", err_status);
			/* error while sending the command */
			return MCD_ERR;
		} else if (card_response[0] == card_status_tran_state.reg) {
			sdmmc_dbg("CMD13, Done %x  \n", card_response[0]);
			/* the status is the expected one - success */
			return MCD_ERR_NO;
		} else {
			/* try again
			 * check for timeout
			 */
			time_out =
			    (MCD_CARD_V1 ==
			     mcd_ver) ? MCD_CMD_TIMEOUT_V1 : MCD_CMD_TIMEOUT_V2;
			if (hal_get_uptime() - start_time > time_out) {
				sdmmc_err("CMD13, Timeout\n");
				return MCD_ERR;
			}
		}
	}
}

int mcd_open(struct mcd_csd_reg *mcd_csd, mcd_card_ver_t mcd_ver)
{
	int err_status = MCD_ERR_NO;
	uint32_t response[4] = { 0 };
	mcd_card_status_t card_status = { 0 };
	bool is_mmc = false;
	int data_bus_width;
	uint64_t start_time = 0;

	sdmmc_dbg("Enter\n");

	/* RCA = 0x0000 for card identification phase. */
	mcd_rca_reg = 0;
	/* assume it's not an SDHC card */

	mcd_card_is_sdhc = false;
	mcd_ocr_reg = MCD_SDMMC_OCR_VOLT_PROF_MASK;

	/* Disable all interrupt, and delay mclk output, using 0 pclk */
	hal_sdmmc_open(0x0);

	/* Set mclk output <= 400KHz */
	hal_sdmmc_set_clk(400000);
	/* 1. wait for sd mclk stable
	 * 2. must be wait for >= 2ms: 1ms + >= 76 clk, refer to sd spec 2.0 page 129.
	 */
	mdelay(1);
	sdmmc_dbg("CMD0\n");
	/* Send Power On command */
	err_status =
	    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_GO_IDLE_STATE, 0, response, false);
	if (MCD_ERR_NO != err_status) {
		sdmmc_err("No card\n");
		mcd_status = MCD_STATUS_NOTPRESENT;
		hal_sdmmc_close();

		return err_status;
	}

	sdmmc_dbg("CMD8\n");
	mdelay(100);
	/* Check if the card is a spec vers.2 one, response is r7 */
	err_status = mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_SEND_IF_COND,
					(MCD_CMD8_VOLTAGE_SEL |
					 MCD_CMD8_CHECK_PATTERN), response,
					false);

	if (MCD_ERR_NO != err_status) {
		sdmmc_err("CMD8 err=%d\n", err_status);
	} else {
		sdmmc_dbg("CMD8 Done (support spec v2.0) response=0x%8x\n",
			  response[0]);
		sdmmc_dbg(" hwp_sdmmc->sdmmc_mclk_adjust: 0x%x\n",
			  hwp_sdmmc->sdmmc_mclk_adjust);
		/* This card comply to the SD spec version 2. Is it compatible with the
		 * proposed voltage, and is it an high capacity one ?
		 */
		if ((response[0] & 0xff) != MCD_CMD8_CHECK_PATTERN) {
			sdmmc_err("CMD8, Not Supported\n");

			/* Bad pattern or unsupported voltage. */
			hal_sdmmc_close();
			mcd_status = MCD_STATUS_NOTPRESENT;
			/* mcd_ver = MCD_CARD_V1; */
			return MCD_ERR_UNUSABLE_CARD;
		} else {
			sdmmc_dbg("CMD8, Supported\n");
			mcd_ocr_reg |= MCD_OCR_HCS;
		}
	}

	/* TODO HCS mask bit to ACMD 41 if high capacity */
	/* Send OCR, as long as the card return busy */
	start_time = hal_get_uptime();
	while (1) {
		if (hal_get_uptime() - start_time > MCD_SDMMC_OCR_TIMEOUT) {
			sdmmc_err("ACMD41, Retry Timeout\n");
			hal_sdmmc_close();
			mcd_status = MCD_STATUS_NOTPRESENT;
			return MCD_ERR;
		}

		sdmmc_dbg("ACMD41, response r3\n");
		err_status =
		    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_SEND_OP_COND, mcd_ocr_reg,
				       response, false);
		if (err_status == MCD_ERR_CARD_NO_RESPONSE) {
			sdmmc_err("ACMD41, No Response, MMC Card?\n");
			is_mmc = true;
			break;
		}
		sdmmc_dbg("ACMD41 Done, response = 0x%08x\n", response[0]);

		/* Bit 31 is power up busy status bit(pdf spec p. 109) */
		mcd_ocr_reg = (response[0] & 0x7fffffff);
		/* Power up busy bit is set to 1 */
		if (response[0] & 0x80000000) {
			if ((mcd_ocr_reg & MCD_OCR_HCS) == MCD_OCR_HCS) {
				/* Card is V2: check for high capacity */
				if (response[0] & MCD_OCR_CCS_MASK) {
					mcd_card_is_sdhc = true;
					sdmmc_dbg("Card is SDHC\n");
				} else {
					mcd_card_is_sdhc = false;
					sdmmc_dbg("Card is V2, but NOT SDHC\n");
				}
			} else {
				sdmmc_dbg("Card is NOT V2\n");
			}
			sdmmc_dbg("Inserted Card is a SD card\n");
			break;
		}
	}
	/* Not support ACMD41 */
	if (is_mmc) {
		sdmmc_dbg("MMC Card\n");
		while (1) {
			if (hal_get_uptime() - start_time >
			    MCD_SDMMC_OCR_TIMEOUT) {
				hal_sdmmc_close();
				mcd_status = MCD_STATUS_NOTPRESENT;
				return MCD_ERR;
			}

			err_status =
			    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_MMC_SEND_OP_COND,
					       mcd_ocr_reg, response, false);
			if (err_status == MCD_ERR_CARD_NO_RESPONSE) {
				hal_sdmmc_close();
				mcd_status = MCD_STATUS_NOTPRESENT;
				return MCD_ERR;
			}
			/* Bit 31 is power up busy status bit(pdf spec p. 109) */
			mcd_ocr_reg = (response[0] & 0x7fffffff);

			/* Power up busy bit is set to 1,
			 * we can continue ...
			 */
			if (response[0] & 0x80000000) {
				/* 30bit: 1: sector mode, > 2G 0: BYTE mode <=2G */
				mcd_card_is_sdhc =
				    (response[0] & 0x40000000) >
				    0 ? true : false;
				if (mcd_card_is_sdhc)
					sdmmc_dbg("Card is MMC > 2GB\n");
				else
					sdmmc_dbg("Card is MMC <= 2GB\n");
				break;
			}
		}
	}

	sdmmc_dbg("CMD2\n");

	/* Get the CID of the card. */
	err_status =
	    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_ALL_SEND_CID, 0, response, false);
	if (MCD_ERR_NO != err_status) {
		sdmmc_err("CMD2 Fail\n");
		hal_sdmmc_close();
		mcd_status = MCD_STATUS_NOTPRESENT;
		return err_status;
	}

	sdmmc_dbg("CMD3\n");

	/* Get card RCA: set rca to card and nor get this. */
	if (is_mmc)
		mcd_rca_reg = 0x10000;
	err_status = mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_SEND_RELATIVE_ADDR,
					mcd_rca_reg, response, false);

	if (MCD_ERR_NO != err_status) {
		sdmmc_err("CMD3 Fail\n");
		hal_sdmmc_close();
		mcd_status = MCD_STATUS_NOTPRESENT;
		return err_status;
	}
	/* Spec Ver 2 pdf p. 81 - rca are the 16 upper bit of this
	 * R6 answer. (lower bits are stuff bits)
	 */
	if (is_mmc == false) {
		mcd_rca_reg = response[0] & 0xffff0000;
		sdmmc_dbg("CMD3 Done, response=0x%8x, rca=0x%6x\n",
			  response[0], mcd_rca_reg);
	}
	/* Get card CSD */
	err_status = mcd_read_csd(mcd_csd);
	if (err_status != MCD_ERR_NO) {
		sdmmc_dbg("Because Get CSD, Initialize Failed\n");
		hal_sdmmc_close();
		mcd_status = MCD_STATUS_NOTPRESENT;
		return err_status;
	}
	/* If possible, set the DSR */
	if (mcd_csd->dsr_imp) {
		err_status =
		    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_SET_DSR, 0, response,
				       false);
		if (err_status != MCD_ERR_NO) {
			sdmmc_dbg("Because Set DSR, Initialize Failed\n");
			hal_sdmmc_close();
			mcd_status = MCD_STATUS_NOTPRESENT;
			return err_status;
		}
	}

	sdmmc_dbg("CMD7\n");
	/* Select the card */
	err_status =
	    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_SELECT_CARD, mcd_rca_reg, response,
			       false);
	if (err_status != MCD_ERR_NO) {
		sdmmc_err("CMD7 Fail\n");
		hal_sdmmc_close();
		mcd_status = MCD_STATUS_NOTPRESENT;
		return err_status;
	}
	/* That command returns the card status, we check we're in data mode. */
	card_status.reg = response[0];
	sdmmc_dbg("CMD7 Done, card_status=0x%8x\n", response[0]);

	if (!(card_status.fields.ready_for_data)) {
		sdmmc_err("Ready for Data check Fail\n");
		hal_sdmmc_close();
		mcd_status = MCD_STATUS_NOTPRESENT;
		return MCD_ERR;
	}

	data_bus_width = HAL_SDMMC_DATA_BUS_WIDTH_4;

	err_status =
	    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_SET_BUS_WIDTH, data_bus_width,
			       response, false);
	if (err_status != MCD_ERR_NO) {
		sdmmc_err("Because Set Bus, Initialize Failed");
		hal_sdmmc_close();	//pmd_EnablePower(PMD_POWER_SDMMC, false);
		mcd_status = MCD_STATUS_NOTPRESENT;
		return err_status;
	}
	card_status.reg = response[0];
	if (!(card_status.fields.app_cmd)
	    || !(card_status.fields.ready_for_data)
	    || (card_status.fields.current_state != MCD_CARD_STATE_TRAN)) {
		sdmmc_err("ACMD6 status=%0x", card_status.reg);
		hal_sdmmc_close();
		mcd_status = MCD_STATUS_NOTPRESENT;
		return MCD_ERR;
	}

	hal_sdmmc_set_data_width(data_bus_width);

	sdmmc_dbg("CMD16 - set block len mcd_block_len=%d\n", mcd_block_len);

	/* Configure the block lenght */
	err_status =
	    mcd_sdmmc_send_cmd(HAL_SDMMC_CMD_SET_BLOCKLEN, mcd_block_len,
			       response, false);
	if (err_status != MCD_ERR_NO) {
		sdmmc_err("CMD16, SET_BLOCKLEN fail\n");
		hal_sdmmc_close();
		mcd_status = MCD_STATUS_NOTPRESENT;
		return err_status;
	}
#if 1
	/* That command returns the card status, in tran state */
	card_status.reg = response[0];
	mcd_card_status_t expectedcard_status;
	expectedcard_status.reg = 0;
	expectedcard_status.fields.ready_for_data = 1;
	expectedcard_status.fields.current_state = MCD_CARD_STATE_TRAN;

	if (card_status.reg != expectedcard_status.reg) {
		sdmmc_err("CMD16 status Fail\n");
		hal_sdmmc_close();
		mcd_status = MCD_STATUS_NOTPRESENT;
		return MCD_ERR;
	}
#endif

	/* TODO: DELETE!!! */
	hal_sdmmc_enter_trans_mode();

	/* For cards, fpp = 25MHz for sd, fpp = 50MHz, for sdhc */
#define WORKING_CLK_FREQ   5000000
	sdmmc_dbg("Set Clock: %d\n", WORKING_CLK_FREQ);
	hal_sdmmc_set_clk(WORKING_CLK_FREQ);

	mcd_status = MCD_STATUS_OPEN;
	mcd_ver = mcd_ver;

	sdmmc_dbg("Done\n");

	return MCD_ERR_NO;
}

int mcd_close(void)
{
	int err_status = MCD_ERR_NO;

	/*
	 * Don't close the MCD driver if a transfer is in progress,
	 * and be definitive about it:
	 */
	if (hal_sdmmc_is_busy() == true) {
		sdmmc_dbg
		    ("Attempt to close MCD while a transfer is in progress");
	}
	/* Brutal force stop current transfer, if any. */
	hal_sdmmc_stop_trans(&mcd_sdmmc_trans);

	/* Close the SDMMC module */
	hal_sdmmc_close();
	/* without GPIO */
	mcd_status = MCD_STATUS_NOTOPEN_PRESENT;

	return err_status;
}

static int mcd_sdmmc_mlt_blk_read_polling(uint8_t * pread, uint32_t start_block,
					  uint32_t block_num)
{
	uint32_t card_response[4] = { 0 };
	int err_status = MCD_ERR_NO;
	uint32_t read_cmd;
	mcd_card_status_t card_status_tran_state;
	uint64_t start_time, tran_time_out, read_time_out;
	uint32_t buf_size, buf_idx;

	card_status_tran_state.reg = 0;
	card_status_tran_state.fields.ready_for_data = 1;
	card_status_tran_state.fields.current_state = MCD_CARD_STATE_TRAN;

	volatile uint32_t *reg_status2 = &(hwp_sdmmc->sdmmc_status2);
	volatile uint32_t *reg_txrx_fifo = &(hwp_sdmmc->apbi_fifo_txrx);

	sdmmc_dbg("Enter\n");

	mcd_sdmmc_trans.sys_mem_addr = (uint8_t *) pread;
	mcd_sdmmc_trans.sdcard_addr = (uint8_t *) start_block;
	mcd_sdmmc_trans.block_num = block_num;
	mcd_sdmmc_trans.block_size = mcd_block_len;
	mcd_sdmmc_trans.direction = HAL_SDMMC_DIRECTION_READ;

	/* Command are different for reading one or several blocks of data */
	if (block_num == 1)
		read_cmd = HAL_SDMMC_CMD_READ_SINGLE_BLOCK;
	else
		read_cmd = HAL_SDMMC_CMD_READ_MULT_BLOCK;

	/* Initiate data migration through Ifc. */
	if (hal_sdmmc_set_tans_info(&mcd_sdmmc_trans) != 0) {
		sdmmc_err("write sd no ifc channel\n");
		return MCD_ERR_DMA_BUSY;
	}

	sdmmc_dbg("send command\n");
	/* Initiate data migration of multiple blocks through SD bus. */
	err_status = mcd_sdmmc_send_cmd(read_cmd,
					start_block, card_response, false);

	if (err_status != MCD_ERR_NO) {
		sdmmc_err("send command err=%d\n", err_status);
		hal_sdmmc_stop_trans(&mcd_sdmmc_trans);
		return MCD_ERR_CMD;
	}

	if (card_response[0] != card_status_tran_state.reg) {
		sdmmc_err("command response\n");
		hal_sdmmc_stop_trans(&mcd_sdmmc_trans);
		return MCD_ERR;
	}

	/* Total buffer size in bytes */
	buf_size = block_num << 9;
	buf_idx = 0;
	sdmmc_dbg("reg_status2 0x%x \n", *reg_status2);
	while (buf_idx < buf_size) {
		while ((*reg_status2 & 0x0700UL) == 0UL) ;
		*((uint32_t *) (&(pread[buf_idx]))) = *reg_txrx_fifo;
		buf_idx += 4;
	}

	/* Wait */
	start_time = hal_get_uptime();
	read_time_out = (MCD_CARD_V1 == mcd_ver) ?
	    MCD_READ_TIMEOUT_V1 : MCD_READ_TIMEOUT_V2;

	while (!hal_sdmmc_trans_done(0)) {
		if (hal_get_uptime() - start_time > (read_time_out * block_num)) {
			sdmmc_err("timeout, INT_STATUS=0x%08X\n",
				  hwp_sdmmc->sdmmc_int_status);
			/* Abort the transfert. */
			hal_sdmmc_stop_trans(&mcd_sdmmc_trans);
			return MCD_ERR_CARD_TIMEOUT;
		}
	}

	/*
	 * Note: CMD12 (stop transfer) is automatically
	 * sent by the SDMMC controller.
	 */

	if (mcd_sdmmc_read_check_crc() != MCD_ERR_NO) {
		sdmmc_err("crc error\n");
		return MCD_ERR;
	}

	tran_time_out = (MCD_CARD_V1 == mcd_ver) ?
	    MCD_TRAN_TIMEOUT_V1 : MCD_TRAN_TIMEOUT_V2;
	/* Check that the card is in tran (Transmission) state. */
	if (MCD_ERR_NO != mcd_sdmmc_tran_state(tran_time_out)) {
		sdmmc_err("trans state timeout\n");
		return MCD_ERR_CARD_TIMEOUT;
	}

	return MCD_ERR_NO;
}

static int mcd_sdmmc_mlt_blk_write_polling(const uint8_t * pwrite,
					   uint32_t start_block,
					   uint32_t block_num)
{
	uint32_t card_response[4] = { 0 }, read_cmd;
	int err_status = MCD_ERR_NO;
	mcd_card_status_t card_status_tran_state;
	uint64_t start_time, tran_time_out, read_time_out;
	uint32_t *pwd_buf = (uint32_t *) pwrite;
	uint32_t wdbuf_size, wdbuf_idx;
	volatile uint32_t *reg_status2 = &(hwp_sdmmc->sdmmc_status2);
	volatile uint32_t *reg_txrx_fifo = &(hwp_sdmmc->apbi_fifo_txrx);

	card_status_tran_state.reg = 0;
	card_status_tran_state.fields.ready_for_data = 1;
	card_status_tran_state.fields.current_state = MCD_CARD_STATE_TRAN;

	sdmmc_dbg("Enter\n");

	mcd_sdmmc_trans.sys_mem_addr = (uint8_t *) pwrite;
	mcd_sdmmc_trans.sdcard_addr = (uint8_t *) start_block;
	mcd_sdmmc_trans.block_num = block_num;
	mcd_sdmmc_trans.block_size = mcd_block_len;
	mcd_sdmmc_trans.direction = HAL_SDMMC_DIRECTION_WRITE;

	/* Command are different for reading one or several blocks of data */
	if (block_num == 1)
		read_cmd = HAL_SDMMC_CMD_WRITE_SINGLE_BLOCK;
	else
		read_cmd = HAL_SDMMC_CMD_WRITE_MULT_BLOCK;

	if (hal_sdmmc_set_tans_info(&mcd_sdmmc_trans) != 0) {
		sdmmc_err("write sd no ifc channel\n");
		return MCD_ERR_DMA_BUSY;
	}
	/* Total buffer count in words */
	wdbuf_size = block_num << 7;
	wdbuf_idx = 0;
	while (wdbuf_idx < wdbuf_size) {
		while ((*reg_status2 & 0x070000) == 0) ;
		*reg_txrx_fifo = *(pwd_buf + wdbuf_idx);
		wdbuf_idx++;

		if (wdbuf_idx == 4) {
			/* Initiate data migration of multiple blocks through SD bus. */
			err_status =
			    mcd_sdmmc_send_cmd(read_cmd, start_block,
					       card_response, false);
			if (err_status != MCD_ERR_NO) {
				sdmmc_err("send command err=%d\n", err_status);
				hal_sdmmc_stop_trans(&mcd_sdmmc_trans);
				return MCD_ERR_CMD;
			}
			sdmmc_dbg("card_response:[0x%x][0x%x][0x%x][0x%x]\n",
				  card_response[0], card_response[1],
				  card_response[2], card_response[3]);
			sdmmc_dbg("hwp_sdmmc->sdmmc_status2:0x%x\n",
				  hwp_sdmmc->sdmmc_status2);
			if (card_response[0] != card_status_tran_state.reg) {
				sdmmc_err("command response=0x%08X\n",
					  card_response[0]);
				hal_sdmmc_stop_trans(&mcd_sdmmc_trans);
				return MCD_ERR;
			}
			sdmmc_dbg("send command done\n");
		}
	}

	/* Wait */
	start_time = hal_get_uptime();
	read_time_out =
	    (MCD_CARD_V1 ==
	     mcd_ver) ? MCD_READ_TIMEOUT_V1 : MCD_READ_TIMEOUT_V2;
	while (!hal_sdmmc_trans_done(1)) {
		if (hal_get_uptime() - start_time > (read_time_out * block_num)) {
			sdmmc_err("timeout\n");
			sdmmc_err("timeout, INT_STATUS=0x%08X\n",
				  hwp_sdmmc->sdmmc_int_status);
			sdmmc_err("sdmmc_config:0x%x\n",
				  hwp_sdmmc->sdmmc_config);
			sdmmc_err("sdmmc_status: 0x%x\n",
				  hwp_sdmmc->sdmmc_status);
			sdmmc_err("sdmmc_status2:0x%x\n",
				  hwp_sdmmc->sdmmc_status2);
			/* Abort the transfert. */
			hal_sdmmc_stop_trans(&mcd_sdmmc_trans);
			return MCD_ERR_CARD_TIMEOUT;
		}
	}

	/*
	 * Note: CMD12 (stop transfer) is automatically
	 * sent by the SDMMC controller.
	 */
	sdmmc_dbg
	    ("before set hwp_sdmmc->sdmmc_status: 0x%x, hwp_sdmmc->sdmmc_int_status:0x%x\n",
	     hwp_sdmmc->sdmmc_status, hwp_sdmmc->sdmmc_int_status);
	if (mcd_sdmmc_read_check_crc() != MCD_ERR_NO) {
		sdmmc_err("crc error\n");
		return MCD_ERR;
	}

	sdmmc_dbg
	    ("before set hwp_sdmmc->sdmmc_status: 0x%x, hwp_sdmmc->sdmmc_int_status:0x%x\n",
	     hwp_sdmmc->sdmmc_status, hwp_sdmmc->sdmmc_int_status);
	tran_time_out =
	    (MCD_CARD_V1 ==
	     mcd_ver) ? MCD_TRAN_TIMEOUT_V1 : MCD_TRAN_TIMEOUT_V2;
	/* Check that the card is in tran (Transmission) state. */
	if (MCD_ERR_NO != mcd_sdmmc_tran_state(tran_time_out)) {
		sdmmc_err("trans state timeout\n");
		return MCD_ERR_CARD_TIMEOUT;
	}
	return MCD_ERR_NO;
}

#define SDMMC_CLKGATE_REG  (*((volatile unsigned int *)0x40000008))
#define SDMMC_CLKGATE_REG2 (*((volatile unsigned int *)0x4000000C))
#define SDMMC_CLKGATE_BP_REG (*((volatile unsigned int *)0x40000028))
#define IOMUX_CTRL_1_REG (*((volatile unsigned int *)0x40001048))
#define IOMUX_CTRL_4_REG (*((volatile unsigned int *)0x40001054))

static int is_init_sdmmc;
void rda_sdmmc_init(unsigned int d0)
{
	uint32_t reg_val;

	/* Enable SDMMC power and clock */
	SDMMC_CLKGATE_REG |= (0x01 << 5);

	if (RDA5981A_PIN(1, 6) == d0) {
		rda_ccfg_gp6(1);
		rda_ccfg_gp7(1);
		/* Config SDMMC pins iomux: clk, cmd, d0, d1, d2, d3 (GPIO0, 6, 7, 9, 12, 13) */
		reg_val =
		    IOMUX_CTRL_1_REG & ~(7) & ~(7 << 18) & ~(7 << 21) & ~(7 <<
									  27);
		IOMUX_CTRL_1_REG =
		    reg_val | (3) | (7 << 18) | (5 << 21) | (3 << 27);
	} else {
		rda_ccfg_gp7(1);
		/* Config SDMMC pins iomux: clk, cmd, d0, d1, d2, d3 (GPIO0, 3, 7, 9, 12, 13) */
		reg_val =
		    IOMUX_CTRL_1_REG & ~(7) & ~(7 << 9) & ~(7 << 21) & ~(7 <<
									 27);
		IOMUX_CTRL_1_REG =
		    reg_val | (3) | (5 << 9) | (5 << 21) | (3 << 27);
	}
	reg_val = IOMUX_CTRL_4_REG & ~(7) & ~(7 << 3);
	IOMUX_CTRL_4_REG = reg_val | (3) | (3 << 3);

	is_init_sdmmc = 1;
}

int rda_sdmmc_open(void)
{
	int ret_val;
	sdmmc_dbg("enter rda_sdmmc_open\n");

	if (!is_init_sdmmc) {
		rda_sdmmc_init(RDA5981A_PIN(1, 6));
	}

	/* Initialize SDMMC Card */
	ret_val = (MCD_ERR_NO == mcd_open(&mcd_csd, MCD_CARD_V2)) ? 1 : 0;

	return ret_val;
}

int rda_sdmmc_read_blocks(uint8_t * buffer, uint32_t block_start,
			  uint32_t block_num)
{
	int ret =
	    mcd_sdmmc_mlt_blk_read_polling(buffer, block_start, block_num);

	if (MCD_ERR_NO != ret) {
		sdmmc_err("Sdmmc Read Blocks Err: %d\r\n", ret);
	}

	return ret;
}

int rda_sdmmc_write_blocks(const uint8_t * buffer, uint32_t block_start,
			   uint32_t block_num)
{
	int ret =
	    mcd_sdmmc_mlt_blk_write_polling(buffer, block_start, block_num);

	if (MCD_ERR_NO != ret) {
		sdmmc_err("Sdmmc Write Blocks Err: %d\r\n", ret);
	}
	return ret;
}

int rda_sdmmc_get_csdinfo(uint32_t * block_size)
{
	/*
	 * csd_structure : csd[127:126]
	 * c_size        : csd[73:62]
	 * c_size_mult   : csd[49:47]
	 * read_bl_len   : csd[83:80]
	 */
	int ret_val = (int)mcd_csd.csd_structure;
	uint32_t block_len, mult, blocknr, capacity;

	switch (ret_val) {
	case 0:
		block_len = 1 << mcd_csd.read_bl_len;
		mult = 1 << (mcd_csd.c_size_mult + 2);
		blocknr = (mcd_csd.c_size + 1) * mult;
		capacity = blocknr * block_len;
		*block_size = capacity / 512;
		break;

	case 1:
		*block_size = (mcd_csd.c_size + 1) * 1024;
		break;

	default:
		*block_size = 0;
		break;
	}

	return ret_val;
}

#if SDMMC_TEST_ENABLE
void rda_sdmmc_test(void)
{
	uint8_t buf[512];
	uint16_t idx;
	uint32_t sector = 0;
	uint32_t blocks = 1;

	for (idx = 0; idx < 512; idx++) {
		buf[idx] = (uint8_t) (idx & 0xff);
	}

	sdmmc_dbg("Init SDMMC\r\n");
	rda_sdmmc_init(RDA5981A_PIN(1, 6));	// for RDA5981A_HDK_V1.0

	rda_sdmmc_open();

	printk("Read sector:%d, blocks:%d\r\n", sector, blocks);
	rda_sdmmc_read_blocks(buf, sector, blocks);

	for (idx = 0; idx < 512; idx++) {
		printk(" %02X", buf[idx]);
		if ((idx % 16) == 15)
			printk("\r\n");
	}

	sdmmc_dbg("Close SDMMC\r\n");
	mcd_close();
}
#endif
