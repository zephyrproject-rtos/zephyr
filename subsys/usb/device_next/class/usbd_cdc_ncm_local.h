/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024, Hardy Griech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */


#ifndef __USBD_CDC_NCM_LOCAL_H_
#define __USBD_CDC_NCM_LOCAL_H_


#ifndef CONFIG_CDC_NCM_ALIGNMENT
    #define CONFIG_CDC_NCM_ALIGNMENT              4
#endif
#if (CONFIG_CDC_NCM_ALIGNMENT != 4)
    #error "CONFIG_CDC_NCM_ALIGNMENT must be 4, otherwise the headers and start of datagrams have to be aligned (which they are currently not)"
#endif

#define CONFIG_CDC_NCM_XMT_MAX_DATAGRAMS_PER_NTB  1
#define CONFIG_CDC_NCM_XMT_NTB_MAX_SIZE           2048    // see discussion in https://github.com/hathach/tinyusb/pull/2227
#define CONFIG_CDC_NCM_RCV_NTB_MAX_SIZE           2048

#if (CONFIG_CDC_NCM_XMT_NTB_MAX_SIZE != CONFIG_CDC_NCM_RCV_NTB_MAX_SIZE)
    #error "CONFIG_CDC_NCM_XMT_NTB_MAX_SIZE != CONFIG_CDC_NCM_RCV_NTB_MAX_SIZE"
#endif

// Table 6.2 Class-Specific Request Codes for Network Control Model subclass
typedef enum
{
    NCM_SET_ETHERNET_MULTICAST_FILTERS               = 0x40,
    NCM_SET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER = 0x41,
    NCM_GET_ETHERNET_POWER_MANAGEMENT_PATTERN_FILTER = 0x42,
    NCM_SET_ETHERNET_PACKET_FILTER                   = 0x43,
    NCM_GET_ETHERNET_STATISTIC                       = 0x44,
    NCM_GET_NTB_PARAMETERS                           = 0x80,    // required
    NCM_GET_NET_ADDRESS                              = 0x81,
    NCM_SET_NET_ADDRESS                              = 0x82,
    NCM_GET_NTB_FORMAT                               = 0x83,
    NCM_SET_NTB_FORMAT                               = 0x84,
    NCM_GET_NTB_INPUT_SIZE                           = 0x85,    // required according to spec
    NCM_SET_NTB_INPUT_SIZE                           = 0x86,    // required according to spec
    NCM_GET_MAX_DATAGRAM_SIZE                        = 0x87,
    NCM_SET_MAX_DATAGRAM_SIZE                        = 0x88,
    NCM_GET_CRC_MODE                                 = 0x89,
    NCM_SET_CRC_MODE                                 = 0x8A,
} ncm_request_code_t;


// Table 6.6 Class-Specific Notification Codes for Networking Control Model subclass
typedef enum
{
    NCM_NOTIFICATION_NETWORK_CONNECTION              = 0x00,
    NCM_NOTIFICATION_RESPONSE_AVAILABLE              = 0x01,
    NCM_NOTIFICATION_CONNECTION_SPEED_CHANGE         = 0x2a,
} ncm_notification_code_t;


#define NTH16_SIGNATURE      0x484D434E
#define NDP16_SIGNATURE_NCM0 0x304D434E
#define NDP16_SIGNATURE_NCM1 0x314D434E

// network endianess = LE!
typedef struct __packed {
    uint16_t wLength;
    uint16_t bmNtbFormatsSupported;
    uint32_t dwNtbInMaxSize;
    uint16_t wNdbInDivisor;
    uint16_t wNdbInPayloadRemainder;
    uint16_t wNdbInAlignment;
    uint16_t wReserved;
    uint32_t dwNtbOutMaxSize;
    uint16_t wNdbOutDivisor;
    uint16_t wNdbOutPayloadRemainder;
    uint16_t wNdbOutAlignment;
    uint16_t wNtbOutMaxDatagrams;
} ntb_parameters_t;

// network endianess = LE!
typedef struct __packed {
    uint32_t dwSignature;
    uint16_t wHeaderLength;
    uint16_t wSequence;
    uint16_t wBlockLength;
    uint16_t wNdpIndex;
} nth16_t;

// network endianess = LE!
typedef struct __packed {
    uint16_t wDatagramIndex;
    uint16_t wDatagramLength;
} ndp16_datagram_t;

// network endianess = LE!
typedef struct __packed {
    uint32_t dwSignature;
    uint16_t wLength;
    uint16_t wNextNdpIndex;
    //ndp16_datagram_t datagram[];
} ndp16_t;

typedef union __packed {
    struct {
        nth16_t          nth;
        ndp16_t          ndp;
        ndp16_datagram_t ndp_datagram[CONFIG_CDC_NCM_XMT_MAX_DATAGRAMS_PER_NTB + 1];
    };
    uint8_t data[CONFIG_CDC_NCM_XMT_NTB_MAX_SIZE];
} xmit_ntb_t;

typedef union __packed {
    struct {
        nth16_t nth;
        // only the header is at a guaranteed position
    };
    uint8_t data[CONFIG_CDC_NCM_RCV_NTB_MAX_SIZE];
} recv_ntb_t;

// network endianess = LE!
typedef struct __packed {
    struct usb_setup_packet header;
    uint32_t                downlink, uplink;
} ncm_notify_connection_speed_change_t;

// network endianess = LE!
typedef struct __packed {
    struct usb_setup_packet header;
} ncm_notify_network_connection_t;


#endif
