/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <soc.h>

/* SMFI register structure check */
IT8XXX2_REG_SIZE_CHECK(smfi_it8xxx2_regs, 0xd1);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_ECINDAR0, 0x3b);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_ECINDAR1, 0x3c);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_ECINDAR2, 0x3d);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_ECINDAR3, 0x3e);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_ECINDDR, 0x3f);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_SCAR0L, 0x40);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_SCAR0M, 0x41);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_SCAR0H, 0x42);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_HRAMWC, 0x5a);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_HRAMW0BA, 0x5b);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_HRAMW1BA, 0x5c);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_HRAMW0AAS, 0x5d);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_HRAMW1AAS, 0x5e);
IT8XXX2_REG_OFFSET_CHECK(smfi_it8xxx2_regs, SMFI_FLHCTRL6R, 0xa2);

/* EC2I register structure check */
IT8XXX2_REG_SIZE_CHECK(ec2i_regs, 0x06);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, IHIOA, 0x00);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, IHD, 0x01);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, LSIOHA, 0x02);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, IBMAE, 0x04);
IT8XXX2_REG_OFFSET_CHECK(ec2i_regs, IBCTL, 0x05);

/* KBC register structure check */
IT8XXX2_REG_SIZE_CHECK(kbc_regs, 0x0b);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHICR, 0x00);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBIRQR, 0x02);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHISR, 0x04);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHIKDOR, 0x06);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHIMDOR, 0x08);
IT8XXX2_REG_OFFSET_CHECK(kbc_regs, KBHIDIR, 0x0a);

/* PMC register structure check */
IT8XXX2_REG_SIZE_CHECK(pmc_regs, 0x100);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM1STS, 0x00);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM1DO, 0x01);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM1DI, 0x04);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM1CTL, 0x06);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM2STS, 0x10);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM2DO, 0x11);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM2DI, 0x14);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, PM2CTL, 0x16);
IT8XXX2_REG_OFFSET_CHECK(pmc_regs, MBXCTRL, 0x19);

/* eSPI slave register structure check */
IT8XXX2_REG_SIZE_CHECK(espi_slave_regs, 0xd8);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, GCAPCFG1, 0x05);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_PC_CAPCFG3, 0x0b);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_VW_CAPCFG3, 0x0f);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_OOB_CAPCFG3, 0x13);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_FLASH_CAPCFG3, 0x17);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, CH_FLASH_CAPCFG2_3, 0x1b);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESPCTRL0, 0x90);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESGCTRL0, 0xa0);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESGCTRL1, 0xa1);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESGCTRL2, 0xa2);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESUCTRL0, 0xb0);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESOCTRL0, 0xc0);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESOCTRL1, 0xc1);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESPISAFSC0, 0xd0);
IT8XXX2_REG_OFFSET_CHECK(espi_slave_regs, ESPISAFSC7, 0xd7);

/* eSPI vw register structure check */
IT8XXX2_REG_SIZE_CHECK(espi_vw_regs, 0x9a);
IT8XXX2_REG_OFFSET_CHECK(espi_vw_regs, VW_INDEX, 0x00);
IT8XXX2_REG_OFFSET_CHECK(espi_vw_regs, VWCTRL0, 0x90);
IT8XXX2_REG_OFFSET_CHECK(espi_vw_regs, VWCTRL1, 0x91);

/* eSPI Queue 0 registers structure check */
IT8XXX2_REG_SIZE_CHECK(espi_queue0_regs, 0xd0);
IT8XXX2_REG_OFFSET_CHECK(espi_queue0_regs, PUT_OOB_DATA, 0x80);

/* eSPI Queue 1 registers structure check */
IT8XXX2_REG_SIZE_CHECK(espi_queue1_regs, 0xc0);
IT8XXX2_REG_OFFSET_CHECK(espi_queue1_regs, UPSTREAM_DATA, 0x00);
IT8XXX2_REG_OFFSET_CHECK(espi_queue1_regs, PUT_FLASH_NP_DATA, 0x80);

