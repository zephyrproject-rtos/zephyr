/* pci_mgr.h - hypervisor PCI bus manager */

/*
 * Copyright (c) 2009-2011, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PCI_PCI_MGR_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCI_PCI_MGR_H_

#ifdef __cplusplus
extern "C" {

/* Remapped reserved  C++ words */
#define class _class

#endif /* __cplusplus */

/* requests available from the PCI bus manager */

#define SYS_PCI_REQUEST_READ_DATA 1
#define SYS_PCI_REQUEST_WRITE_DATA 2
#define SYS_PCI_REQUEST_READ_ADDR 3
#define SYS_PCI_REQUEST_WRITE_ADDR 4

/* requests results */

#define SYS_PCI_REQUEST_OK 0
#define SYS_PCI_REQUEST_FAILED 1

/* data access sizes */

#define SYS_PCI_ACCESS_8BIT 0
#define SYS_PCI_ACCESS_16BIT 1
#define SYS_PCI_ACCESS_32BIT 3

#define PCI_MAX_ADDR_CACHE 8 /* cache up to 8 addr writes */

#define PCI_HEADER_WORDS 64 /* 64 32-bit words in the header */

#define PCI_NO_OFFSET 0

/* default controller to use */

#define DEFAULT_PCI_CONTROLLER 0
#define NO_PCI_CONTROLLER 0xffff

/* maximum number of controllers supported */

#define PCI_MAX_CONTROLLER 4
#define PCI_MAX_MGR PCI_MAX_CONTROLLER

/* maximum number of extended capabilities */

#define PCI_MAX_ECP 32

/* invalid pci controller */

#define PCI_INVALID_CONTROLLER_ID 0xffffffff

#ifndef _ASMLANGUAGE

/*
 *
 * PCI Address Register
 *
 * The configuration address register is a 32-bit register with the format
 * shown below. Bit 31 is an enable flag for determining when accesses to the
 * configuration data should be translated to configuration cycles. Bits 23
 * through 16 allow the configuration software to choose a specific PCI bus in
 * the system. Bits 15 through 11 select the specific device on the PCI Bus.
 * Bits 10 through 8 choose a specific function in a device (if the device
 * supports multiple functions). Bits 7 through 2 select the specific 32-bit
 * area in the device's configuration space.
 *
 * +--------------------------------------------------------+
 * |      Bit 31      |    Bits 30-24    |    Bits 23-16    |
 * +------------------+------------------+------------------+
 * |      Enable      |     Reserved     |    Bus Number    |
 * +--------------------------------------------------------+
 *
 * +---------------------------------------------------------------------------+
 * |    Bits 15-11    |    Bits 10-8     |    Bits 7-2      |    Bits 1-0      |
 * +------------------+------------------+------------------+------------------+
 * |   Device Number  | Function Number  | Register Number  |        00        |
 * +---------------------------------------------------------------------------+
 *
 */

union pci_addr_reg {
	struct {
#ifdef _BIG_ENDIAN
		u32_t enable : 1; /* access enabled */
		u32_t reserved1 : 7;
		u32_t bus : 8;    /* PCI bus number */
		u32_t device : 5; /* PCI device number */
		u32_t func : 3;   /* device function number */
		u32_t reg : 6;    /* config space register number */
		u32_t offset : 2; /* offset for access data */
#else
		u32_t offset : 2; /* offset for access data */
		u32_t reg : 6;    /* config space register number */
		u32_t func : 3;   /* device function number */
		u32_t device : 5; /* PCI device number */
		u32_t bus : 8;    /* PCI bus number */
		u32_t reserved1 : 7;
		u32_t enable : 1; /* access enabled */
#endif
	} field;
	u32_t value;
};


/*
 *
 * PCI Extended Address Register
 *
 * The configuration address register is a 32-bit register with the format
 * shown below. Bit 31 is an enable flag for determining when accesses to the9
 * configuration data should be translated to configuration cycles. Bits 23
 * through 16 allow the configuration software to choose a specific PCI bus in
 * the system. Bits 15 through 11 select the specific device on the PCI Bus.
 * Bits 10 through 8 choose a specific function in a device (if the device
 * supports multiple functions). Bits 7 through 2 select the specific 32-bit
 * area in the device's configuration space.
 *
 * +--------------------------------------------------------+
 * |      Bit 31      |    Bits 30-24    |    Bits 23-16    |
 * +------------------+------------------+------------------+
 * |      Enable      |     Reserved     |    Bus Number    |
 * +--------------------------------------------------------+
 *
 * +---------------------------------------------------------------------------+
 * |    Bits 15-11    |    Bits 10-8     |    Bits 7-2      |    Bits 1-0      |
 * +------------------+------------------+------------------+------------------+
 * |   Device Number  | Function Number  | Register Number  |        00        |
 * +---------------------------------------------------------------------------+
 */

union pcie_addr_reg {
	struct {
#ifdef _BIG_ENDIAN
		u32_t reserved1 : 4;
		u32_t bus : 8;    /* PCI bus number */
		u32_t device : 5; /* PCI device number */
		u32_t func : 3;   /* device function number */
		u32_t reg : 10;   /* config space register number */
		u32_t reserved0 : 2;
#else
		u32_t reserved0 : 2;
		u32_t reg : 10;   /* config space register number */
		u32_t func : 3;   /* device function number */
		u32_t device : 5; /* PCI device number */
		u32_t bus : 8;    /* PCI bus number */
		u32_t reserved1 : 4;
#endif
	} field;
	u32_t value;
};

/*
 * PCI Device Structure
 *
 * The PCI Specification defines the organization of the 256-byte Configuration
 * Space registers and imposes a specific template for the space. The table
 * below shows the layout of the 256-byte Configuration space. All PCI
 * compliant devices must support the Vendor ID, Device ID, Command and Status,
 * Revision ID, Class Code and Header Type fields. Implementation of the other
 * registers is optional, depending upon the devices functionality.
 *
 * The PCI devices follow little ENDIAN ordering. The lower addresses contain
 * the least significant portions of the field. Software to manipulate this
 * structure must take particular care that the endian-ordering follows the PCI
 * devices, not the CPUs.
 *
 * Header Type 0x00:
 *
 * +---------------------------------------------------------------------------+
 * | Register |   Bits 31-24  |   Bits 23-16   |    Bits 15-8   |   Bits 7-0   |
 * +----------+---------------+----------------+----------------+--------------+
 * |    00    | Device ID                      |  Vendor ID                    |
 * +----------+--------------------------------+-------------------------------+
 * |    04    | Status                         |  Command                      |
 * +----------+---------------+----------------+-------------------------------+
 * |    08    | Class Code    |  Subclass      |  Register IF   |  Revision ID |
 * +----------+---------------+----------------+----------------+--------------+
 * |    0C    | BIST          | Header type    |Latency Timer  |Cache Line Size|
 * +----------+---------------+----------------+----------------+--------------+
 * |    10    | Base address #0 (BAR0)                                         |
 * +----------+----------------------------------------------------------------+
 * |    14    | Base address #1 (BAR1)                                         |
 * +----------+----------------------------------------------------------------+
 * |    18    | Base address #2 (BAR2)                                         |
 * +----------+----------------------------------------------------------------+
 * |    1C    | Base address #3 (BAR3)                                         |
 * +----------+----------------------------------------------------------------+
 * |    20    | Base address #4 (BAR4)                                         |
 * +----------+----------------------------------------------------------------+
 * |    24    | Base address #5 (BAR5)                                         |
 * +----------+----------------------------------------------------------------+
 * |    28    | Cardbus CIS Pointer                                            |
 * +----------+--------------------------------+-------------------------------+
 * |    2C    | Subsystem ID                   |  Subsystem Vendor ID          |
 * +----------+--------------------------------+-------------------------------+
 * |    30    | Expansion ROM base address                                     |
 * +----------+----------------------------------------------------------------+
 * |    34    | Reserved                                        |Capability Ptr|
 * +----------+----------------------------------------------------------------+
 * |    38    | Reserved                                                       |
 * +----------+---------------+----------------+----------------+--------------+
 * |    3C    | Max Latency   |  Min Grant     | Interrupt PIN  |Interrupt Line|
 * +---------------------------------------------------------------------------+
 *
 *
 * Header Type 0x01 (PCI-to-PCI bridge):
 *
 * +---------------------------------------------------------------------------+
 * | Register |   Bits 31-24   |   Bits 23-16   |   Bits 15-8   |   Bits 7-0   |
 * +----------+----------------+----------------+---------------+--------------+
 * |    00    |  Device ID                      |  Vendor ID                   |
 * +----------+---------------------------------+------------------------------+
 * |    04    |  Status                         |  Command                     |
 * +----------+----------------+----------------+------------------------------+
 * |    08    |  Class Code    |  Subclass                      | Revision ID  |
 * +----------+----------------+----------------+---------------+--------------+
 * |    0C    |  BIST          | Header type    | Latency Timer |CacheLine Size|
 * +----------+----------------+----------------+---------------+--------------+
 * |    10    |  Base address #0 (BAR0)                                        |
 * +----------+----------------------------------------------------------------+
 * |    14    |  Base address #1 (BAR1)                                        |
 * +----------+----------------+----------------+----------------+-------------+
 * |    18    |  Sec Latency   | Subordinate Bus|  Secondary Bus | Primary Bus |
 * +----------+----------------+----------------+----------------+-------------+
 * |    1C    |  Secondary Status               |    I/O Limit   |    I/O Base |
 * +----------+---------------------------------+----------------+-------------+
 * |    20    |  Memory Limit                   |  Memory Base                 |
 * +----------+---------------------------------+------------------------------+
 * |    24    |  Prefetchable Memory Limit      |  Prefetchable Memory Base    |
 * +----------+---------------------------------+------------------------------+
 * |    28    |  Prefetchable Base Upper 32 Bits                               |
 * +----------+----------------------------------------------------------------+
 * |    2C    |  Prefetchable Limit Upper 32 Bits                              |
 * +----------+---------------------------------+------------------------------+
 * |    30    |  I/O Limit Upper 16 Bits        | I/O Base Upper 16 Bits       |
 * +----------+---------------------------------+----------------+-------------+
 * |    34    |  Reserved                                        |CapabilityPtr|
 * +----------+--------------------------------------------------+-------------+
 * |    38    |  Expansion ROM base address                                    |
 * +----------+----------------------------------------------------------------+
 * |    3C    |  Bridge Control                 | Interrupt PIN  |InterruptLine|
 * +---------------------------------------------------------------------------+
 */

