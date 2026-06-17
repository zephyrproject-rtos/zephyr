/*
 * Copyright 2020 BitBank Software, Inc. All Rights Reserved.
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Software JPEG decoder API.
 *
 * Exposes the C API and state structures for the software JPEG decoder
 * integrated into the MP zvid plugin.
 *
 * Supported input mode:
 * - Memory (RAM) input via JPEG_openRAM()
 * - Decode to a caller-provided framebuffer via JPEG_setFramebuffer()
 */

#ifndef ZEPHYR_INCLUDE_MP_ZVID_JPEG_DEC_H_
#define ZEPHYR_INCLUDE_MP_ZVID_JPEG_DEC_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @name Internal sizing constants
 * @{
 */
#define FILE_HIGHWATER      1536
#define JPEG_FILE_BUF_SIZE  2048
#define HUFF_TABLEN         273
#define HUFF11SIZE          (1 << 11)
#define DC_TABLE_SIZE       1024
#define DCTSIZE             64
#define MAX_MCU_COUNT       6
#define MAX_COMPS_IN_SCAN   4
#define MAX_BUFFERED_PIXELS 2048
#define MCU_SKIP            (-8)
/** @} */

/** @name Decoder options bitmask for JPEG_decode() iOptions parameter
 * @{
 */
#define JPEG_AUTO_ROTATE    1
#define JPEG_SCALE_HALF     2
#define JPEG_SCALE_QUARTER  4
#define JPEG_SCALE_EIGHTH   8
#define JPEG_LE_PIXELS      16
#define JPEG_EXIF_THUMBNAIL 32
#define JPEG_LUMA_ONLY      64
#define JPEG_USES_DMA       128
/** @} */

#define MCU0 (DCTSIZE * 0)
#define MCU1 (DCTSIZE * 1)
#define MCU2 (DCTSIZE * 2)
#define MCU3 (DCTSIZE * 3)
#define MCU4 (DCTSIZE * 4)
#define MCU5 (DCTSIZE * 5)

#if UINTPTR_MAX > 0xffffffffU
#define REGISTER_WIDTH 64
typedef uint64_t my_ulong;
typedef int64_t my_long;
#else
#define REGISTER_WIDTH 32
typedef uint32_t my_ulong;
typedef int32_t my_long;
#endif

/** Supported JPEG decode modes. */
enum {
	JPEG_MODE_BASELINE = 0,
	JPEG_MODE_PROGRESSIVE,
	JPEG_MODE_INVALID,
};

/** Output pixel type selector for JPEG_setPixelType(). */
enum {
	RGB565_LITTLE_ENDIAN = 0,
	RGB565_BIG_ENDIAN,
	RGB8888,
	EIGHT_BIT_GRAYSCALE,
	FOUR_BIT_DITHERED,
	TWO_BIT_DITHERED,
	ONE_BIT_DITHERED,
	INVALID_PIXEL_TYPE,
};

/** Memory type selector. */
enum {
	JPEG_MEM_RAM = 0,
};

/** Error codes returned by JPEG_getLastError(). */
enum {
	JPEG_SUCCESS = 0,
	JPEG_INVALID_PARAMETER,
	JPEG_DECODE_ERROR,
	JPEG_UNSUPPORTED_FEATURE,
	JPEG_INVALID_FILE,
	JPEG_ERROR_MEMORY,
};

/**
 * @brief Buffered bitstream state.
 */
typedef struct buffered_bits {
	/** Current byte pointer into the VLC buffer. */
	unsigned char *pBuf;
	/** Bit accumulator. */
	my_ulong ulBits;
	/** Current bit offset into @ref ulBits. */
	uint32_t ulBitOff;
} BUFFERED_BITS;

/**
 * @brief Input source descriptor.
 */
typedef struct jpeg_file_tag {
	/** Current input position (bytes). */
	int32_t iPos;
	/** Total input size (bytes). */
	int32_t iSize;
	/** Pointer to input data in RAM. */
	uint8_t *pData;
	/** Optional handle for non-RAM backends (unused in Zephyr). */
	void *fHandle;
} JPEGFILE;

/**
 * @brief Draw callback argument.
 */
typedef struct jpeg_draw_tag {
	/** Upper-left X corner of the current MCU (pixels). */
	int x;
	/** Upper-left Y corner of the current MCU (pixels). */
	int y;
	/** Width of the pixel block in @ref pPixels (pixels). */
	int iWidth;
	/** Height of the pixel block in @ref pPixels (pixels). */
	int iHeight;
	/** Clipped width used for odd edges (pixels). */
	int iWidthUsed;
	/** Bits-per-pixel of the pixels pointed to by @ref pPixels. */
	int iBpp;
	/** Pointer to pixel buffer (format depends on pixel type). */
	uint16_t *pPixels;
	/** User pointer forwarded from @ref JPEGIMAGE.pUser. */
	void *pUser;
} JPEGDRAW;

/** Read callback prototype. */
typedef int32_t JPEG_READ_CALLBACK(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen);
/** Seek callback prototype. */
typedef int32_t JPEG_SEEK_CALLBACK(JPEGFILE *pFile, int32_t iPosition);
/** Draw callback prototype. */
typedef int JPEG_DRAW_CALLBACK(JPEGDRAW *pDraw);
/** Open callback prototype. */
typedef void *JPEG_OPEN_CALLBACK(const char *szFilename, int32_t *pFileSize);
/** Close callback prototype. */
typedef void JPEG_CLOSE_CALLBACK(void *pHandle);

/**
 * @brief JPEG color component metadata.
 */
typedef struct _jpegcompinfo {
	unsigned char component_needed;
	unsigned char component_id;
	unsigned char component_index;
	unsigned char quant_tbl_no;
	unsigned char dc_tbl_no;
	unsigned char ac_tbl_no;
} JPEGCOMPINFO;

/**
 * @brief JPEG decoder state.
 *
 * Callers allocate this structure and pass it to the JPEG API functions.
 */
typedef struct jpeg_image_tag {
	/** Image width in pixels. */
	int iWidth;
	/** Image height in pixels. */
	int iHeight;
	/** Thumbnail width in pixels (if present). */
	int iThumbWidth;
	/** Thumbnail height in pixels (if present). */
	int iThumbHeight;
	/** Offset to thumbnail data. */
	int iThumbData;
	/** X offset for decode output placement (pixels). */
	int iXOffset;
	/** Y offset for decode output placement (pixels). */
	int iYOffset;
	/** Crop origin X (pixels). */
	int iCropX;
	/** Crop origin Y (pixels). */
	int iCropY;
	/** Crop width (pixels). */
	int iCropCX;
	/** Crop height (pixels). */
	int iCropCY;
	/** Bits-per-pixel of decoded output. */
	uint8_t ucBpp;
	/** JPEG subsampling mode. */
	uint8_t ucSubSample;
	/** Bitmask of Huffman tables used/defined. */
	uint8_t ucHuffTableUsed;
	uint8_t ucMode;
	/** EXIF orientation. */
	uint8_t ucOrientation;
	/** Whether a thumbnail is present. */
	uint8_t ucHasThumb;
	uint8_t b11Bit;
	uint8_t ucComponentsInScan;
	uint8_t cApproxBitsLow;
	uint8_t cApproxBitsHigh;
	uint8_t iScanStart;
	uint8_t iScanEnd;
	/** Marker filtering state (0xFF stuffing). */
	uint8_t ucFF;
	/** Number of components in the image. */
	uint8_t ucNumComponents;
	uint8_t ucACTable;
	uint8_t ucDCTable;
	/** Input memory type. */
	uint8_t ucMemType;
	/** Output pixel type. */
	uint8_t ucPixelType;
	uint16_t u16MCUFlags;
	/** Offset to EXIF TIFF. */
	int iEXIF;
	/** Last error code. */
	int iError;
	/** Decoder options bitmask. */
	int iOptions;
	/** Current VLC data offset. */
	int iVLCOff;
	/** Current VLC data size. */
	int iVLCSize;
	/** Restart interval. */
	int iResInterval;
	/** Restart counter. */
	int iResCount;
	/** Max MCUs of pixels per draw callback invocation. */
	int iMaxMCUs;

	JPEG_READ_CALLBACK *pfnRead;
	JPEG_SEEK_CALLBACK *pfnSeek;
	JPEG_DRAW_CALLBACK *pfnDraw;
	JPEG_OPEN_CALLBACK *pfnOpen;
	JPEG_CLOSE_CALLBACK *pfnClose;

	JPEGCOMPINFO JPCI[MAX_COMPS_IN_SCAN];
	JPEGFILE JPEGFile;
	BUFFERED_BITS bb;

	/** User pointer available to callbacks. */
	void *pUser;
	/** Optional dithering buffer. */
	uint8_t *pDitherBuffer;
	/** Pixel buffer pointer (aligned). */
	uint16_t *usPixels;
	/** Unaligned storage backing @ref usPixels. */
	uint16_t usUnalignedPixels[MAX_BUFFERED_PIXELS + 8];
	/** MCU buffer pointer (aligned). */
	int16_t *sMCUs;
	/** Unaligned storage backing @ref sMCUs. */
	int16_t sUnalignedMCUs[8 + (DCTSIZE * MAX_MCU_COUNT)];
	/** Output framebuffer base pointer. */
	void *pFramebuffer;
	/** Quantization tables. */
	int16_t sQuantTable[DCTSIZE * 4];
	/** Input/VLC buffer. */
	uint8_t ucFileBuf[JPEG_FILE_BUF_SIZE];
	/** Huffman DC decode tables. */
	uint8_t ucHuffDC[DC_TABLE_SIZE * 2];
	/** Huffman AC decode tables. */
	uint16_t usHuffAC[HUFF11SIZE * 2];
} JPEGIMAGE;

/** @name Endianness/unaligned helpers
 *
 * Use Zephyr sys_get_*() for safe unaligned access.
 * @{
 */
#define INTELSHORT(p) sys_get_le16((const uint8_t *)(p))
#define INTELLONG(p)  sys_get_le32((const uint8_t *)(p))
#define MOTOSHORT(p)  sys_get_be16((const uint8_t *)(p))
#if REGISTER_WIDTH == 64
#define MOTOLONG(p) sys_get_be64((const uint8_t *)(p))
#else
#define MOTOLONG(p) sys_get_be32((const uint8_t *)(p))
#endif
/** @} */

/**
 * @brief Initialize the decoder to read JPEG data from RAM.
 *
 * @param pJPEG   Pointer to caller-allocated decoder state.
 * @param pData   Pointer to JPEG data in RAM.
 * @param iDataSize Size of the JPEG data in bytes.
 * @param pfnDraw Draw callback invoked for each decoded MCU row.
 *
 * @retval 1 on success.
 * @retval 0 on failure.
 */
int JPEG_openRAM(JPEGIMAGE *pJPEG, uint8_t *pData, int iDataSize, JPEG_DRAW_CALLBACK *pfnDraw);

/**
 * @brief Set the output framebuffer pointer.
 *
 * @param pJPEG        Pointer to decoder state.
 * @param pFramebuffer Pointer to the output framebuffer.
 */
void JPEG_setFramebuffer(JPEGIMAGE *pJPEG, void *pFramebuffer);

/**
 * @brief Set the crop area in pixels.
 *
 * @param pJPEG Pointer to decoder state.
 * @param x     Crop origin X.
 * @param y     Crop origin Y.
 * @param w     Crop width.
 * @param h     Crop height.
 */
void JPEG_setCropArea(JPEGIMAGE *pJPEG, int x, int y, int w, int h);

/**
 * @brief Get the current crop area in pixels.
 *
 * @param pJPEG Pointer to decoder state.
 * @param[out] x Crop origin X.
 * @param[out] y Crop origin Y.
 * @param[out] w Crop width.
 * @param[out] h Crop height.
 */
void JPEG_getCropArea(JPEGIMAGE *pJPEG, int *x, int *y, int *w, int *h);

/**
 * @brief Decode the JPEG into the configured framebuffer.
 *
 * @param pJPEG    Pointer to decoder state.
 * @param x        X offset for output placement.
 * @param y        Y offset for output placement.
 * @param iOptions Bitmask of JPEG_* decode options.
 *
 * @retval 1 on success.
 * @retval 0 on failure.
 */
int JPEG_decode(JPEGIMAGE *pJPEG, int x, int y, int iOptions);

/**
 * @brief Decode with dithering enabled.
 *
 * @param pJPEG    Pointer to decoder state.
 * @param pDither  Pointer to dithering buffer.
 * @param iOptions Bitmask of JPEG_* decode options.
 *
 * @retval 1 on success.
 * @retval 0 on failure.
 */
int JPEG_decodeDither(JPEGIMAGE *pJPEG, uint8_t *pDither, int iOptions);

/**
 * @brief Close the decoder and release resources.
 *
 * @param pJPEG Pointer to decoder state.
 */
void JPEG_close(JPEGIMAGE *pJPEG);

/**
 * @brief Get decoded image width.
 *
 * @param pJPEG Pointer to decoder state.
 *
 * @return Image width in pixels.
 */
int JPEG_getWidth(JPEGIMAGE *pJPEG);

/**
 * @brief Get decoded image height.
 *
 * @param pJPEG Pointer to decoder state.
 *
 * @return Image height in pixels.
 */
int JPEG_getHeight(JPEGIMAGE *pJPEG);

/**
 * @brief Get bits-per-pixel of decoded output.
 *
 * @param pJPEG Pointer to decoder state.
 *
 * @return Bits per pixel.
 */
int JPEG_getBpp(JPEGIMAGE *pJPEG);

/**
 * @brief Get JPEG subsampling mode.
 *
 * @param pJPEG Pointer to decoder state.
 *
 * @return Subsampling mode value.
 */
int JPEG_getSubSample(JPEGIMAGE *pJPEG);

/**
 * @brief Get EXIF orientation.
 *
 * @param pJPEG Pointer to decoder state.
 *
 * @return Orientation value (1-8).
 */
int JPEG_getOrientation(JPEGIMAGE *pJPEG);

/**
 * @brief Get last error code.
 *
 * @param pJPEG Pointer to decoder state.
 *
 * @return Error code (JPEG_SUCCESS on no error).
 */
int JPEG_getLastError(JPEGIMAGE *pJPEG);

/**
 * @brief Check whether the JPEG contains an embedded thumbnail.
 *
 * @param pJPEG Pointer to decoder state.
 *
 * @retval 1 if thumbnail is present.
 * @retval 0 if no thumbnail.
 */
int JPEG_hasThumb(JPEGIMAGE *pJPEG);

/**
 * @brief Get embedded thumbnail width.
 *
 * @param pJPEG Pointer to decoder state.
 *
 * @return Thumbnail width in pixels.
 */
int JPEG_getThumbWidth(JPEGIMAGE *pJPEG);

/**
 * @brief Get embedded thumbnail height.
 *
 * @param pJPEG Pointer to decoder state.
 *
 * @return Thumbnail height in pixels.
 */
int JPEG_getThumbHeight(JPEGIMAGE *pJPEG);

/**
 * @brief Select the output pixel type.
 *
 * @param pJPEG Pointer to decoder state.
 * @param iType Pixel type (one of the pixel type enum values).
 */
void JPEG_setPixelType(JPEGIMAGE *pJPEG, int iType);

/**
 * @brief Limit the number of MCUs output per draw call.
 *
 * @param pJPEG   Pointer to decoder state.
 * @param iMaxMCUs Maximum MCU count per callback invocation.
 */
void JPEG_setMaxOutputSize(JPEGIMAGE *pJPEG, int iMaxMCUs);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MP_ZVID_JPEG_DEC_H_ */