/* GPIO register structure check */
#ifdef CONFIG_SOC_IT8XXX2_REG_SET_V1
IT8XXX2_REG_SIZE_CHECK(gpio_it8xxx2_regs, 0x100);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR, 0x00);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR31, 0xD5);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR18, 0xE2);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR21, 0xE6);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR29, 0xEE);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR2, 0xF1);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR7, 0xF6);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR14, 0xFD);
#elif CONFIG_SOC_IT8XXX2_REG_SET_V2
IT8XXX2_REG_SIZE_CHECK(gpio_it8xxx2_regs, 0x2f);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR, 0x00);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR2, 0x11);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR7, 0x16);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR12, 0x1b);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_PGWCR, 0x1f);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR21, 0x26);
IT8XXX2_REG_OFFSET_CHECK(gpio_it8xxx2_regs, GPIO_GCR30, 0x2d);
#endif

/* GCTRL register structure check */
IT8XXX2_REG_SIZE_CHECK(gctrl_it8xxx2_regs, 0x88);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_RSTS, 0x06);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_BADRSEL, 0x0a);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_WNCKR, 0x0b);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_SPCTRL1, 0x0d);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_SPCTRL4, 0x1c);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_RSTC5, 0x21);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, gctrl_pmer2, 0x33);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_EPLR, 0x37);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_IVTBAR, 0x41);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_MCCR2, 0x44);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_P80H81HSR, 0x50);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_P80HDR, 0x51);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_H2ROFSR, 0x53);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_RVILMCR0, 0x5D);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_ECHIPID2, 0x86);
IT8XXX2_REG_OFFSET_CHECK(gctrl_it8xxx2_regs, GCTRL_ECHIPID3, 0x87);

/* PECI register structure check */
IT8XXX2_REG_SIZE_CHECK(peci_it8xxx2_regs, 0x0F);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, HOSTAR, 0x00);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, HOCTLR, 0x01);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, HOCMDR, 0x02);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, HOTRADDR, 0x03);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, HOWRLR, 0x04);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, HORDLR, 0x05);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, HOWRDR, 0x06);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, HORDDR, 0x07);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, HOCTL2R, 0x08);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, RWFCSV, 0x09);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, RRFCSV, 0x0A);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, WFCSV, 0x0B);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, RFCSV, 0x0C);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, AWFCSV, 0x0D);
IT8XXX2_REG_OFFSET_CHECK(peci_it8xxx2_regs, PADCTLR, 0x0E);

/* USB Device register structure check */
IT8XXX2_REG_SIZE_CHECK(usb_it82xx2_regs, 0xE9);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_tx_ctrl, 0x00);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_tx_trans_type, 0x01);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_tx_line_ctrl, 0x02);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_tx_sof_enable, 0x03);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_tx_addr, 0x04);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_tx_endp, 0x05);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_frame_num_msp, 0x06);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_frame_num_lsp, 0x07);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_interrupt_status, 0x08);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_interrupt_mask, 0x09);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_rx_status, 0x0A);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_rx_pid, 0x0B);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, misc_control, 0x0C);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, misc_status, 0x0D);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_rx_connect_state, 0x0E);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_sof_timer_msb, 0x0F);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, usb_ep_regs[0].ep_ctrl, 0x40);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, usb_ep_regs[0].ep_status, 0x41);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs,
	usb_ep_regs[EP0].ep_transtype_sts, 0x42);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs,
	usb_ep_regs[EP0].ep_nak_transtype_sts, 0x43);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, usb_ep_regs[3].ep_ctrl, 0x4C);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, usb_ep_regs[3].ep_status, 0x4D);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs,
	usb_ep_regs[EP3].ep_transtype_sts, 0x4E);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs,
	usb_ep_regs[EP3].ep_nak_transtype_sts, 0x4F);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, dc_control, 0x50);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, dc_frame_num_lsp, 0x56);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, fifo_regs[0].ep_rx_fifo_data, 0x60);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, fifo_regs[0].ep_tx_fifo_ctrl, 0x74);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs,
	fifo_regs[EP_EXT_REGS_9X].ext_4_15.epn0n1_ext_ctrl, 0x98);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs,
	fifo_regs[EP_EXT_REGS_BX].fifo_ctrl.ep_fifo_ctrl, 0xB8);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs,
	fifo_regs[EP_EXT_REGS_DX].ext_0_3.epn_ext_ctrl, 0xD6);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, host_device_control, 0xE0);
IT8XXX2_REG_OFFSET_CHECK(usb_it82xx2_regs, port1_misc_control, 0xE8);