union pci_dev {

	/* Header Type 0x00: standard device */

	struct {
/* offset 00:				*/
#ifdef _BIG_ENDIAN
		u32_t device_id
			: 16; /*   device identification		*/
		u32_t vendor_id
			: 16; /*   vendor identification		*/
#else
		u32_t vendor_id
			: 16; /*   vendor identification		*/
		u32_t device_id
			: 16; /*   device identification		*/
#endif

/* offset 04:				*/

#ifdef _BIG_ENDIAN
		u32_t status : 16;  /*   device status */
		u32_t command : 16; /*   device command register	*/
#else
		u32_t command : 16; /*   device command register	*/
		u32_t status : 16;  /*   device status */
#endif

/* offset 08:				*/
#ifdef _BIG_ENDIAN
		u32_t class : 8;
		u32_t subclass : 8;
		u32_t reg_if : 8;
		u32_t revision : 8;
#else
		u32_t revision : 8;
		u32_t reg_if : 8;
		u32_t subclass : 8;
		u32_t class : 8;
#endif

/* offset 0C:				*/
#ifdef _BIG_ENDIAN
		u32_t bist : 8;
		u32_t hdr_type : 8;
		u32_t latency_timer : 8;
		u32_t cache_line : 8;
#else
		u32_t cache_line : 8;
		u32_t latency_timer : 8;
		u32_t hdr_type : 8;
		u32_t bist : 8;
#endif

		u32_t bar0;    /* offset 10: base address register 0	*/
		u32_t bar1;    /* offset 14: base address register 0	*/
		u32_t bar2;    /* offset 18: base address register 0	*/
		u32_t bar3;    /* offset 1C: base address register 0	*/
		u32_t bar4;    /* offset 20: base address register 0	*/
		u32_t bar5;    /* offset 24: base address register 0	*/
		u32_t cardbus; /* offset 28: base address register 0	*/

/* offset 2C:				*/
#ifdef _BIG_ENDIAN
		u32_t subsys_id
			: 16; /*   subsystem identifier		*/
		u32_t subvendor_id
			: 16; /*   subsystem vendor identifier	*/
#else
		u32_t subvendor_id
			: 16; /*   subsystem vendor identifier	*/
		u32_t subsys_id
			: 16; /*   subsystem identifier		*/
#endif

		/* offset 30:				*/
		u32_t rom_address;

/* offset 34:				*/

#ifdef _BIG_ENDIAN
		u32_t reserved1 : 24;
		u32_t capability_ptr : 8;
#else
		u32_t capability_ptr : 8;
		u32_t reserved1 : 24;
#endif

		u32_t reserved2; /* offset 38: */

/* offset 3C:				*/
#ifdef _BIG_ENDIAN
		u32_t max_latency : 8;
		u32_t min_grant : 8;
		u32_t interrupt_pin
			: 8; /*   interrupt pin assignment		*/
		u32_t interrupt_line
			: 8; /*   interrupt line assignment		*/
#else
		u32_t interrupt_line
			: 8; /*   interrupt line assignment		*/
		u32_t interrupt_pin
			: 8; /*   interrupt pin assignment		*/
		u32_t min_grant : 8;
		u32_t max_latency : 8;
#endif
	} field;

	/* Header Type 0x01: PCI-to-PCI bridge */

	struct {
/* offset 00:				*/
#ifdef _BIG_ENDIAN
		u32_t device_id
			: 16; /*   device identification		*/
		u32_t vendor_id
			: 16; /*   vendor identification		*/
#else
		u32_t vendor_id
			: 16; /*   vendor identification		*/
		u32_t device_id
			: 16; /*   device identification		*/
#endif

/* offset 04:				*/
#ifdef _BIG_ENDIAN
		u32_t status : 16; /*   device status */
		u32_t command
			: 16; /*   device command register		*/
#else
		u32_t command
			: 16; /*   device command register		*/
		u32_t status : 16; /*   device status */
#endif

/* offset 08:				*/
#ifdef _BIG_ENDIAN
		u32_t class : 8;
		u32_t subclass : 8;
		u32_t reg_if : 8;
		u32_t revision : 8;
#else
		u32_t revision : 8;
		u32_t reg_if : 8;
		u32_t subclass : 8;
		u32_t class : 8;
#endif

/* offset 0C:				*/
#ifdef _BIG_ENDIAN
		u32_t bist : 8;
		u32_t hdr_type : 8;
		u32_t latency_timer : 8;
		u32_t cache_line : 8;
#else
		u32_t cache_line : 8;
		u32_t latency_timer : 8;
		u32_t hdr_type : 8;
		u32_t bist : 8;
#endif

		u32_t bar0; /* offset 10: base address register 0	*/
		u32_t bar1; /* offset 14: base address register 0	*/

/* offset 18:				*/
#ifdef _BIG_ENDIAN
		u32_t secondary_latency : 8;
		u32_t subord_bus : 8;
		u32_t secondary_bus : 8;
		u32_t primary_bus : 8;
#else
		u32_t primary_bus : 8;
		u32_t secondary_bus : 8;
		u32_t subord_bus : 8;
		u32_t secondary_latency : 8;

#endif

/* offset 1C:				*/
#ifdef _BIG_ENDIAN
		u32_t secondary_status : 16;
		u32_t io_limit : 8;
		u32_t io_base : 8;
#else
		u32_t io_base : 8;
		u32_t io_limit : 8;
		u32_t secondary_status : 16;
#endif

/* offset 20:				*/
#ifdef _BIG_ENDIAN
		u32_t mem_limit : 16;
		u32_t mem_base : 16;
#else
		u32_t mem_base : 16;
		u32_t mem_limit : 16;
#endif

/* offset 24:				*/
#ifdef _BIG_ENDIAN
		u32_t pre_mem_limit : 16;
		u32_t pre_mem_base : 16;
#else
		u32_t pre_mem_base : 16;
		u32_t pre_mem_limit : 16;
#endif

		/* offset 28:				*/
		u32_t pre_mem_base_upper;

		/* offset 2C:				*/
		u32_t pre_mem_limit_upper;

/* offset 30:				*/
#ifdef _BIG_ENDIAN
		u32_t io_limit_upper : 16;
		u32_t io_base_upper : 16;
#else
		u32_t io_base_upper : 16;
		u32_t io_limit_upper : 16;
#endif

/* offset 34:				*/
#ifdef _BIG_ENDIAN
		u32_t reserved : 24;
		u32_t capability_ptr : 8;
#else
		u32_t capability_ptr : 8;
		u32_t reserved : 24;
#endif

		/* offset 38:				*/
		u32_t rom_address;

/* offset 3C:				*/
#ifdef _BIG_ENDIAN
		u32_t bridge_control : 16;
		u32_t interrupt_pin
			: 8; /*   interrupt pin assignment		*/
		u32_t interrupt_line
			: 8; /*   interrupt line assignment		*/
#else
		u32_t interrupt_line
			: 8; /*   interrupt line assignment		*/
		u32_t interrupt_pin
			: 8; /*   interrupt pin assignment		*/
		u32_t bridge_control : 16;
#endif

	} bridge_field;

	/* direct access to each word in the PCI header */

	struct {
		u32_t word0;  /* word  0: offset 00			*/
		u32_t word1;  /* word  1: offset 04			*/
		u32_t word2;  /* word  2: offset 08			*/
		u32_t word3;  /* word  3: offset 0C			*/
		u32_t word4;  /* word  4: offset 10			*/
		u32_t word5;  /* word  5: offset 14			*/
		u32_t word6;  /* word  6: offset 18			*/
		u32_t word7;  /* word  7: offset 1C			*/
		u32_t word8;  /* word  8: offset 20			*/
		u32_t word9;  /* word  9: offset 24			*/
		u32_t word10; /* word 10: offset 28			*/
		u32_t word11; /* word 11: offset 2C			*/
		u32_t word12; /* word 12: offset 30			*/
		u32_t word13; /* word 13: offset 34			*/
		u32_t word14; /* word 14: offset 38			*/
		u32_t word15; /* word 15: offset 3C			*/
	} word;

	struct {
		/* array of words for the header */
		u32_t word[PCI_HEADER_WORDS];
	} words;

};

/*
 * Generic Capability register set header:
 *
 * +---------------------------------------------------------------------------+
 * | Register |   Bits 31-24   |   Bits 23-16   |   Bits 15-8   |  Bits 7-0    |
 * +----------+----------------+----------------+---------------+--------------+
 * |    00    |  Capability specific data       | Next Pointer  |   Cap ID     |
 * +---------------------------------------------------------------------------+
 */

union pci_cap_hdr {
	struct {
		/* offset 00:				*/
		u32_t id : 8; /*   capability ID			*/
		u32_t next_ptr
			: 8; /*   pointer to next capability		*/
		u32_t feature
			: 16; /*   capability specific field		*/
	} field;

	u32_t word; /* array of words for the header	*/
} pci_cap_hdr_t;

union pcie_cap_hdr {
	struct {
		/* offset 00:				*/
		u32_t id : 16;     /*   capability ID			*/
		u32_t version : 4; /*   version		*/
		u32_t next_ptr
			: 12; /*   pointer to next capability		*/
	} field;

	u32_t word; /* array of words for the header	*/
};

