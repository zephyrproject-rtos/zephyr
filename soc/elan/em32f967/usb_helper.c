#include <zephyr/kernel.h>

#define USBD_BASE     DT_REG_ADDR(DT_NODELABEL(usbd))
#define SYS_CTRL_BASE DT_REG_ADDR(DT_NODELABEL(sysctrl))
#define CLK_CTRL_BASE DT_REG_ADDR(DT_NODELABEL(clkctrl))
#define CODE_BASE     0x100A6000

#define UDC_CLK_IDX   0x15
#define ATRIM_CLK_IDX 0x16
#define AIP_CLK_IDX   0x1c

#define CLK_CTRL_REG   (*(__IO uint32_t *)(SYS_CTRL_BASE + 0x100))
#define CLK_CTRL_REG_2 (*(__IO uint32_t *)(CLK_CTRL_REG + 0x04))

void peripheral_clk_enable(uint32_t idx)
{
	if (idx <= 31) {
		CLK_CTRL_REG |= (0x01 << idx);
	} else {
		CLK_CTRL_REG_2 |= (0x01 << idx);
	}
}

void peripheral_clk_disable(uint32_t idx)
{
	if (idx <= 31) {
		CLK_CTRL_REG &= ~(0x01 << idx);
	} else {
		CLK_CTRL_REG_2 &= ~(0x01 << (idx - 32));
	}
}

void usb_clk_enable(void)
{
	peripheral_clk_enable(UDC_CLK_IDX);
}

void usb_clk_disable(void)
{
	peripheral_clk_disable(UDC_CLK_IDX);
}

void aip_clk_enable(void)
{
	peripheral_clk_enable(AIP_CLK_IDX);
}

void aip_clk_disable(void)
{
	peripheral_clk_disable(AIP_CLK_IDX);
}

void atrim_clk_enable(void)
{
	peripheral_clk_enable(ATRIM_CLK_IDX);
}

void atrim_clk_disable(void)
{
	peripheral_clk_disable(ATRIM_CLK_IDX);
}

void e967_usb_clock_set(uint8_t clk_sel)
{
	volatile uint32_t reg;
	uint32_t code;
	struct e967_sys_control *reg_sys_ctrl = (struct e967_sys_control *)SYS_CTRL_BASE;
	struct e967_xtal_ctrl *reg_xtal_ctrl = (struct e967_xtal_ctrl *)(CLK_CTRL_BASE + 0x0200);
	struct e967_ljirc_ctrl *reg_ljirc_ctrl = (struct e967_ljirc_ctrl *)(CLK_CTRL_BASE + 0x04);
	struct e967_phy_ctrl *reg_usb_phy = (struct e967_phy_ctrl *)(CLK_CTRL_BASE + 0x0700);
	struct e967_usb_pll_ctrl *reg_usbpll_ctrl =
		(struct e967_usb_pll_ctrl *)(CLK_CTRL_BASE + 0x0400);

	code = 0;
	aip_clk_disable();

	if ((clk_sel == USB_XTAL_12M) || (clk_sel == USB_XTAL_24M)) {
		reg_sys_ctrl->xtal_lirc_sel = 0;

		if (clk_sel == USB_XTAL_12M) {
			reg_xtal_ctrl->xtal_freq_sel = 0x03;
		} else {
			reg_xtal_ctrl->xtal_freq_sel = 0x01;
		}

		reg_xtal_ctrl->xtal_pd = 0;
		k_busy_wait(2000);
		do {
			reg = reg_xtal_ctrl->xtal_stable;
		} while (reg == 0);
		k_busy_wait(12000);

	} else {
		code = *((uint32_t *)(CODE_BASE + 0x090));
		reg_ljirc_ctrl->lj_irc_fr = code & 0x0000000F;
		reg_ljirc_ctrl->lj_irc_ca = (code & 0x000001F0) >> 4;
		reg_ljirc_ctrl->lj_irc_fc = (code & 0x00000E00) >> 9;
		reg_ljirc_ctrl->lj_irc_tmv10 = (code & 0x00003000) >> 12;

		code = *((uint32_t *)(CODE_BASE + 0x0F0));
		reg_usb_phy->phy_rtrim = code;
		reg_sys_ctrl->xtal_lirc_sel = 1;
	}

	reg_ljirc_ctrl->ljirc_pd = 0;
	k_busy_wait(2000);
	reg_sys_ctrl->usb_clk_sel = 0;
	usb_clk_disable();

	reg_usbpll_ctrl->usb_pll_pd = 0;
	while (reg_usbpll_ctrl->usb_pll_stable == 0) {
	}

	reg_usb_phy->usb_phy_pd_b = 1;
}

static const unsigned char ep_conf_data[6] = {0x43, 0x43, 0x42, 0x42, 0xFA, 0x00};
static const uint32_t ep1_size = 64;
static const uint32_t ep2_size = 64;
static const uint32_t ep3_size = 64;
static const uint32_t ep4_size = 64;

void e967_usb_configure_ep(void)
{
	int index;
	struct udc_cf_data *reg_udc_cf_data = (struct udc_cf_data *)(USBD_BASE + 0x04);
	uint32_t *epbuf_0 = (uint32_t *)(USBD_BASE + 0x60);
	uint32_t *epbuf_1 = (uint32_t *)(USBD_BASE + 0x64);

	for (index = 0; index < 4; index++) {
		reg_udc_cf_data->bits.config_data = ep_conf_data[index];
		while (reg_udc_cf_data->bits.ep_config_rdy == 0) {
		}
	}

	reg_udc_cf_data->bits.config_data = ep_conf_data[4];
	while (reg_udc_cf_data->bits.ep_config_done == 0) {
	}

	*epbuf_0 = (ep2_size << 16) + ep1_size;
	*epbuf_1 = (ep4_size << 16) + ep3_size;
}
