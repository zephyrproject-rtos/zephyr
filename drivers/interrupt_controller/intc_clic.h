/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 * Copyright (c) 2025 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CLIC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CLIC_H_

union CLICCFG {
	struct {
		uint32_t _reserved0: 1;
		/** number of interrupt level bits */
		uint32_t nlbits: 4;
		/** number of clicintattr[i].MODE bits */
		uint32_t nmbits: 2;
		uint32_t _reserved1: 25;
	} w;
	uint32_t qw;
};

union CLICINTIP {
	struct {
		/** Interrupt Pending */
		uint8_t IP: 1;
		uint8_t reserved0: 7;
	} b;
	uint8_t w;
};

union CLICINTIE {
	struct {
		/** Interrupt Enabled */
		uint8_t IE: 1;
		uint8_t reserved0: 7;
	} b;
	uint8_t w;
};

union CLICINTATTR {
	struct {
		/** 0: non-vectored 1:vectored */
		uint8_t shv: 1;
		/** 0: level 1: rising edge 2: falling edge */
		uint8_t trg: 2;
		uint8_t reserved0: 3;
		uint8_t mode: 2;
	} b;
	uint8_t w;
};

union CLICCTRL {
	struct {
		volatile union CLICINTIP INTIP;
		volatile union CLICINTIE INTIE;
		volatile union CLICINTATTR INTATTR;
		volatile uint8_t INTCTRL;
	} w;
	uint32_t qw;
};

union CLICINFO {
	struct {
		/** number of max supported interrupts */
		uint32_t numint: 13;
		/** architecture version */
		uint32_t version: 8;
		/** supported bits in the clicintctl */
		uint32_t intctlbits: 4;
		uint32_t _reserved0: 7;
	} b;
	uint32_t qw;
};

union CLICMTH {
	struct {
		uint32_t reserved0: 24;
		/**  machine mode interrupt level threshold */
		uint32_t mth: 8;
	} b;
	uint32_t qw;
};

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_CLIC_H_ */