/*
 * MSI Capability register set (32-bit):
 *
 * +---------------------------------------------------------------------------+
 * |Register|   Bits 31-24   |   Bits 23-16   |    Bits 15-8   |   Bits 7-0    |
 * +--------+----------------+----------------+----------------+---------------+
 * |    00  |  Message Control Register       |  Next Pointer  |    Cap ID     |
 * +--------+---------------------------------+----------------+---------------+
 * |    04  |                      Message Address Register                0 0 |
 * +--------+---------------------------------+--------------------------------+
 * |    0C  |                                 |      Message Data Register     |
 * +---------------------------------------------------------------------------+
 *
 * MSI Capability register set (64-bit):
 *
 * +---------------------------------------------------------------------------+
 * |Register|   Bits 31-24   |   Bits 23-16   |    Bits 15-8   |   Bits 7-0    |
 * +--------+----------------+----------------+----------------+---------------+
 * |    00  |  Message Control Register       |  Next Pointer  |    Cap ID     |
 * +--------+---------------------------------+----------------+---------------+
 * |    04  |  Least-Significant 32-bits of Message Address Register       0 0 |
 * +--------+--------------------------------------------------+---------------+
 * |    08  |  Most-Significant 32-bits of Message Address Register            |
 * +--------+---------------------------------+--------------------------------+
 * |    0C  |                                 |      Message Data Register     |
 * +---------------------------------------------------------------------------+
 */

struct _pci_msi_hdr {
	/* common MSI header */
	union {
		struct {
						/* offset 00:		      */
			u32_t id : 8;	/* capability ID              */
			u32_t next_ptr : 8;	/* pointer to next capability */
			u32_t enabled : 1;	/* MSI enabled		      */
			u32_t msg_req : 3;	/* requested message count    */
			u32_t msg_grant : 3; /* granted message count      */
			u32_t is_64_bit : 1; /* 64-bit capable             */
			u32_t reserved : 8;	/*			      */
		} msi_cap;

		struct {
						/* offset 00:		      */
			u32_t id : 8;	/* capability ID              */
			u32_t next_ptr : 8;	/* pointer to next capability */
			u32_t table_size : 11; /* MSI-x table size         */
			u32_t reserved : 3;	/*		              */
			u32_t func_mask : 1; /* 1 for vectors masked	      */
			u32_t enabled : 1;	/* MSI-x enabled              */
		} msix_cap;
	} cap;

	union {
		/* 32-bit MSI header */
		struct {
						/* offset 04:		      */
			u32_t addr;          /* message address register   */
						/* offset 08		      */
			u32_t data : 16;	/*   message data register    */
			u32_t spare : 16;	/*			      */
		} regs32;
		/* 64-bit MSI header */
		struct {
						/* offset 04:		      */
			u32_t addr_low;	/*  message address register
						 *  (lower)
						 */
						/* offset 08:		      */
			u32_t addr_high;	/*   message address register
						 * (upper)
						 */
						/* offset 0C:		      */
			u32_t data : 16;	/*  message data register     */
			u32_t spare : 16;	/*			      */
		} regs64;
	} regs;
};

union pci_msi_hdr {
	struct _pci_msi_hdr field; /* MSI header fields		  */
	u32_t word[4];   /* array of words for the header	  */
};

/*
 * number of pci controllers; initialized to 0 until the controllers have been
 * created
 */
extern u32_t pci_controller_cnt;

struct pci_msix_table {
	u32_t msg_addr;
	u32_t msg_addr_high;
	u32_t msg_data;
	u32_t vec_ctrl;
};

struct pci_msix_entry {
	u32_t vector; /* guest to write a vector */
	u16_t entry;  /* driver to specify entry, guest OS writes */
};

struct pci_msix_info {
	u32_t bus_no;
	u32_t dev_no;
	u32_t func_no;
	u32_t msix_vec;
};

/* manager interface API */

extern void pci_read(u32_t controller,
		    union pci_addr_reg addr,
		    u32_t size,
		    u32_t *data);
extern void pci_write(u32_t controller,
		     union pci_addr_reg addr,
		     u32_t size,
		     u32_t data);

extern void pci_header_get(u32_t controller,
			 union pci_addr_reg pci_ctrl_addr,
			 union pci_dev *pci_dev_header);

/* General PCI configuration access routines */

extern void pci_config_out_long(u32_t bus,    /* bus number */
			     u32_t dev,    /* device number */
			     u32_t func,   /* function number */
			     u32_t offset, /* offset into the configuration
					       * space
					       */
			     u32_t data /* data written to the offset */
			     );

extern void pci_config_out_word(u32_t bus,    /* bus number */
			     u32_t dev,    /* device number */
			     u32_t func,   /* function number */
			     u32_t offset, /* offset into the configuration
					       * space
					       */
			     u16_t data /* data written to the offset */
			     );

extern void pci_config_out_byte(u32_t bus,    /* bus number */
			     u32_t dev,    /* device number */
			     u32_t func,   /* function number */
			     u32_t offset, /* offset into the configuration
					       * space
					       */
			     u8_t data /* data written to the offset */
			     );

extern void pci_config_in_long(u32_t bus,    /* bus number */
			    u32_t dev,    /* device number */
			    u32_t func,   /* function number */
			    u32_t offset, /* offset into the configuration
					      * space
					      */
			    u32_t *data /* return value address */
			    );

extern void pci_config_in_word(u32_t bus,    /* bus number */
			    u32_t dev,    /* device number */
			    u32_t func,   /* function number */
			    u32_t offset, /* offset into the configuration
					      * space
					      */
			    u16_t *data /* return value address */
			    );

extern void pci_config_in_byte(u32_t bus,    /* bus number */
			    u32_t dev,    /* device number */
			    u32_t func,   /* function number */
			    u32_t offset, /* offset into the configuration
					      * space
					      */
			    u8_t *data /* return value address */
			    );

extern int pci_config_ext_cap_ptr_find(
	u8_t ext_cap_find_id, /* Extended capabilities ID to search for */
	u32_t bus,	 /* PCI bus number */
	u32_t device,      /* PCI device number */
	u32_t function,    /* PCI function number */
	u8_t *p_offset      /* returned config space offset */
	);

#endif /* _ASMLANGUAGE */

#define PCI_CMD 4 /* command register */

/* PCI command bits */

#define PCI_CMD_IO_ENABLE 0x0001     /* IO access enable */
#define PCI_CMD_MEM_ENABLE 0x0002    /* memory access enable */
#define PCI_CMD_MASTER_ENABLE 0x0004 /* bus master enable */
#define PCI_CMD_MON_ENABLE 0x0008    /* monitor special cycles enable */
#define PCI_CMD_WI_ENABLE 0x0010     /* write and invalidate enable */
#define PCI_CMD_SNOOP_ENABLE 0x0020  /* palette snoop enable */
#define PCI_CMD_PERR_ENABLE 0x0040   /* parity error enable */
#define PCI_CMD_WC_ENABLE 0x0080     /* wait cycle enable */
#define PCI_CMD_SERR_ENABLE 0x0100   /* system error enable */
#define PCI_CMD_FBTB_ENABLE 0x0200   /* fast back to back enable */
#define PCI_CMD_INTX_DISABLE 0x0400  /* INTx disable */

/* PCI status bits */

#define PCI_STATUS_NEW_CAP 0x0010
#define PCI_STATUS_66_MHZ 0x0020
#define PCI_STATUS_UDF 0x0040
#define PCI_STATUS_FAST_BB 0x0080
#define PCI_STATUS_DATA_PARITY_ERR 0x0100
#define PCI_STATUS_TARGET_ABORT_GEN 0x0800
#define PCI_STATUS_TARGET_ABORT_RCV 0x1000
#define PCI_STATUS_MASTER_ABORT_RCV 0x2000
#define PCI_STATUS_ASSERT_SERR 0x4000
#define PCI_STATUS_PARITY_ERROR 0x8000

/* Standard device Type 0 configuration register offsets */
/* Note that only modulo-4 addresses are written to the address register */

#define PCI_CFG_VENDOR_ID 0x00
#define PCI_CFG_DEVICE_ID 0x02
#define PCI_CFG_COMMAND 0x04
#define PCI_CFG_STATUS 0x06
#define PCI_CFG_REVISION 0x08
#define PCI_CFG_PROGRAMMING_IF 0x09
#define PCI_CFG_SUBCLASS 0x0a
#define PCI_CFG_CLASS 0x0b
#define PCI_CFG_CACHE_LINE_SIZE 0x0c
#define PCI_CFG_LATENCY_TIMER 0x0d
#define PCI_CFG_HEADER_TYPE 0x0e
#define PCI_CFG_BIST 0x0f
#define PCI_CFG_BASE_ADDRESS_0 0x10
#define PCI_CFG_BASE_ADDRESS_1 0x14
#define PCI_CFG_BASE_ADDRESS_2 0x18
#define PCI_CFG_BASE_ADDRESS_3 0x1c
#define PCI_CFG_BASE_ADDRESS_4 0x20
#define PCI_CFG_BASE_ADDRESS_5 0x24
#define PCI_CFG_CIS 0x28
#define PCI_CFG_SUB_VENDER_ID 0x2c
#define PCI_CFG_SUB_SYSTEM_ID 0x2e
#define PCI_CFG_EXPANSION_ROM 0x30
#define PCI_CFG_CAP_PTR 0x34
#define PCI_CFG_RESERVED_0 0x35
#define PCI_CFG_RESERVED_1 0x38
#define PCI_CFG_DEV_INT_LINE 0x3c
#define PCI_CFG_DEV_INT_PIN 0x3d
#define PCI_CFG_MIN_GRANT 0x3e
#define PCI_CFG_MAX_LATENCY 0x3f
#define PCI_CFG_SPECIAL_USE 0x41
#define PCI_CFG_MODE 0x43
#define PCI_NUM_BARS 6 /* Number of BARs */

/* PCI base address mask bits */

#define PCI_MEMBASE_MASK ~0xf  /* mask for memory base address */
#define PCI_IOBASE_MASK ~0x3   /* mask for IO base address */
#define PCI_BASE_IO 0x1	/* IO space indicator */
#define PCI_BASE_BELOW_1M 0x2  /* memory locate below 1MB */
#define PCI_BASE_IN_64BITS 0x4 /* memory locate anywhere in 64 bits */
#define PCI_BASE_PREFETCH 0x8  /* memory prefetchable */

/* Base Address Register Memory/IO Attribute bits */

