/*
 * Copyright (c) 2024 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_USB_VIDEO_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_USB_VIDEO_H

/* UVC GUIDs as defined on Linux source */
#define UVC_GUID_MJPEG             4d 4a 50 47 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_YUY2              59 55 59 32 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_YUY2_ISIGHT       59 55 59 32 00 00 10 00 80 00 00 00 00 38 9b 71
#define UVC_GUID_NV12              4e 56 31 32 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_YV12              59 56 31 32 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_I420              49 34 32 30 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_UYVY              55 59 56 59 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_Y800              59 38 30 30 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_Y8                59 38 20 20 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_Y10               59 31 30 20 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_Y12               59 31 32 20 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_Y16               59 31 36 20 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_BY8               42 59 38 20 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_BA81              42 41 38 31 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_GBRG              47 42 52 47 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_GRBG              47 52 42 47 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_RGGB              52 47 47 42 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_BG16              42 47 31 36 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_GB16              47 42 31 36 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_RG16              52 47 31 36 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_GR16              47 52 31 36 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_RGBP              52 47 42 50 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_BGR3              7d eb 36 e4 4f 52 ce 11 9f 53 00 20 af 0b a7 70
#define UVC_GUID_BGR4              7e eb 36 e4 4f 52 ce 11 9f 53 00 20 af 0b a7 70
#define UVC_GUID_M420              4d 34 32 30 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_H264              48 32 36 34 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_H265              48 32 36 35 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_Y8I               59 38 49 20 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_Y12I              59 31 32 49 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_Z16               5a 31 36 20 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_RW10              52 57 31 30 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_INVZ              49 4e 56 5a 90 2d 58 4a 92 0b 77 3f 1f 2c 55 6b
#define UVC_GUID_INZI              49 4e 5a 49 66 1a 42 a2 90 65 d0 18 14 a8 ef 8a
#define UVC_GUID_INVI              49 4e 56 49 db 57 49 5e 8e 3f f4 79 53 2b 94 6f
#define UVC_GUID_CNF4              43 20 20 20 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_D3DFMT_L8         32 00 00 00 00 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_KSMEDIA_L8_IR     32 00 00 00 02 00 10 00 80 00 00 aa 00 38 9b 71
#define UVC_GUID_HEVC              48 45 56 43 00 00 10 00 80 00 00 aa 00 38 9b 71

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_USB_VIDEO_H */