/* KSCAN register structure check */
IT8XXX2_REG_SIZE_CHECK(kscan_it8xxx2_regs, 0x0F);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSOL, 0x00);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSOCTRL, 0x02);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSI, 0x04);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSIGDAT, 0x08);
IT8XXX2_REG_OFFSET_CHECK(kscan_it8xxx2_regs, KBS_KSOLGOEN, 0x0e);

/* ADC register structure check */
IT8XXX2_REG_SIZE_CHECK(adc_it8xxx2_regs, 0xf1);
IT8XXX2_REG_OFFSET_CHECK(adc_it8xxx2_regs, ADCGCR, 0x03);
IT8XXX2_REG_OFFSET_CHECK(adc_it8xxx2_regs, VCH0DATM, 0x19);
IT8XXX2_REG_OFFSET_CHECK(adc_it8xxx2_regs, ADCIVMFSCS1, 0x55);
IT8XXX2_REG_OFFSET_CHECK(adc_it8xxx2_regs, ADCIVMFSCS2, 0x56);
IT8XXX2_REG_OFFSET_CHECK(adc_it8xxx2_regs, ADCIVMFSCS3, 0x57);
IT8XXX2_REG_OFFSET_CHECK(adc_it8xxx2_regs, adc_vchs_ctrl[0].VCHCTL, 0x60);
IT8XXX2_REG_OFFSET_CHECK(adc_it8xxx2_regs, adc_vchs_ctrl[2].VCHDATM, 0x67);
IT8XXX2_REG_OFFSET_CHECK(adc_it8xxx2_regs, ADCDVSTS2, 0x6c);
IT8XXX2_REG_OFFSET_CHECK(adc_it8xxx2_regs, ADCCTL1, 0xf0);

/* Watchdog register structure check */
IT8XXX2_REG_SIZE_CHECK(wdt_it8xxx2_regs, 0x0f);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, ETWCFG, 0x01);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, ET1PSR, 0x02);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, ET1CNTLHR, 0x03);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, ET1CNTLLR, 0x04);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, ETWCTRL, 0x05);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, EWDCNTLR, 0x06);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, EWDKEYR, 0x07);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, EWDCNTHR, 0x09);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, ET2PSR, 0x0a);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, ET2CNTLHR, 0x0b);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, ET2CNTLLR, 0x0c);
IT8XXX2_REG_OFFSET_CHECK(wdt_it8xxx2_regs, ET2CNTLH2R, 0x0e);

/* SPISC register structure check */
IT8XXX2_REG_SIZE_CHECK(spisc_it8xxx2_regs, 0x28);
IT8XXX2_REG_OFFSET_CHECK(spisc_it8xxx2_regs, SPISC_IMR, 0x04);
IT8XXX2_REG_OFFSET_CHECK(spisc_it8xxx2_regs, SPISC_RXFSR, 0x07);
IT8XXX2_REG_OFFSET_CHECK(spisc_it8xxx2_regs, SPISC_CPUWTXFDB2R, 0x0a);
IT8XXX2_REG_OFFSET_CHECK(spisc_it8xxx2_regs, SPISC_RXFRDRB1, 0x0d);
IT8XXX2_REG_OFFSET_CHECK(spisc_it8xxx2_regs, SPISC_FTCB1R, 0x19);
IT8XXX2_REG_OFFSET_CHECK(spisc_it8xxx2_regs, SPISC_HPR2, 0x1e);
IT8XXX2_REG_OFFSET_CHECK(spisc_it8xxx2_regs, SPISC_RXVLISR, 0x27);

/* PWM register structure check */
IT8XXX2_REG_SIZE_CHECK(pwm_it8xxx2_regs, 0x4a);
IT8XXX2_REG_OFFSET_CHECK(pwm_it8xxx2_regs, C0CPRS, 0x00);
IT8XXX2_REG_OFFSET_CHECK(pwm_it8xxx2_regs, CTR1M, 0x10);
IT8XXX2_REG_OFFSET_CHECK(pwm_it8xxx2_regs, C4CPRS, 0x27);
IT8XXX2_REG_OFFSET_CHECK(pwm_it8xxx2_regs, CTR2, 0x42);
IT8XXX2_REG_OFFSET_CHECK(pwm_it8xxx2_regs, PWMODENR, 0x49);