#define PCI_BAR_MEM_TYPE_MASK (0x06)
#define PCI_BAR_MEM_ADDR32 (0x00)
#define PCI_BAR_MEM_BELOW_1MB (0x02)
#define PCI_BAR_MEM_ADDR64 (0x04)
#define PCI_BAR_MEM_RESERVED (0x06)

#define PCI_BAR_MEM_PREF_MASK (0x08)
#define PCI_BAR_MEM_PREFETCH (0x08)
#define PCI_BAR_MEM_NON_PREF (0x00)

#define PCI_BAR_ALL_MASK \
	(PCI_BAR_SPACE_MASK | PCI_BAR_MEM_TYPE_MASK | PCI_BAR_MEM_PREF_MASK)

#define PCI_BAR_SPACE_MASK (0x01)
#define PCI_BAR_SPACE_IO (0x01)
#define PCI_BAR_SPACE_MEM (0x00)
#define PCI_BAR_SPACE_NONE PCI_BAR_MEM_RESERVED

#define PCI_BAR_NONE 0
#define PCI_BAR_IO 0x1
#define PCI_BAR_MEM 0x2

#define PCI_EXP_2_CAP 0x24	 /* Dev 2 cap */
#define PCI_EXP_2_CTRL 0x28	/* Dev 2 control */
#define PCI_EXP_2_STATUS 0x2a      /* Dev 2 status */
#define PCI_EXP_2_LINK_CAP 0x2c    /* Dev 2 Link cap */
#define PCI_EXP_2_LINK_CTRL 0x30   /* Dev 2 Link control */
#define PCI_EXP_2_LINK_STATUS 0x32 /* Dev 2 Link status */
#define PCI_EXP_2_SLOT_CAP 0x34    /* Dev 2 Slot cap */
#define PCI_EXP_2_SLOT_CTRL 0x38   /* Dev 2 slot control */
#define PCI_EXP_2_SLOT_STATUS 0x3a /* Dev 2 slot status */

#define PCI_CFG_SPACE_SIZE 0x100
#define PCIE_CFG_SPACE_SIZE 0x1000
#define PCIE_CFG_SPACE_TOTAL ((PCIE_CFG_SPACE_SIZE - PCI_CFG_SPACE_SIZE) / 4)

/* Extended Capabilities (PCI-X 2.0 and Express) */
#define PCIE_EXT_CAP_ID(header) (header & 0x0000ffff)
#define PCIE_EXT_CAP_VER(header) ((header >> 16) & 0xf)
#define PCIE_EXT_CAP_NEXT(header) ((header >> 20) & 0xffc)

#define PCIE_EXT_CAP_ID_ERR 1
#define PCIE_EXT_CAP_ID_VC 2
#define PCIE_EXT_CAP_ID_DSN 3
#define PCIE_EXT_CAP_ID_PWR 4
#define PCIE_EXT_CAP_ID_ARI 14
#define PCIE_EXT_CAP_ID_ATS 15
#define PCIE_EXT_CAP_ID_SRIOV 16

/* Advanced Error Reporting */

#define PCIE_ERR_UNCOR_STATUS 4			 /* Uncorrectable Error Status*/
#define PCIE_ERR_UNC_TRAIN 0x00000001		 /* Training */
#define PCIE_ERR_UNC_DLP 0x00000010		 /* Data Link Protocol */
#define PCIE_ERR_UNC_POISON_TLP 0x00001000       /* Poisoned TLP */
#define PCIE_ERR_UNC_FCP 0x00002000		 /* Flow Control Protocol */
#define PCIE_ERR_UNC_COMP_TIME 0x00004000	/* Completion Timeout */
#define PCIE_ERR_UNC_COMP_ABORT 0x00008000       /* Completer Abort */
#define PCIE_ERR_UNC_UNX_COMP 0x00010000	 /* Unexpected Completion */
#define PCIE_ERR_UNC_RX_OVER 0x00020000		 /* Receiver Overflow */
#define PCIE_ERR_UNC_MALF_TLP 0x00040000	 /* Malformed TLP */
#define PCIE_ERR_UNC_ECRC 0x00080000		 /* ECRC Error Status */
#define PCIE_ERR_UNC_UNSUP 0x00100000		 /* Unsupported Request */
#define PCIE_ERR_UNCOR_MASK 8			 /* Uncorrectable Error Mask */
						 /* Same bits as above */
#define PCIE_ERR_UNCOR_SEVER 12			 /* Uncorrectable Erro Sev. */
						 /* Same bits as above */
#define PCIE_ERR_COR_STATUS 16			 /* Correctable Error Status */
#define PCIE_ERR_COR_RCVR 0x00000001		 /* Receiver Error Status */
#define PCIE_ERR_COR_BAD_TLP 0x00000040		 /* Bad TLP Status */
#define PCIE_ERR_COR_BAD_DLLP 0x00000080	 /* Bad DLLP Status */
#define PCIE_ERR_COR_REP_ROLL 0x00000100	 /* REPLAY_NUM Rollover */
#define PCIE_ERR_COR_REP_TIMER 0x00001000	/* Replay Timer Timeout */
#define PCIE_ERR_COR_MASK 20			 /* Correctable Error Mask */
						 /* Same bits as above */
#define PCIE_ERR_CAP 24				 /* Advanced Error Cap */
#define PCIE_ERR_CAP_FEP(x) ((x) & 31)		 /* First Error Pointer */
#define PCIE_ERR_CAP_ECRC_GENC 0x00000020	/* ECRC Generation Capable */
#define PCIE_ERR_CAP_ECRC_GENE 0x00000040	/* ECRC Generation Enable */
#define PCIE_ERR_CAP_ECRC_CHKC 0x00000080	/* ECRC Check Capable */
#define PCIE_ERR_CAP_ECRC_CHKE 0x00000100	/* ECRC Check Enable */
#define PCIE_ERR_HEADER_LOG 28			 /* Header Log Reg (16 bytes) */
#define PCIE_ERR_ROOT_COMMAND 44		 /* Root Error Command */
#define PCIE_ERR_ROOT_CMD_COR_EN 0x00000001      /* Correctable Err Report */
#define PCIE_ERR_ROOT_CMD_NONFATAL_EN 0x00000002 /* Non-fatal Err Reporting */
#define PCIE_ERR_ROOT_CMD_FATAL_EN 0x00000004    /* Fatal Err Reporting En */
#define PCIE_ERR_ROOT_STATUS 48
#define PCIE_ERR_ROOT_COR_RCV 0x00000001	 /* ERR_COR Received */
#define PCIE_ERR_ROOT_MULTI_COR_RCV 0x00000002   /* Multi ERR_COR Received */
#define PCIE_ERR_ROOT_UNCOR_RCV 0x00000004       /* ERR_FATAL/NONFATAL Rcv */
#define PCIE_ERR_ROOT_MULTI_UNCOR_RCV 0x00000008 /* Multi ERR_FATAL/NONFATAL*/
#define PCIE_ERR_ROOT_FIRST_FATAL 0x00000010     /* First Fatal */
#define PCIE_ERR_ROOT_NONFATAL_RCV 0x00000020    /* Non-Fatal Received */
#define PCIE_ERR_ROOT_FATAL_RCV 0x00000040       /* Fatal Received */
#define PCIE_ERR_ROOT_COR_SRC 52
#define PCIE_ERR_ROOT_SRC 54

/* Virtual Channel */

#define PCIE_VC_PORT_REG1 4
#define PCIE_VC_PORT_REG2 8
#define PCIE_VC_PORT_CTRL 12
#define PCIE_VC_PORT_STATUS 14
#define PCIE_VC_RES_CAP 16
#define PCIE_VC_RES_CTRL 20
#define PCIE_VC_RES_STATUS 26

/* Power Budgeting */

#define PCIE_PWR_DSR 4				    /* Data Select Register */
#define PCIE_PWR_DATA 8				    /* Data Register */
#define PCIE_PWR_DATA_BASE(x) ((x) & 0xff)	  /* Base Power */
#define PCIE_PWR_DATA_SCALE(x) (((x) >> 8) & 3)     /* Data Scale */
#define PCIE_PWR_DATA_PM_SUB(x) (((x) >> 10) & 7)   /* PM Sub State */
#define PCIE_PWR_DATA_PM_STATE(x) (((x) >> 13) & 3) /* PM State */
#define PCIE_PWR_DATA_TYPE(x) (((x) >> 15) & 7)     /* Type */
#define PCIE_PWR_DATA_RAIL(x) (((x) >> 18) & 7)     /* Power Rail */
#define PCIE_PWR_CAP 12				    /* Capability */
#define PCIE_PWR_CAP_BUDGET(x) ((x) & 1)	    /* Included in sys budget */

/*
 * Hypertransport sub capability types
 *
 * Unfortunately there are both 3 bit and 5 bit capability types defined
 * in the HT spec, catering for that is a little messy. You probably don't
 * want to use these directly, just use pci_find_ht_capability() and it
 * will do the right thing for you.
 */

#define HT_3BIT_CAP_MASK 0xE0
#define HT_CAPTYPE_SLAVE 0x00 /* Slave/Primary link configuration */
#define HT_CAPTYPE_HOST 0x20  /* Host/Secondary link configuration */

#define HT_5BIT_CAP_MASK 0xF8
#define HT_CAPTYPE_IRQ 0x80	  /* IRQ Configuration */
#define HT_CAPTYPE_REMAPPING_40 0xA0 /* 40 bit address remapping */
#define HT_CAPTYPE_REMAPPING_64 0xA2 /* 64 bit address remapping */
#define HT_CAPTYPE_UNITID_CLUMP 0x90 /* Unit ID clumping */
#define HT_CAPTYPE_EXTCONF 0x98      /* Extended Configuration Space Access*/
#define HT_CAPTYPE_MSI_MAPPING 0xA8  /* MSI Mapping Capability */
#define HT_MSI_FLAGS 0x02	    /* Offset to flags */
#define HT_MSI_FLAGS_ENABLE 0x1      /* Mapping enable */
#define HT_MSI_FLAGS_FIXED 0x2       /* Fixed mapping only */
				     /* Fixed addr */
