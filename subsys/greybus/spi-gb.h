/*
 * Copyright (c) 2015 Google, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GREYBUS_SPI_H_
#define _GREYBUS_SPI_H_

#include <greybus/types.h>

/* SPI Protocol Operation Types */
#define GB_SPI_PROTOCOL_VERSION     0x01    /* Protocol Version */
#define GB_SPI_TYPE_MASTER_CONFIG   0x02    /* Get config for SPI master */
#define GB_SPI_TYPE_DEVICE_CONFIG   0x03    /* Get config for SPI device */
#define GB_SPI_PROTOCOL_TRANSFER    0x04    /* Transfer */

/* SPI Protocol Mode Bit Masks */
#define GB_SPI_MODE_CPHA        0x01    /* clock phase */
#define GB_SPI_MODE_CPOL        0x02    /* clock polarity */
#define GB_SPI_MODE_CS_HIGH     0x04    /* chipselect active high */
#define GB_SPI_MODE_LSB_FIRST   0x08    /* per-word bits-on-wire */
#define GB_SPI_MODE_3WIRE       0x10    /* SI/SO signals shared */
#define GB_SPI_MODE_LOOP        0x20    /* loopback mode */
#define GB_SPI_MODE_NO_CS       0x40    /* one dev/bus, no chipselect */
#define GB_SPI_MODE_READY       0x80    /* slave pulls low to pause */

/* SPI Protocol Flags */
#define GB_SPI_FLAG_HALF_DUPLEX 0x01    /* can't do full duplex */
#define GB_SPI_FLAG_NO_RX       0x02    /* can't do buffer read */
#define GB_SPI_FLAG_NO_TX       0x04    /* can't do buffer write */

/* SPI Transfer Type */
#define GB_SPI_XFER_READ        0x01    /* flag for read transfer */
#define GB_SPI_XFER_WRITE       0x02    /* flag for write transfer */

/**
 * SPI Protocol Version Response
 */
struct gb_spi_proto_version_response {
    __u8    major; /**< SPI Protocol major version */
    __u8    minor; /**< SPI Protocol minor version */
} __packed;

/**
 * SPI Protocol master configure response
 */
struct gb_spi_master_config_response {
    /** bit per word */
    __le32  bpw_mask;
    /** minimum Transfer speed in Hz */
    __le32  min_speed_hz;
    /** maximum Transfer speed in Hz */
    __le32  max_speed_hz;
    /** mode bit */
    __le16  mode;
    /** flag bit */
    __le16  flags;
    /** supported chip number */
    __u8  num_chipselect;
} __packed;

/**
 * SPI Protocol device configure request
 */
struct gb_spi_device_config_request{
    /** required chip number */
    __u8    chip_select;
} __packed;

/**
 * SPI Protocol device configure response
 */
struct gb_spi_device_config_response {
    /** mode be set in device */
    __le16  mode;
    /** bit per word be set in device */
    __u8    bpw;
    /** max speed be set in device */
    __le32  max_speed_hz;
    /** SPI device type */
	__u8	device_type;
    /** chip name */
    __u8    name[32];
} __packed;

/**
 * SPI transfer descriptor
 */
struct gb_spi_transfer_desc {
    /** speed for setting this an op */
    __le32  speed_hz;
    /** length to read and/or write */
    __le32  len;
    /** Wait period after completion of transfer */
    __le16  delay_usecs;
    /** Toggle chip select pin after this transfer completes */
    __u8    cs_change;
    /** Select bits per word for this transfer */
    __u8    bits_per_word;
    /** Bit Mask indicating Read (0x01) and/or Write (0x02) transfer type */
    __u8    rdwr;
} __packed;

/**
 * SPI Protocol Transfer Request
 */
struct gb_spi_transfer_request {
    /** chip-select pin for the slave device */
    __u8    chip_select;
    /** Greybus SPI Protocol Mode Bit Masks */
    __u8    mode;
    /** Number of gb_spi_transfer_desc */
    __le16  count;
    /** SPI gb_spi_transfer_desc array in the transfer */
    struct gb_spi_transfer_desc  transfers[0];
} __packed;

/**
 * SPI Protocol Transfer Response
 */
struct gb_spi_transfer_response {
    /** Data array for read gb_spi_transfer descriptor on the transfer */
    __u8    data[0];
} __packed;

#endif /* _GREYBUS_SPI_H_ */