#define HT_MSI_FIXED_ADDR 0x00000000FEE00000ULL
#define HT_MSI_ADDR_LO 0x04 /* Offset to low addr bits */
				/* Low address bit mask */
#define HT_MSI_ADDR_LO_MASK 0xFFF00000
#define HT_MSI_ADDR_HI 0x08	  /* Offset to high addr bits */
#define HT_CAPTYPE_DIRECT_ROUTE 0xB0 /* Direct routing configuration */
#define HT_CAPTYPE_VCSET 0xB8	/* Virtual Channel configuration */
#define HT_CAPTYPE_ERROR_RETRY 0xC0  /* Retry on error configuration */
#define HT_CAPTYPE_GEN3 0xD0	 /* Generation 3 hypertransport config */
#define HT_CAPTYPE_PM 0xE0	   /* Hypertransport powermanagement cfg */

/* Alternative Routing-ID Interpretation */
#define PCIE_ARI_CAP 0x04	/* ARI Capability Register */
#define PCIE_ARI_CAP_MFVC 0x0001 /* MFVC Function Groups Capability */
#define PCIE_ARI_CAP_ACS 0x0002  /* ACS Function Groups Capability */
				 /* Next Function Number */
#define PCIE_ARI_CAP_NFN(x) (((x) >> 8) & 0xff)
#define PCIE_ARI_CTRL 0x06	/* ARI Control Register */
#define PCIE_ARI_CTRL_MFVC 0x0001 /* MFVC Function Groups Enable */
#define PCIE_ARI_CTRL_ACS 0x0002  /* ACS Function Groups Enable */
				  /* Function Group */
#define PCIE_ARI_CTRL_FG(x) (((x) >> 4) & 7)

/* Address Translation Service */
#define PCIE_ATS_CAP 0x04 /* ATS Capability Register */
			  /* Invalidate Queue Depth */
#define PCIE_ATS_CAP_QDEP(x) ((x) & 0x1f)
#define PCIE_ATS_MAX_QDEP 32	/* Max Invalidate Queue Depth */
#define PCIE_ATS_CTRL 0x06	  /* ATS Control Register */
#define PCIE_ATS_CTRL_ENABLE 0x8000 /* ATS Enable */
					/* Smallest Translation Unit */
#define PCIE_ATS_CTRL_STU(x) ((x) & 0x1f)
#define PCIE_ATS_MIN_STU 12 /* shift of minimum STU block */

/* Single Root I/O Virtualization */
#define PCIE_SRIOV_CAP 0x04     /* SR-IOV Capabilities */
#define PCIE_SRIOV_CAP_VFM 0x01 /* VF Migration Capable */
				/* Interrupt Message Number */
#define PCIE_SRIOV_CAP_INTR(x) ((x) >> 21)
#define PCIE_SRIOV_CTRL 0x08       /* SR-IOV Control */
#define PCIE_SRIOV_CTRL_VFE 0x01   /* VF Enable */
#define PCIE_SRIOV_CTRL_VFM 0x02   /* VF Migration Enable */
#define PCIE_SRIOV_CTRL_INTR 0x04  /* VF Migration Interrupt Enable */
#define PCIE_SRIOV_CTRL_MSE 0x08   /* VF Memory Space Enable */
#define PCIE_SRIOV_CTRL_ARI 0x10   /* ARI Capable Hierarchy */
#define PCIE_SRIOV_STATUS 0x0a     /* SR-IOV Status */
#define PCIE_SRIOV_STATUS_VFM 0x01 /* VF Migration Status */
#define PCIE_SRIOV_INITIAL_VF 0x0c /* Initial VFs */
#define PCIE_SRIOV_TOTAL_VF 0x0e   /* Total VFs */
#define PCIE_SRIOV_NUM_VF 0x10     /* Number of VFs */
#define PCIE_SRIOV_FUNC_LINK 0x12  /* Function Dependency Link */
#define PCIE_SRIOV_VF_OFFSET 0x14  /* First VF Offset */
#define PCIE_SRIOV_VF_STRIDE 0x16  /* Following VF Stride */
#define PCIE_SRIOV_VF_DID 0x1a     /* VF Device ID */
#define PCIE_SRIOV_SUP_PGSIZE 0x1c /* Supported Page Sizes */
#define PCIE_SRIOV_SYS_PGSIZE 0x20 /* System Page Size */
#define PCIE_SRIOV_BAR0 0x24       /* VF BAR0 */
#define PCIE_SRIOV_BAR0_UPPER 0x28 /* VF BAR0 upper*/
#define PCIE_SRIOV_BAR3 0x30       /* VF BAR3 */
#define PCIE_SRIOV_BAR3_UPPER 0x34 /* VF BAR3 upper */

#define PCIE_SRIOV_NUM_BARS 6 /* Number of VF BARs */
#define PCIE_SRIOV_VFM 0x3c   /* VF Migration State Array Offset*/
			      /* State BIR */
#define PCIE_SRIOV_VFM_BIR(x) ((x) & 7)
/* State Offset */
#define PCIE_SRIOV_VFM_OFFSET(x) ((x) & ~7)
#define PCIE_SRIOV_VFM_UA 0x0 /* Inactive.Unavailable */
#define PCIE_SRIOV_VFM_MI 0x1 /* Dormant.MigrateIn */
#define PCIE_SRIOV_VFM_MO 0x2 /* Active.MigrateOut */
#define PCIE_SRIOV_VFM_AV 0x3 /* Active.Available */

/* Extended Capabilities (PCI-X 2.0 and Express) */

#define PCI_EXT_CAP_ID(header) (header & 0x0000ffff)
#define PCI_EXT_CAP_VER(header) ((header >> 16) & 0xf)
#define PCI_EXT_CAP_NEXT(header) ((header >> 20) & 0xffc)

#define PCI_MSIX_ENTRY_SIZE 16
#define PCI_MSIX_ENTRY_LOWER_ADDR 0
#define PCI_MSIX_ENTRY_UPPER_ADDR 4
#define PCI_MSIX_ENTRY_DATA 8
#define PCI_MSIX_ENTRY_VECTOR_CTRL 12
#define PCI_MSIX_ENTRY_VECTOR_MASKED 0x1

/* PCI-to-PCI bridge Type 1 configuration register offsets */
/* Note that only modulo-4 addresses are written to the address register */

#define PCI_CFG_PRIMARY_BUS 0x18
#define PCI_CFG_SECONDARY_BUS 0x19
#define PCI_CFG_SUBORDINATE_BUS 0x1a
#define PCI_CFG_SEC_LATENCY 0x1b
#define PCI_CFG_IO_BASE 0x1c
#define PCI_CFG_IO_LIMIT 0x1d
#define PCI_CFG_SEC_STATUS 0x1e
#define PCI_CFG_MEM_BASE 0x20
#define PCI_CFG_MEM_LIMIT 0x22
#define PCI_CFG_PRE_MEM_BASE 0x24
#define PCI_CFG_PRE_MEM_LIMIT 0x26
#define PCI_CFG_PRE_MEM_BASE_U 0x28
#define PCI_CFG_PRE_MEM_LIMIT_U 0x2c
#define PCI_CFG_IO_BASE_U 0x30
#define PCI_CFG_IO_LIMIT_U 0x32
#define PCI_CFG_ROM_BASE 0x38
#define PCI_CFG_BRG_INT_LINE 0x3c
#define PCI_CFG_BRG_INT_PIN 0x3d
#define PCI_CFG_BRIDGE_CONTROL 0x3e

/* PCI-to-CardBus bridge Type 2 configuration register offsets */

#define PCI_CFG_CB_CAP_PTR 0x14
/*  0x15 - reserved */
#define PCI_CFG_CB_SEC_STATUS 0x16
#define PCI_CFG_CB_PRIMARY_BUS 0x18     /* PCI bus no. */
#define PCI_CFG_CB_BUS 0x19		/* CardBus bus no */
#define PCI_CFG_CB_SUBORDINATE_BUS 0x1a /* Subordinate bus no. */
#define PCI_CFG_CB_LATENCY_TIMER 0x1b   /* CardBus latency timer */
#define PCI_CFG_CB_MEM_BASE_0 0x1c
#define PCI_CFG_CB_MEM_LIMIT_0 0x20
#define PCI_CFG_CB_MEM_BASE_1 0x24
#define PCI_CFG_CB_MEM_LIMIT_1 0x28
#define PCI_CFG_CB_IO_BASE_0 0x2c
#define PCI_CFG_CB_IO_BASE_0_HI 0x2e
#define PCI_CFG_CB_IO_LIMIT_0 0x30
#define PCI_CFG_CB_IO_LIMIT_0_HI 0x32
#define PCI_CFG_CB_IO_BASE_1 0x34
#define PCI_CFG_CB_IO_BASE_1_HI 0x36
#define PCI_CFG_CB_IO_LIMIT_1 0x38
#define PCI_CFG_CB_IO_LIMIT_1_HI 0x3a
#define PCI_CFG_CB_BRIDGE_CONTROL 0x3e
#define PCI_CFG_CB_SUB_VENDOR_ID 0x40
#define PCI_CFG_CB_SUB_SYSTEM_ID 0x42
#define PCI_CFG_CB_16BIT_LEGACY 0x44
/* 0x48 - 0x7f are reserved */

/* PCI Bridge Control Register (0x3E) bits */

#define PCI_CFG_PARITY_ERROR 0x01  /* Enable parity detection */
#define PCI_CFG_SERR 0x02	  /* SERR enable */
#define PCI_CFG_ISA_ENABLE 0x04    /* ISA Disable - bit set = disable*/
#define PCI_CFG_VGA_ENABLE 0x08    /* Enable VGA addresses */
#define PCI_CFG_MASTER_ABORT 0x20  /* Signal master abort */
#define PCI_CFG_SEC_BUS_RESET 0x40 /* secondary bus reset */
#define PCI_CFG_FAST_BACK 0x80     /* FBB enabled on secondary */
#define PCI_CFG_PRI_DIS_TO 0x100 /* Primary Discard Timeout: *2^10 PCI cycles */
#define PCI_CFG_SEC_DIS_TO 0x200 /* 2ndary Discard Timeout: * 2^10 PCI cycles */
#define PCI_CFG_DIS_TIMER_STAT 0x400   /* Discard Timer status */
#define PCI_CFG_DIS_TIMER_ENABLE 0x800 /* Discard Timer enable */

/* Cardbus Bridge Control Register (0x3E) bits */

#define PCI_CFG_CB_PARITY_ERROR 0x01  /* Enable parity detection */
#define PCI_CFG_CB_SERR 0x02	  /* SERR enable */
#define PCI_CFG_CB_ISA_ENABLE 0x04    /* ISA Disable - bit set = disable*/
#define PCI_CFG_CB_VGA_ENABLE 0x08    /* Enable VGA addresses */
#define PCI_CFG_CB_MASTER_ABORT 0x20  /* Signal master abort */
#define PCI_CFG_CB_RESET 0x40	 /* Cardbus reset */
#define PCI_CFG_CB_16BIT_INT 0x80     /* Enable ints for 16-bit cards */
#define PCI_CFG_CB_PREFETCH0 0x0100   /* Memory 0 prefetch enable */
#define PCI_CFG_CB_PREFETCH1 0x0200   /* Memory 1 prefetch enable */
#define PCI_CFG_CB_POST_WRITES 0x0400 /* Posted Writes */

/* Power Management registers */

#define PCI_PM_CTRL 4		  /* PM control register */
#define PCI_PM_CTRL_NO_RST 0x0008 /* No reset issued by d3-d0 */

#define PCI_PSTATE_MASK 0x0003 /* Power state bits */
#define PCI_PSTATE_D0 0x0000
#define PCI_PSTATE_D1 0x0001
#define PCI_PSTATE_D2 0x0002
#define PCI_PSTATE_D3_HOT 0x0003
#define PCI_PSTATE_D3_COLD 0x0004

/* Advanced Features */

#define PCI_AF_CAP 3
#define PCI_AF_CAP_TRPND 0x01
#define PCI_AF_CAP_FLR 0x02
#define PCI_AF_CTRL 4
#define PCI_AF_CTRL_FLR 0x01
#define PCI_AF_STATUS 5
#define PCI_AF_STATUS_TRPND 0x01

/* CompactPCI Hot Swap Control & Status Register (HSCSR) defines */

#define PCI_HS_CSR_RSVD0 0x01 /* Reserved */
#define PCI_HS_CSR_EIM 0x02   /* ENUM Interrupt Mask */
#define PCI_HS_CSR_RSVD2 0x04 /* Reserved */
#define PCI_HS_CSR_LOO 0x08   /* Blue LED On/Off */
#define PCI_HS_CSR_RSVD4 0x10 /* Reserved */
#define PCI_HS_CSR_RSVD5 0x20 /* Reserved */
#define PCI_HS_CSR_EXT 0x40   /* ENUM Status - EXTract */
#define PCI_HS_CSR_INS 0x80   /* ENUM Status - INSert */

/* PCI Standard Classifications */

/*
 * PCI classifications are composed from the concatenation of four byte-size
 * components: primary (base) class, sub-class, register interface, and
 * revision ID.  The following comprise the standard PCI classification
 * definitions.
 */

/*
 * PCI Primary (Base) Class definitions for find by class function
 * Classes 0x12 - 0xFE are reserved for future enhancements
 */

#define PCI_CLASS_PRE_PCI20 0x00
#define PCI_CLASS_MASS_STORAGE 0x01
#define PCI_CLASS_NETWORK_CTLR 0x02
#define PCI_CLASS_DISPLAY_CTLR 0x03
#define PCI_CLASS_MMEDIA_DEVICE 0x04
#define PCI_CLASS_MEM_CTLR 0x05
#define PCI_CLASS_BRIDGE_CTLR 0x06
#define PCI_CLASS_COMM_CTLR 0x07
#define PCI_CLASS_BASE_PERIPH 0x08
#define PCI_CLASS_INPUT_DEVICE 0x09
#define PCI_CLASS_DOCK_DEVICE 0x0A
#define PCI_CLASS_PROCESSOR 0x0B
#define PCI_CLASS_SERIAL_BUS 0x0C
#define PCI_CLASS_WIRELESS 0x0D
#define PCI_CLASS_INTLGNT_IO 0x0E
#define PCI_CLASS_SAT_COMM 0x0F
#define PCI_CLASS_EN_DECRYPTION 0x10
#define PCI_CLASS_DAQ_DSP 0x11
#define PCI_CLASS_UNDEFINED 0xFF

/* PCI Subclass definitions */

#define PCI_SUBCLASS_00 0x00
#define PCI_SUBCLASS_01 0x01
#define PCI_SUBCLASS_02 0x02
#define PCI_SUBCLASS_03 0x03
#define PCI_SUBCLASS_04 0x04
#define PCI_SUBCLASS_05 0x05
#define PCI_SUBCLASS_06 0x06
#define PCI_SUBCLASS_07 0x07
#define PCI_SUBCLASS_08 0x08
#define PCI_SUBCLASS_09 0x09
#define PCI_SUBCLASS_0A 0x0A
#define PCI_SUBCLASS_10 0x10
#define PCI_SUBCLASS_11 0x11
#define PCI_SUBCLASS_12 0x12
#define PCI_SUBCLASS_20 0x20
#define PCI_SUBCLASS_40 0x40
#define PCI_SUBCLASS_80 0x80
#define PCI_SUBCLASS_UNDEFINED 0xFF

/* Base Class 00 are Rev 1.0 and are not defined here. */

/* Mass Storage subclasses - Base Class 01h */

#define PCI_SUBCLASS_MASS_SCSI (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_MASS_IDE (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_MASS_FLOPPY (PCI_SUBCLASS_02)
#define PCI_SUBCLASS_MASS_IPI (PCI_SUBCLASS_03)
#define PCI_SUBCLASS_MASS_RAID (PCI_SUBCLASS_04)
#define PCI_SUBCLASS_MASS_ATA (PCI_SUBCLASS_05)
#define PCI_REG_IF_ATA_SNGL 0x20
#define PCI_REG_IF_ATA_CHND 0x30
#define PCI_SUBCLASS_MASS_OTHER (PCI_SUBCLASS_80)

/* Network subclasses - Base Class 02h */

#define PCI_SUBCLASS_NET_ETHERNET (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_NET_TOKEN_RING (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_NET_FDDI (PCI_SUBCLASS_02)
#define PCI_SUBCLASS_NET_ATM (PCI_SUBCLASS_03)
#define PCI_SUBCLASS_NET_ISDN (PCI_SUBCLASS_04)
#define PCI_SUBCLASS_NET_WFIP (PCI_SUBCLASS_05)
#define PCI_SUBCLASS_NET_PCMIG214 (PCI_SUBCLASS_06)
#define PCI_SUBCLASS_NET_OTHER (PCI_SUBCLASS_80)

/* Display subclasses - Base Class 03h */

#define PCI_SUBCLASS_DISPLAY_VGA (PCI_SUBCLASS_00)
#define PCI_REG_IF_VGA_STD 0x00
#define PCI_REG_IF_VGA_8514 0x01
#define PCI_SUBCLASS_DISPLAY_XGA (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_DISPLAY_3D (PCI_SUBCLASS_02)
#define PCI_SUBCLASS_DISPLAY_OTHER (PCI_SUBCLASS_80)

/* Multimedia subclasses  - Base Class 04h */

#define PCI_SUBCLASS_MMEDIA_VIDEO (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_MMEDIA_AUDIO (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_MMEDIA_PHONY (PCI_SUBCLASS_02)
#define PCI_SUBCLASS_MMEDIA_OTHER (PCI_SUBCLASS_80)

/* Memory subclasses - Base Class 05h */

#define PCI_SUBCLASS_MEM_RAM (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_MEM_FLASH (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_MEM_OTHER (PCI_SUBCLASS_80)

/* Bus Bridge Device subclasses - Base Class 06h */

#define PCI_SUBCLASS_HOST_PCI_BRIDGE (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_ISA_BRIDGE (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_EISA_BRIDGE (PCI_SUBCLASS_02)
#define PCI_SUBCLASS_MCA_BRIDGE (PCI_SUBCLASS_03)
#define PCI_SUBCLASS_P2P_BRIDGE (PCI_SUBCLASS_04)
#define PCI_REG_IF_P2P_STD 0x00
#define PCI_REG_IF_P2P_SUB_DECODE 0x01
#define PCI_SUBCLASS_PCMCIA_BRIDGE (PCI_SUBCLASS_05)
#define PCI_SUBCLASS_NUBUS_BRIDGE (PCI_SUBCLASS_06)
#define PCI_SUBCLASS_CARDBUS_BRIDGE (PCI_SUBCLASS_07)
#define PCI_SUBCLASS_RACEWAY_BRIDGE (PCI_SUBCLASS_08)
#define PCI_REG_IF_RACEWAY_XPARENT 0x00
#define PCI_REG_IF_RACEWAY_END_PNT 0x01
#define PCI_SUBCLASS_SEMI_XPARENT (PCI_SUBCLASS_09)
#define PCI_REG_IF_SEMI_XPARENT_PRI 0x40
#define PCI_REG_IF_SEMI_XPARENT_SEC 0x80
#define PCI_SUBCLASS_INFINI2PCI (PCI_SUBCLASS_0A)
#define PCI_SUBCLASS_OTHER_BRIDGE (PCI_SUBCLASS_80)

/* Simple Communications Controller subclasses - Base Class 07h */

#define PCI_SUBCLASS_SCC_SERIAL (PCI_SUBCLASS_00)
#define PCI_REG_IF_SERIAL_XT 0x00
#define PCI_REG_IF_SERIAL_16450 0x01
#define PCI_REG_IF_SERIAL_16550 0x02
#define PCI_REG_IF_SERIAL_16650 0x03
#define PCI_REG_IF_SERIAL_16750 0x04
#define PCI_REG_IF_SERIAL_16850 0x05
#define PCI_REG_IF_SERIAL_16950 0x06
#define PCI_SUBCLASS_SCC_PARLEL (PCI_SUBCLASS_01)
#define PCI_REG_IF_PARLEL_XT 0x00
#define PCI_REG_IF_PARLEL_BIDIR 0x01
#define PCI_REG_IF_PARLEL_ECP 0x02
#define PCI_REG_IF_PARLEL_1284CTLR 0x03
#define PCI_REG_IF_PARLEL_1284TGT 0xFE
#define PCI_SUBCLASS_SCC_MULTI (PCI_SUBCLASS_02)
#define PCI_SUBCLASS_SCC_MODEM (PCI_SUBCLASS_03)
#define PCI_REG_IF_MODEM_GENERIC 0x00
#define PCI_REG_IF_MODEM_16450 0x01
#define PCI_REG_IF_MODEM_16550 0x02
#define PCI_REG_IF_MODEM_16650 0x03
#define PCI_REG_IF_MODEM_16750 0x04
#define PCI_SUBCLASS_SCC_GPIB (PCI_SUBCLASS_04)
#define PCI_SUBCLASS_SCC_SMRTCRD (PCI_SUBCLASS_05)
#define PCI_SUBCLASS_SCC_OTHER (PCI_SUBCLASS_80)

/* Base System subclasses - Base Class 08h */

#define PCI_SUBCLASS_BASESYS_PIC (PCI_SUBCLASS_00)
#define PCI_REG_IF_PIC_GEN8259 0x00
#define PCI_REG_IF_PIC_ISA 0x01
#define PCI_REG_IF_PIC_EISA 0x02
#define PCI_REG_IF_PIC_APIC 0x10
#define PCI_REG_IF_PIC_xAPIC 0x20
#define PCI_SUBCLASS_BASESYS_DMA (PCI_SUBCLASS_01)
#define PCI_REG_IF_DMA_GEN8237 0x00
#define PCI_REG_IF_DMA_ISA 0x01
#define PCI_REG_IF_DMA_EISA 0x02
#define PCI_SUBCLASS_BASESYS_TIMER (PCI_SUBCLASS_02)
#define PCI_REG_IF_TIMER_GEN8254 0x00
#define PCI_REG_IF_TIMER_ISA 0x01
#define PCI_REG_IF_TIMER_EISA 0x02
#define PCI_SUBCLASS_BASESYS_RTC (PCI_SUBCLASS_03)
#define PCI_REG_IF_RTC_GENERIC 0x00
#define PCI_REG_IF_RTC_ISA 0x01
#define PCI_SUBCLASS_BASESYS_HOTPLUG (PCI_SUBCLASS_04)
#define PCI_SUBCLASS_BASESYS_OTHER (PCI_SUBCLASS_80)

/* Input Device subclasses - Base Class 09h */

#define PCI_SUBCLASS_INPUT_KEYBD (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_INPUT_PEN (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_INPUT_MOUSE (PCI_SUBCLASS_02)
#define PCI_SUBCLASS_INPUT_SCANR (PCI_SUBCLASS_03)
#define PCI_SUBCLASS_INPUT_GAMEPORT (PCI_SUBCLASS_04)
#define PCI_REG_IF_GAMEPORT_GENERIC 0x00
#define PCI_REG_IF_GAMEPORT_LEGACY 0x10
#define PCI_SUBCLASS_INPUT_OTHER (PCI_SUBCLASS_80)

/* Docking Station subclasses - Base Class 0Ah */

#define PCI_SUBCLASS_DOCSTATN_GENERIC (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_DOCSTATN_OTHER (PCI_SUBCLASS_80)

/* Processor subclasses - Base Class 0Bh */

#define PCI_SUBCLASS_PROCESSOR_386 (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_PROCESSOR_486 (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_PROCESSOR_PENTIUM (PCI_SUBCLASS_02)
#define PCI_SUBCLASS_PROCESSOR_ALPHA (PCI_SUBCLASS_10)
#define PCI_SUBCLASS_PROCESSOR_POWERPC (PCI_SUBCLASS_20)
#define PCI_SUBCLASS_PROCESSOR_MIPS (PCI_SUBCLASS_30)
#define PCI_SUBCLASS_PROCESSOR_COPROC (PCI_SUBCLASS_40)

/* Serial bus subclasses - Base Class 0Ch */

#define PCI_SUBCLASS_SERBUS_FIREWIRE (PCI_SUBCLASS_00)
#define PCI_REG_IF_FIREWIRE_1394 0x00
#define PCI_REG_IF_FIREWIRE_HCI1394 0x10
#define PCI_SUBCLASS_SERBUS_ACCESS (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_SERBUS_SSA (PCI_SUBCLASS_02)
#define PCI_SUBCLASS_SERBUS_USB (PCI_SUBCLASS_03)
#define PCI_REG_IF_USB_UHCI 0x00
#define PCI_REG_IF_USB_OHCI 0x10
#define PCI_REG_IF_USB_EHCI 0x20
#define PCI_REG_IF_USB_XHCI 0x30
#define PCI_REG_IF_USB_ANY 0x80
#define PCI_REG_IF_USB_NONHOST 0xFE
#define PCI_SUBCLASS_SERBUS_FIBRE_CHAN (PCI_SUBCLASS_04)
#define PCI_SUBCLASS_SERBUS_SMBUS (PCI_SUBCLASS_05)
#define PCI_SUBCLASS_SERBUS_INFINI (PCI_SUBCLASS_06)
#define PCI_SUBCLASS_SERBUS_IPMI (PCI_SUBCLASS_07)
#define PCI_REG_IF_IPMI_SMIC 0x00
#define PCI_REG_IF_IPMI_KYBD 0x01
#define PCI_REG_IF_IPMI_BLCK 0x02
#define PCI_SUBCLASS_SERBUS_SERCOS (PCI_SUBCLASS_08)
#define PCI_SUBCLASS_SERBUS_CAN (PCI_SUBCLASS_09)
#define PCI_SUBCLASS_SERBUS_OTHER (PCI_SUBCLASS_80)

/* Wireless subclasses - Base Class 0Dh */

#define PCI_SUBCLASS_WIRELESS_IRDA (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_WIRELESS_OTHER_IR (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_WIRELESS_RF (PCI_SUBCLASS_10)
#define PCI_SUBCLASS_WIRELESS_BTOOTH (PCI_SUBCLASS_11)
#define PCI_SUBCLASS_WIRELESS_BBAND (PCI_SUBCLASS_12)
#define PCI_SUBCLASS_WIRELESS_OTHER (PCI_SUBCLASS_80)

/*
 * Intelligent I/O subclasses - Base Class 0Eh
 * REG_IF values greater than 0x00 are reserved for I2O
 */

#define PCI_SUBCLASS_INTELIO (PCI_SUBCLASS_00)
#define PCI_REG_IF_INTELIO_MSG_FIFO 0x00
#define PCI_8UBCLASS_INTELIO_OTHER (PCI_SUBCLASS_00)

/* Satellite Device Communication subclasses - Base Class 0Fh */

#define PCI_SUBCLASS_SATCOM_TV (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_SATCOM_AUDIO (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_SATCOM_VOICE (PCI_SUBCLASS_03)
#define PCI_SUBCLASS_SATCOM_DATA (PCI_SUBCLASS_04)
#define PCI_SUBCLASS_SATCOM_OTHER (PCI_SUBCLASS_80)

/* Encryption/Decryption subclasses - Base Class 10h */

#define PCI_SUBCLASS_EN_DECRYP_NETWORK (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_EN_DECRYP_ENTRTMNT (PCI_SUBCLASS_10)
#define PCI_SUBCLASS_EN_DECRYP_OTHER (PCI_SUBCLASS_80)

/* Data Acquisition and Signal Processing subclasses - Base Class 11h */

#define PCI_SUBCLASS_DAQ_DSP_DPIO (PCI_SUBCLASS_00)
#define PCI_SUBCLASS_DAQ_DSP_PCTRS (PCI_SUBCLASS_01)
#define PCI_SUBCLASS_DAQ_DSP_COMM (PCI_SUBCLASS_10)
#define PCI_SUBCLASS_DAQ_DSP_MGMT (PCI_SUBCLASS_20)
#define PCI_SUBCLASS_DAQ_DSP_OTHER (PCI_SUBCLASS_80)

/* PCI status field bit definitions */

#define PCI_STATUS_RESERVED_0 0x0001
#define PCI_STATUS_RESERVED_1 0x0002
#define PCI_STATUS_RESERVED_2 0x0004
#define PCI_STATUS_INT_STATUS 0x0008
#define PCI_STATUS_CAPABILITY_LIST 0x0010
#define PCI_STATUS_66MHZ 0x0020
#define PCI_STATUS_UDF_SUPPORTED 0x0040
#define PCI_STATUS_FAST_B_TO_B 0x0080
#define PCI_STATUS_DATA_PARITY_ERROR 0x0100
#define PCI_STATUS_DEVSEL_TIMING 0x0600
#define PCI_STATUS_SIG_TGT_ABORT 0x0800
#define PCI_STATUS_RCV_TGT_ABORT 0x1000
#define PCI_STATUS_RCV_MAST_ABORT 0x2000
#define PCI_STATUS_SIG_SYS_ERROR 0x4000
#define PCI_STATUS_DETECT_PARITY_ERROR 0x8000

/* PCI Capability register set identification */

#define PCI_CAP_RESERVED 0x00
#define PCI_CAP_POWER_MGMT 0x01      /* Power Management interface */
#define PCI_CAP_AGP 0x02	     /* AGP Capability	      */
#define PCI_CAP_VPD 0x03	     /* Vital Product Data	      */
#define PCI_CAP_SLOT_ID 0x04	 /* Bridge Slot Identification */
#define PCI_CAP_MSI 0x05	     /* Message Signaled Interrupts*/
#define PCI_CAP_HOT_SWAP 0x06	/* CompactPCI Hot Swap	      */
#define PCI_CAP_PCIX 0x07	    /* PCI-X features	      */
#define PCI_CAP_HYPERTRANSPORT 0x08  /* hypertransport support     */
#define PCI_CAP_VENDOR_SPECIFIC 0x09 /* vendor specific	      */
#define PCI_CAP_DEBUG_PORT 0x0a      /* debug port info	      */
#define PCI_CAP_CPCI_RES_CTRL 0x0b   /* CompactPCI resource control*/
#define PCI_CAP_SHPC 0x0c	    /* hot-plug controller	      */
#define PCI_CAP_P2P_SSID 0x0d	/* subsystem ID capability    */
#define PCI_CAP_AGP_TARGET 0x0e      /* Accelerated Graphics Port  */
#define PCI_CAP_SECURE 0x0f	  /* secure device	      */
#define PCI_CAP_PCI_EXPRESS 0x10     /* PCI Express		      */
#define PCI_CAP_MSIX 0x11	    /* optional extension to MSI  */
#define PCI_CAP_ADVANCED 0x13	/* advanced Features	      */

/* Extended Capability IDs */

#define PCI_EXT_CAP_PCI_PM PCI_CAP_POWER_MGMT
#define PCI_EXT_CAP_AGP PCI_CAP_AGP
#define PCI_EXT_CAP_VPD PCI_CAP_VPD
#define PCI_EXT_CAP_SLOTID PCI_CAP_SLOT_ID
#define PCI_EXT_CAP_MSI PCI_CAP_MSI
#define PCI_EXT_CAP_HOT_SWAP PCI_CAP_HOT_SWAP
#define PCI_EXT_CAP_PCIX PCI_CAP_PCIX
#define PCI_EXT_CAP_DBG_PORT PCI_CAP_DEBUG_PORT
#define PCI_EXT_CAP_CPCI_RES PCI_CAP_CPCI_RES_CTRL
#define PCI_EXT_CAP_HPC PCI_CAP_SHPC
#define PCI_EXT_CAP_EXP PCI_CAP_PCI_EXPRESS
#define PCI_EXT_CAP_MSIX PCI_CAP_MSIX
#define PCI_EXT_CAP_AF PCI_CAP_ADVANCED

/* Power Management Registers */

#define PCI_POWER_MGMT_CAP 2	  /* PM Capabilities Register */
#define PCI_POWER_MGMT_CSR 4	  /* PM CSR Register */
#define PCI_PM_STATE_MASK 0x0003      /* Current power state (D0 to D3) */
#define PCI_PM_NO_SOFT_RESET 0x0008   /* No reset for D3hot->D0 */
#define PCI_PM_PME_ENABLE 0x0100      /* PME pin enable */
#define PCI_PM_DATA_SEL_MASK 0x1e00   /* Data select (??) */
#define PCI_PM_DATA_SCALE_MASK 0x6000 /* Data scale (??) */
#define PCI_PM_PME_STATUS 0x8000      /* PME pin status */

#define PCI_PWR_D0 0
#define PCI_PWR_D1 1
#define PCI_PWR_D2 2
#define PCI_PWR_D3hot 3
#define PCI_PWR_D3cold 4
#define PCI_PWR_BOOTUP 5

#define PCI_MSI_FLAGS_ENABLE 0x1
#define PCI_MSI_FLAGS 2

/* MSI-X registers (these are at offset PCI_MSIX_FLAGS) */

#define PCI_MSIX_FLAGS 2
#define PCI_MSIX_FLAGS_QSIZE 0x7FF
#define PCI_MSIX_FLAGS_ENABLE (1 << 15)
#define PCI_MSIX_FLAGS_MASKALL (1 << 14)
#define PCI_MSIX_FLAGS_BIRMASK (7 << 0)
#define PCI_MSIX_TABLE_OFFSET 0x4

/* Macros to support Intel VT-d code */

#define PCI_BUS(bdf) (((bdf) >> 8) & 0xff)
#define PCI_SLOT(bdf) (((bdf) >> 3) & 0x1f)
#define PCI_FUNC(bdf) ((bdf) & 0x07)
#define PCI_DEVFN(d, f) ((((d) & 0x1f) << 3) | ((f) & 0x07))
#define PCI_DEVFN2(bdf) ((bdf) & 0xff)
#define PCI_BDF(b, d, f) ((((b) & 0xff) << 8) | PCI_DEVFN(d, f))
#define PCI_BDF2(b, df) ((((b) & 0xff) << 8) | ((df) & 0xff))

/* PCI Express capability registers */

#define PCI_EXP_FLAGS 2			 /* Capabilities register */
#define PCI_EXP_FLAGS_VERS 0x000f	/* Capability version */
#define PCI_EXP_FLAGS_TYPE 0x00f0	/* Device/Port type */
#define PCI_EXP_TYPE_ENDPOINT 0x0	/* Express Endpoint */
#define PCI_EXP_TYPE_LEG_END 0x1	 /* Legacy Endpoint */
#define PCI_EXP_TYPE_ROOT_PORT 0x4       /* Root Port */
#define PCI_EXP_TYPE_UPSTREAM 0x5	/* Upstream Port */
#define PCI_EXP_TYPE_DOWNSTREAM 0x6      /* Downstream Port */
#define PCI_EXP_TYPE_PCI_BRIDGE 0x7      /* PCI/PCI-X Bridge */
#define PCI_EXP_TYPE_RC_END 0x9		 /* Root Complex Integrated endpt.*/
#define PCI_EXP_FLAGS_SLOT 0x0100	/* Slot implemented */
#define PCI_EXP_FLAGS_IRQ 0x3e00	 /* Interrupt message number */
#define PCI_EXP_DEVCAP 4		 /* Device capabilities */
#define PCI_EXP_DEVCAP_PAYLOAD 0x07      /* Max_Payload_Size */
#define PCI_EXP_DEVCAP_PHANTOM 0x18      /* Phantom functions */
#define PCI_EXP_DEVCAP_EXT_TAG 0x20      /* Extended tags */
#define PCI_EXP_DEVCAP_L0S 0x1c0	 /* L0s Acceptable Latency */
#define PCI_EXP_DEVCAP_L1 0xe00		 /* L1 Acceptable Latency */
#define PCI_EXP_DEVCAP_ATN_BUT 0x1000    /* Attention Button Present */
#define PCI_EXP_DEVCAP_ATN_IND 0x2000    /* Attention Indicator Present */
#define PCI_EXP_DEVCAP_PWR_IND 0x4000    /* Power Indicator Present */
#define PCI_EXP_DEVCAP_PWR_VAL 0x3fc0000 /* Slot Power Limit Value */
#define PCI_EXP_DEVCAP_PWR_SCL 0xc000000 /* Slot Power Limit Scale */
#define PCI_EXP_DEVCAP_FLR 0x10000000    /* Function Level Reset */
#define PCI_EXP_DEVCTL 8		 /* Device Control */
#define PCI_EXP_DEVCTL_CERE 0x0001       /* Correctable Error Reporting En*/
#define PCI_EXP_DEVCTL_NFERE 0x0002      /* Non-Fatal Error Reporting En */
#define PCI_EXP_DEVCTL_FERE 0x0004       /* Fatal Error Reporting Enable */
#define PCI_EXP_DEVCTL_URRE 0x0008       /* Unsupported Request Reporting */
#define PCI_EXP_DEVCTL_RELAX_EN 0x0010   /* Enable relaxed ordering */
#define PCI_EXP_DEVCTL_PAYLOAD 0x00e0    /* Max_Payload_Size */
#define PCI_EXP_DEVCTL_EXT_TAG 0x0100    /* Extended Tag Field Enable */
#define PCI_EXP_DEVCTL_PHANTOM 0x0200    /* Phantom Functions Enable */
#define PCI_EXP_DEVCTL_AUX_PME 0x0400    /* Auxiliary Power PM Enable */
#define PCI_EXP_DEVCTL_NOSNOOP_EN 0x0800 /* Enable No Snoop */
#define PCI_EXP_DEVCTL_READRQ 0x7000     /* Max_Read_Request_Size */
#define PCI_EXP_DEVCTL_BCR_FLR 0x8000    /* BCR / FLR */
#define PCI_EXP_DEVSTA 10		 /* Device Status */
#define PCI_EXP_DEVSTA_CED 0x01		 /* Correctable Error Detected */
#define PCI_EXP_DEVSTA_NFED 0x02	 /* Non-Fatal Error Detected */
#define PCI_EXP_DEVSTA_FED 0x04		 /* Fatal Error Detected */
#define PCI_EXP_DEVSTA_URD 0x08		 /* Unsupported Request Detected */
#define PCI_EXP_DEVSTA_AUXPD 0x10	/* AUX Power Detected */
#define PCI_EXP_DEVSTA_TRPND 0x20	/* Transactions Pending */
#define PCI_EXP_LNKCAP 12		 /* Link Capabilities */
#define PCI_EXP_LNKCTL 16		 /* Link Control */
#define PCI_EXP_LNKCTL_CLKREQ_EN 0x100   /* Enable clkreq */
#define PCI_EXP_LNKSTA 18		 /* Link Status */
#define PCI_EXP_SLTCAP 20		 /* Slot Capabilities */
#define PCI_EXP_SLTCTL 24		 /* Slot Control */
#define PCI_EXP_SLTSTA 26		 /* Slot Status */
#define PCI_EXP_RTCTL 28		 /* Root Control */
#define PCI_EXP_RTCTL_SECEE 0x01	 /* System Error on Corr. Error */
#define PCI_EXP_RTCTL_SENFEE 0x02	/* System Error on Non-Fatal Err */
#define PCI_EXP_RTCTL_SEFEE 0x04	 /* System Error on Fatal Error */
#define PCI_EXP_RTCTL_PMEIE 0x08	 /* PME Interrupt Enable */
#define PCI_EXP_RTCTL_CRSSVE 0x10	/* CRS Software Visibility En */
#define PCI_EXP_RTCAP 30		 /* Root Capabilities */
#define PCI_EXP_RTSTA 32		 /* Root Status */

/* PCI-X registers */

#define PCI_X_CMD 2

#ifdef __cplusplus

/* Remapped reserved C++ words */
#undef class
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCI_PCI_MGR_H_ */
