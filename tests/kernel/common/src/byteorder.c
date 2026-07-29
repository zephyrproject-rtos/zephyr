/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/ztest.h>

#include <zephyr/sys/byteorder.h>

/**
 * @defgroup kernel_byteorder_tests Byteorder Operations
 * @ingroup all_tests
 * @{
 * @}
 *
 * @addtogroup kernel_byteorder_tests
 * @{
 */

/**
 * @brief Verify sys_memcpy_swap() copies a buffer in reversed byte order.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_memcpy_swap() produces a destination buffer that is the
 * source buffer reversed byte-for-byte, leaving the source untouched, and that
 * the operation is its own inverse (swapping the swapped result restores the
 * original). This guarantees correct byte reordering for non-overlapping
 * buffers regardless of buffer length.
 *
 * Test steps:
 * - Swap-copy an 8-byte source into a destination and compare with the
 *   expected reversed buffer.
 * - Swap-copy the reversed buffer back and confirm the original is restored.
 *
 * Expected result:
 * - Both destination buffers match the expected reversed contents.
 *
 * @see sys_memcpy_swap()
 */
ZTEST(byteorder, test_byteorder_memcpy_swap)
{
	uint8_t buf_orig[8] = { 0x00, 0x01, 0x02, 0x03,
			     0x04, 0x05, 0x06, 0x07 };
	uint8_t buf_chk[8] = { 0x07, 0x06, 0x05, 0x04,
			    0x03, 0x02, 0x01, 0x00 };
	uint8_t buf_dst[8] = { 0 };

	sys_memcpy_swap(buf_dst, buf_orig, 8);
	zassert_true((memcmp(buf_dst, buf_chk, 8) == 0),
		     "Swap memcpy failed");

	sys_memcpy_swap(buf_dst, buf_chk, 8);
	zassert_true((memcmp(buf_dst, buf_orig, 8) == 0),
		     "Swap memcpy failed");
}

/**
 * @brief Verify sys_mem_swap() reverses a buffer in place.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_mem_swap() reverses the byte order of a buffer in place,
 * for both even- and odd-length buffers, proving the in-place swap correctly
 * handles the middle element of odd-sized regions.
 *
 * Test steps:
 * - Swap an 8-byte buffer in place and compare with the expected reversal.
 * - Swap an 11-byte buffer in place and compare with the expected reversal.
 *
 * Expected result:
 * - Both buffers contain their byte-reversed contents.
 *
 * @see sys_mem_swap()
 */
ZTEST(byteorder, test_byteorder_mem_swap)
{
	uint8_t buf_orig_1[8] = { 0x00, 0x01, 0x02, 0x03,
			       0x04, 0x05, 0x06, 0x07 };
	uint8_t buf_orig_2[11] = { 0x00, 0x01, 0x02, 0x03,
				0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0xa0 };
	uint8_t buf_chk_1[8] = { 0x07, 0x06, 0x05, 0x04,
			      0x03, 0x02, 0x01, 0x00 };
	uint8_t buf_chk_2[11] = { 0xa0, 0x09, 0x08, 0x07,
			       0x06, 0x05, 0x04, 0x03,
			       0x02, 0x01, 0x00 };

	sys_mem_swap(buf_orig_1, 8);
	zassert_true((memcmp(buf_orig_1, buf_chk_1, 8) == 0),
		     "Swapping buffer failed");

	sys_mem_swap(buf_orig_2, 11);
	zassert_true((memcmp(buf_orig_2, buf_chk_2, 11) == 0),
		     "Swapping buffer failed");
}

/**
 * @brief Verify BSWAP_16() reverses the byte order of a 16-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that the BSWAP_16() internal helper macro, used by
 * sys_be16_to_cpu()/sys_cpu_to_be16()/sys_le16_to_cpu()/sys_cpu_to_le16(),
 * reverses the byte order of a 16-bit value.
 *
 * Test steps:
 * - Byte-swap a known 16-bit value with BSWAP_16().
 *
 * Expected result:
 * - The result equals the expected byte-reversed value.
 *
 * @see BSWAP_16()
 */
ZTEST(byteorder, test_bswap_16)
{
	uint16_t val = 0xf0e1;
	uint16_t expected = 0xe1f0;
	uint16_t result = BSWAP_16(val);

	zassert_equal(result, expected, "BSWAP_16() failed");
}

/**
 * @brief Verify BSWAP_24() reverses the byte order of a 24-bit value and
 *        ignores garbage in its unused MSB padding byte.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that the BSWAP_24() internal helper macro, used by
 * sys_be24_to_cpu()/sys_cpu_to_be24()/sys_le24_to_cpu()/sys_cpu_to_le24(),
 * reverses the byte order of a 24-bit value held in the low 3 bytes of a
 * 32-bit container. Also confirms that the top byte
 * (bits 31:24), which is not part of the logical value, does not leak into
 * the result when it holds non-zero garbage instead of the usual zero
 * padding.
 *
 * Test steps:
 * - Byte-swap a known, cleanly zero-padded 24-bit value with BSWAP_24().
 * - Byte-swap the same 24-bit value again, this time with non-zero garbage
 *   in the unused top byte.
 *
 * Expected result:
 * - Both calls return the same expected byte-reversed value.
 *
 * @see BSWAP_24()
 */
ZTEST(byteorder, test_bswap_24)
{
	uint32_t expected = 0xd2e1f0;
	uint32_t clean_val = 0xf0e1d2;
	uint32_t dirty_val = 0xaaf0e1d2;
	uint32_t result;

	result = BSWAP_24(clean_val);
	zexpect_equal(result, expected, "BSWAP_24() failed");

	result = BSWAP_24(dirty_val);
	zexpect_equal(result, expected, "BSWAP_24() leaked MSB padding into the result");
}

/**
 * @brief Verify BSWAP_32() reverses the byte order of a 32-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that the BSWAP_32() internal helper macro, used by
 * sys_be32_to_cpu()/sys_cpu_to_be32()/sys_le32_to_cpu()/sys_cpu_to_le32(),
 * reverses the byte order of a 32-bit value.
 *
 * Test steps:
 * - Byte-swap a known 32-bit value with BSWAP_32().
 *
 * Expected result:
 * - The result equals the expected byte-reversed value.
 *
 * @see BSWAP_32()
 */
ZTEST(byteorder, test_bswap_32)
{
	uint32_t val = 0xf0e1d2c3;
	uint32_t expected = 0xc3d2e1f0;
	uint32_t result = BSWAP_32(val);

	zassert_equal(result, expected, "BSWAP_32() failed");
}

/**
 * @brief Verify BSWAP_40() reverses the byte order of a 40-bit value and
 *        ignores garbage in its unused MSB padding bytes.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that the BSWAP_40() internal helper macro, used by
 * sys_be40_to_cpu()/sys_cpu_to_be40()/sys_le40_to_cpu()/sys_cpu_to_le40(),
 * reverses the byte order of a 40-bit value held in the low 5 bytes of a
 * 64-bit container. Also confirms that the top 3
 * bytes (bits 63:40), which are not part of the logical value, do not leak
 * into the result when they hold non-zero garbage instead of the usual
 * zero padding.
 *
 * Test steps:
 * - Byte-swap a known, cleanly zero-padded 40-bit value with BSWAP_40().
 * - Byte-swap the same 40-bit value again, this time with non-zero garbage
 *   in the unused top 3 bytes.
 *
 * Expected result:
 * - Both calls return the same expected byte-reversed value.
 *
 * @see BSWAP_40()
 */
ZTEST(byteorder, test_bswap_40)
{
	uint64_t expected = 0xb4c3d2e1f0;
	uint64_t clean_val = 0xf0e1d2c3b4;
	uint64_t dirty_val = 0xaabbccf0e1d2c3b4;
	uint64_t result;

	result = BSWAP_40(clean_val);
	zexpect_equal(result, expected, "BSWAP_40() failed");

	result = BSWAP_40(dirty_val);
	zexpect_equal(result, expected, "BSWAP_40() leaked MSB padding into the result");
}

/**
 * @brief Verify BSWAP_48() reverses the byte order of a 48-bit value and
 *        ignores garbage in its unused MSB padding bytes.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that the BSWAP_48() internal helper macro, used by
 * sys_be48_to_cpu()/sys_cpu_to_be48()/sys_le48_to_cpu()/sys_cpu_to_le48(),
 * reverses the byte order of a 48-bit value held in the low 6 bytes of a
 * 64-bit container. Also confirms that the top 2
 * bytes (bits 63:48), which are not part of the logical value, do not leak
 * into the result when they hold non-zero garbage instead of the usual
 * zero padding.
 *
 * Test steps:
 * - Byte-swap a known, cleanly zero-padded 48-bit value with BSWAP_48().
 * - Byte-swap the same 48-bit value again, this time with non-zero garbage
 *   in the unused top 2 bytes.
 *
 * Expected result:
 * - Both calls return the same expected byte-reversed value.
 *
 * @see BSWAP_48()
 */
ZTEST(byteorder, test_bswap_48)
{
	uint64_t expected = 0xa5b4c3d2e1f0;
	uint64_t clean_val = 0xf0e1d2c3b4a5;
	uint64_t dirty_val = 0xaabbf0e1d2c3b4a5;
	uint64_t result;

	result = BSWAP_48(clean_val);
	zexpect_equal(result, expected, "BSWAP_48() failed");

	result = BSWAP_48(dirty_val);
	zexpect_equal(result, expected, "BSWAP_48() leaked MSB padding into the result");
}

/**
 * @brief Verify BSWAP_64() reverses the byte order of a 64-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that the BSWAP_64() internal helper macro, used by
 * sys_be64_to_cpu()/sys_cpu_to_be64()/sys_le64_to_cpu()/sys_cpu_to_le64(),
 * reverses the byte order of a 64-bit value.
 *
 * Test steps:
 * - Byte-swap a known 64-bit value with BSWAP_64().
 *
 * Expected result:
 * - The result equals the expected byte-reversed value.
 *
 * @see BSWAP_64()
 */
ZTEST(byteorder, test_bswap_64)
{
	uint64_t val = 0xf0e1d2c3b4a59687;
	uint64_t expected = 0x8796a5b4c3d2e1f0;
	uint64_t result = BSWAP_64(val);

	zassert_equal(result, expected, "BSWAP_64() failed");
}

/**
 * @brief Verify sys_get_be64() decodes a big-endian 64-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_be64() interprets a byte buffer as a big-endian
 * (most-significant-byte-first) 64-bit integer independent of host endianness.
 *
 * Test steps:
 * - Decode a known 8-byte big-endian buffer with sys_get_be64().
 *
 * Expected result:
 * - The returned value equals the expected 64-bit integer.
 *
 * @see sys_get_be64()
 */
ZTEST(byteorder, test_sys_get_be64)
{
	uint64_t val = 0xf0e1d2c3b4a59687, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87
	};

	tmp = sys_get_be64(buf);

	zassert_equal(tmp, val, "sys_get_be64() failed");
}

/**
 * @brief Verify sys_put_be64() encodes a 64-bit value as big-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_be64() serializes a 64-bit integer into a byte buffer
 * in big-endian (most-significant-byte-first) order independent of host
 * endianness.
 *
 * Test steps:
 * - Encode a known 64-bit value with sys_put_be64() into a buffer.
 *
 * Expected result:
 * - The buffer matches the expected big-endian byte sequence.
 *
 * @see sys_put_be64()
 */
ZTEST(byteorder, test_sys_put_be64)
{
	uint64_t val = 0xf0e1d2c3b4a59687;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87
	};
	uint8_t tmp[sizeof(uint64_t)];

	sys_put_be64(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint64_t), "sys_put_be64() failed");
}

/**
 * @brief Verify sys_get_be40() decodes a big-endian 40-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_be40() interprets a 5-byte buffer as a big-endian
 * 40-bit integer, zero-extended into a 64-bit result, independent of host
 * endianness.
 *
 * Test steps:
 * - Decode a known 5-byte big-endian buffer with sys_get_be40().
 *
 * Expected result:
 * - The returned value equals the expected 40-bit integer.
 *
 * @see sys_get_be40()
 */
ZTEST(byteorder, test_sys_get_be40)
{
	uint64_t val = 0xf0e1d2c3b4, tmp;
	uint8_t buf[] = {0xf0, 0xe1, 0xd2, 0xc3, 0xb4};

	tmp = sys_get_be40(buf);

	zassert_equal(tmp, val, "sys_get_be64() failed");
}

/**
 * @brief Verify sys_put_be40() encodes a 40-bit value as big-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_be40() serializes the low 40 bits of a value into a
 * 5-byte buffer in big-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 40-bit value with sys_put_be40() into a 5-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected big-endian byte sequence.
 *
 * @see sys_put_be40()
 */
ZTEST(byteorder, test_sys_put_be40)
{
	uint64_t val = 0xf0e1d2c3b4;
	uint8_t buf[] = {0xf0, 0xe1, 0xd2, 0xc3, 0xb4};
	uint8_t tmp[sizeof(buf)];

	sys_put_be40(val, tmp);
	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_be40() failed");
}

/**
 * @brief Verify sys_get_be48() decodes a big-endian 48-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_be48() interprets a 6-byte buffer as a big-endian
 * 48-bit integer, zero-extended into a 64-bit result, independent of host
 * endianness.
 *
 * Test steps:
 * - Decode a known 6-byte big-endian buffer with sys_get_be48().
 *
 * Expected result:
 * - The returned value equals the expected 48-bit integer.
 *
 * @see sys_get_be48()
 */
ZTEST(byteorder, test_sys_get_be48)
{
	uint64_t val = 0xf0e1d2c3b4a5, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5
	};

	tmp = sys_get_be48(buf);

	zassert_equal(tmp, val, "sys_get_be64() failed");
}

/**
 * @brief Verify sys_put_be48() encodes a 48-bit value as big-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_be48() serializes the low 48 bits of a value into a
 * 6-byte buffer in big-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 48-bit value with sys_put_be48() into a 6-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected big-endian byte sequence.
 *
 * @see sys_put_be48()
 */
ZTEST(byteorder, test_sys_put_be48)
{
	uint64_t val = 0xf0e1d2c3b4a5;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5
	};
	uint8_t tmp[sizeof(buf)];

	sys_put_be48(val, tmp);
	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_be48() failed");
}

/**
 * @brief Verify sys_get_be32() decodes a big-endian 32-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_be32() interprets a 4-byte buffer as a big-endian
 * 32-bit integer independent of host endianness.
 *
 * Test steps:
 * - Decode a known 4-byte big-endian buffer with sys_get_be32().
 *
 * Expected result:
 * - The returned value equals the expected 32-bit integer.
 *
 * @see sys_get_be32()
 */
ZTEST(byteorder, test_sys_get_be32)
{
	uint32_t val = 0xf0e1d2c3, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3
	};

	tmp = sys_get_be32(buf);

	zassert_equal(tmp, val, "sys_get_be32() failed");
}

/**
 * @brief Verify sys_put_be32() encodes a 32-bit value as big-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_be32() serializes a 32-bit integer into a 4-byte
 * buffer in big-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 32-bit value with sys_put_be32() into a 4-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected big-endian byte sequence.
 *
 * @see sys_put_be32()
 */
ZTEST(byteorder, test_sys_put_be32)
{
	uint64_t val = 0xf0e1d2c3;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2, 0xc3
	};
	uint8_t tmp[sizeof(uint32_t)];

	sys_put_be32(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint32_t), "sys_put_be32() failed");
}

/**
 * @brief Verify sys_get_be24() decodes a big-endian 24-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_be24() interprets a 3-byte buffer as a big-endian
 * 24-bit integer, zero-extended into a 32-bit result, independent of host
 * endianness.
 *
 * Test steps:
 * - Decode a known 3-byte big-endian buffer with sys_get_be24().
 *
 * Expected result:
 * - The returned value equals the expected 24-bit integer.
 *
 * @see sys_get_be24()
 */
ZTEST(byteorder, test_sys_get_be24)
{
	uint32_t val = 0xf0e1d2, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2
	};

	tmp = sys_get_be24(buf);

	zassert_equal(tmp, val, "sys_get_be24() failed");
}

/**
 * @brief Verify sys_put_be24() encodes a 24-bit value as big-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_be24() serializes the low 24 bits of a value into a
 * 3-byte buffer in big-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 24-bit value with sys_put_be24() into a 3-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected big-endian byte sequence.
 *
 * @see sys_put_be24()
 */
ZTEST(byteorder, test_sys_put_be24)
{
	uint64_t val = 0xf0e1d2;
	uint8_t buf[] = {
		0xf0, 0xe1, 0xd2
	};
	uint8_t tmp[sizeof(buf)];

	sys_put_be24(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_be24() failed");
}

/**
 * @brief Verify sys_get_be16() decodes a big-endian 16-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_be16() interprets a 2-byte buffer as a big-endian
 * 16-bit integer independent of host endianness.
 *
 * Test steps:
 * - Decode a known 2-byte big-endian buffer with sys_get_be16().
 *
 * Expected result:
 * - The returned value equals the expected 16-bit integer.
 *
 * @see sys_get_be16()
 */
ZTEST(byteorder, test_sys_get_be16)
{
	uint32_t val = 0xf0e1, tmp;
	uint8_t buf[] = {
		0xf0, 0xe1
	};

	tmp = sys_get_be16(buf);

	zassert_equal(tmp, val, "sys_get_be16() failed");
}

/**
 * @brief Verify sys_put_be16() encodes a 16-bit value as big-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_be16() serializes a 16-bit integer into a 2-byte
 * buffer in big-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 16-bit value with sys_put_be16() into a 2-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected big-endian byte sequence.
 *
 * @see sys_put_be16()
 */
ZTEST(byteorder, test_sys_put_be16)
{
	uint64_t val = 0xf0e1;
	uint8_t buf[] = {
		0xf0, 0xe1
	};
	uint8_t tmp[sizeof(uint16_t)];

	sys_put_be16(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint16_t), "sys_put_be16() failed");
}

/**
 * @brief Verify sys_get_le16() decodes a little-endian 16-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_le16() interprets a 2-byte buffer as a little-endian
 * (least-significant-byte-first) 16-bit integer independent of host endianness.
 *
 * Test steps:
 * - Decode a known 2-byte little-endian buffer with sys_get_le16().
 *
 * Expected result:
 * - The returned value equals the expected 16-bit integer.
 *
 * @see sys_get_le16()
 */
ZTEST(byteorder, test_sys_get_le16)
{
	uint32_t val = 0xf0e1, tmp;
	uint8_t buf[] = {
		0xe1, 0xf0
	};

	tmp = sys_get_le16(buf);

	zassert_equal(tmp, val, "sys_get_le16() failed");
}

/**
 * @brief Verify sys_put_le16() encodes a 16-bit value as little-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_le16() serializes a 16-bit integer into a 2-byte
 * buffer in little-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 16-bit value with sys_put_le16() into a 2-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected little-endian byte sequence.
 *
 * @see sys_put_le16()
 */
ZTEST(byteorder, test_sys_put_le16)
{
	uint64_t val = 0xf0e1;
	uint8_t buf[] = {
		0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint16_t)];

	sys_put_le16(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint16_t), "sys_put_le16() failed");
}

/**
 * @brief Verify sys_get_le24() decodes a little-endian 24-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_le24() interprets a 3-byte buffer as a little-endian
 * 24-bit integer, zero-extended into a 32-bit result, independent of host
 * endianness.
 *
 * Test steps:
 * - Decode a known 3-byte little-endian buffer with sys_get_le24().
 *
 * Expected result:
 * - The returned value equals the expected 24-bit integer.
 *
 * @see sys_get_le24()
 */
ZTEST(byteorder, test_sys_get_le24)
{
	uint32_t val = 0xf0e1d2, tmp;
	uint8_t buf[] = {
		0xd2, 0xe1, 0xf0
	};

	tmp = sys_get_le24(buf);

	zassert_equal(tmp, val, "sys_get_le24() failed");
}

/**
 * @brief Verify sys_put_le24() encodes a 24-bit value as little-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_le24() serializes the low 24 bits of a value into a
 * 3-byte buffer in little-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 24-bit value with sys_put_le24() into a 3-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected little-endian byte sequence.
 *
 * @see sys_put_le24()
 */
ZTEST(byteorder, test_sys_put_le24)
{
	uint64_t val = 0xf0e1d2;
	uint8_t buf[] = {
		0xd2, 0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint32_t)];

	sys_put_le24(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_le24() failed");
}

/**
 * @brief Verify sys_get_le32() decodes a little-endian 32-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_le32() interprets a 4-byte buffer as a little-endian
 * 32-bit integer independent of host endianness.
 *
 * Test steps:
 * - Decode a known 4-byte little-endian buffer with sys_get_le32().
 *
 * Expected result:
 * - The returned value equals the expected 32-bit integer.
 *
 * @see sys_get_le32()
 */
ZTEST(byteorder, test_sys_get_le32)
{
	uint32_t val = 0xf0e1d2c3, tmp;
	uint8_t buf[] = {
		0xc3, 0xd2, 0xe1, 0xf0
	};

	tmp = sys_get_le32(buf);

	zassert_equal(tmp, val, "sys_get_le32() failed");
}

/**
 * @brief Verify sys_put_le32() encodes a 32-bit value as little-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_le32() serializes a 32-bit integer into a 4-byte
 * buffer in little-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 32-bit value with sys_put_le32() into a 4-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected little-endian byte sequence.
 *
 * @see sys_put_le32()
 */
ZTEST(byteorder, test_sys_put_le32)
{
	uint64_t val = 0xf0e1d2c3;
	uint8_t buf[] = {
		0xc3, 0xd2, 0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint32_t)];

	sys_put_le32(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint32_t), "sys_put_le32() failed");
}

/**
 * @brief Verify sys_get_le40() decodes a little-endian 40-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_le40() interprets a 5-byte buffer as a little-endian
 * 40-bit integer, zero-extended into a 64-bit result, independent of host
 * endianness.
 *
 * Test steps:
 * - Decode a known 5-byte little-endian buffer with sys_get_le40().
 *
 * Expected result:
 * - The returned value equals the expected 40-bit integer.
 *
 * @see sys_get_le40()
 */
ZTEST(byteorder, test_sys_get_le40)
{
	uint64_t val = 0xf0e1d2c3b4, tmp;
	uint8_t buf[] = {0xb4, 0xc3, 0xd2, 0xe1, 0xf0};

	tmp = sys_get_le40(buf);

	zassert_equal(tmp, val, "sys_get_le40() failed");
}

/**
 * @brief Verify sys_put_le40() encodes a 40-bit value as little-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_le40() serializes the low 40 bits of a value into a
 * 5-byte buffer in little-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 40-bit value with sys_put_le40() into a 5-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected little-endian byte sequence.
 *
 * @see sys_put_le40()
 */
ZTEST(byteorder, test_sys_put_le40)
{
	uint64_t val = 0xf0e1d2c3b4;
	uint8_t buf[] = {0xb4, 0xc3, 0xd2, 0xe1, 0xf0};
	uint8_t tmp[sizeof(uint64_t)];

	sys_put_le40(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_le40() failed");
}

/**
 * @brief Verify sys_get_le48() decodes a little-endian 48-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_le48() interprets a 6-byte buffer as a little-endian
 * 48-bit integer, zero-extended into a 64-bit result, independent of host
 * endianness.
 *
 * Test steps:
 * - Decode a known 6-byte little-endian buffer with sys_get_le48().
 *
 * Expected result:
 * - The returned value equals the expected 48-bit integer.
 *
 * @see sys_get_le48()
 */
ZTEST(byteorder, test_sys_get_le48)
{
	uint64_t val = 0xf0e1d2c3b4a5, tmp;
	uint8_t buf[] = {
		0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
	};

	tmp = sys_get_le48(buf);

	zassert_equal(tmp, val, "sys_get_le48() failed");
}

/**
 * @brief Verify sys_put_le48() encodes a 48-bit value as little-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_le48() serializes the low 48 bits of a value into a
 * 6-byte buffer in little-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 48-bit value with sys_put_le48() into a 6-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected little-endian byte sequence.
 *
 * @see sys_put_le48()
 */
ZTEST(byteorder, test_sys_put_le48)
{
	uint64_t val = 0xf0e1d2c3b4a5;
	uint8_t buf[] = {
		0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint64_t)];

	sys_put_le48(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(buf), "sys_put_le48() failed");
}

/**
 * @brief Verify sys_get_le64() decodes a little-endian 64-bit value.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_le64() interprets an 8-byte buffer as a little-endian
 * 64-bit integer independent of host endianness.
 *
 * Test steps:
 * - Decode a known 8-byte little-endian buffer with sys_get_le64().
 *
 * Expected result:
 * - The returned value equals the expected 64-bit integer.
 *
 * @see sys_get_le64()
 */
ZTEST(byteorder, test_sys_get_le64)
{
	uint64_t val = 0xf0e1d2c3b4a59687, tmp;
	uint8_t buf[] = {
		0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
	};

	tmp = sys_get_le64(buf);

	zassert_equal(tmp, val, "sys_get_le64() failed");
}

/**
 * @brief Verify sys_put_le64() encodes a 64-bit value as little-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_le64() serializes a 64-bit integer into an 8-byte
 * buffer in little-endian order independent of host endianness.
 *
 * Test steps:
 * - Encode a known 64-bit value with sys_put_le64() into an 8-byte buffer.
 *
 * Expected result:
 * - The buffer matches the expected little-endian byte sequence.
 *
 * @see sys_put_le64()
 */
ZTEST(byteorder, test_sys_put_le64)
{
	uint64_t val = 0xf0e1d2c3b4a59687;
	uint8_t buf[] = {
		0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0
	};
	uint8_t tmp[sizeof(uint64_t)];

	sys_put_le64(val, tmp);

	zassert_mem_equal(tmp, buf, sizeof(uint64_t), "sys_put_le64() failed");
}

/**
 * @brief Verify sys_uint16_to_array() builds a host-endian byte array.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that the sys_uint16_to_array() initializer macro expands a 16-bit
 * constant into a byte array laid out in the host's native byte order, so the
 * expected layout depends on CONFIG_LITTLE_ENDIAN.
 *
 * Test steps:
 * - Initialize a 2-byte array from a known constant using the macro.
 * - Compare against the layout selected for the build's endianness.
 *
 * Expected result:
 * - The array matches the expected host-endian byte sequence.
 *
 * @see sys_uint16_to_array()
 */
ZTEST(byteorder, test_sys_uint16_to_array)
{
	#define VAL 0xf0e1
	uint8_t tmp[sizeof(uint16_t)] = sys_uint16_to_array(VAL);
	uint8_t buf[] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xe1, 0xf0),
		(0xf0, 0xe1))
	};

	zassert_mem_equal(tmp, buf, sizeof(uint16_t), "sys_uint16_to_array() failed");
	#undef VAL
}

/**
 * @brief Verify sys_uint32_to_array() builds a host-endian byte array.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that the sys_uint32_to_array() initializer macro expands a 32-bit
 * constant into a byte array laid out in the host's native byte order, so the
 * expected layout depends on CONFIG_LITTLE_ENDIAN.
 *
 * Test steps:
 * - Initialize a 4-byte array from a known constant using the macro.
 * - Compare against the layout selected for the build's endianness.
 *
 * Expected result:
 * - The array matches the expected host-endian byte sequence.
 *
 * @see sys_uint32_to_array()
 */
ZTEST(byteorder, test_sys_uint32_to_array)
{
	#define VAL 0xf0e1d2c3
	uint8_t tmp[sizeof(uint32_t)] = sys_uint32_to_array(VAL);
	uint8_t buf[] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xc3, 0xd2, 0xe1, 0xf0),
		(0xf0, 0xe1, 0xd2, 0xc3))
	};

	zassert_mem_equal(tmp, buf, sizeof(uint32_t), "sys_uint32_to_array() failed");
	#undef VAL
}

/**
 * @brief Verify sys_uint64_to_array() builds a host-endian byte array.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that the sys_uint64_to_array() initializer macro expands a 64-bit
 * constant into a byte array laid out in the host's native byte order, so the
 * expected layout depends on CONFIG_LITTLE_ENDIAN.
 *
 * Test steps:
 * - Initialize an 8-byte array from a known constant using the macro.
 * - Compare against the layout selected for the build's endianness.
 *
 * Expected result:
 * - The array matches the expected host-endian byte sequence.
 *
 * @see sys_uint64_to_array()
 */
ZTEST(byteorder, test_sys_uint64_to_array)
{
	#define VAL 0xf0e1d2c3b4a59687
	uint8_t tmp[sizeof(uint64_t)] = sys_uint64_to_array(VAL);
	uint8_t buf[] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0),
		(0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87))
	};

	zassert_mem_equal(tmp, buf, sizeof(uint64_t), "sys_uint64_to_array() failed");
	#undef VAL
}

/**
 * @brief Verify sys_le_to_cpu() converts a little-endian buffer to host order.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_le_to_cpu() reorders a multi-byte buffer in place from
 * little-endian into the host's native byte order: a no-op on little-endian
 * hosts and a full byte reversal on big-endian hosts. This validates the
 * arbitrary-length conversion handles odd-sized buffers correctly.
 *
 * Test steps:
 * - Convert a 9-byte little-endian buffer in place with sys_le_to_cpu().
 * - Compare against the layout selected for the build's endianness.
 *
 * Expected result:
 * - The buffer matches the expected host-order byte sequence.
 *
 * @see sys_le_to_cpu()
 */
ZTEST(byteorder, test_sys_le_to_cpu)
{
	uint8_t val[9] = { 0x87, 0x95, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab };
	uint8_t expected[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x95, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab),
		(0xab, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x95, 0x87))
	};

	sys_le_to_cpu(val, sizeof(val));

	zassert_mem_equal(val, expected, sizeof(expected), "sys_le_to_cpu() failed");
}

/**
 * @brief Verify sys_cpu_to_le() converts a host-order buffer to little-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_cpu_to_le() reorders a multi-byte buffer in place from the
 * host's native byte order into little-endian: a no-op on little-endian hosts
 * and a full byte reversal on big-endian hosts, for an arbitrary-length buffer.
 *
 * Test steps:
 * - Convert a 9-byte host-order buffer in place with sys_cpu_to_le().
 * - Compare against the layout selected for the build's endianness.
 *
 * Expected result:
 * - The buffer matches the expected little-endian byte sequence.
 *
 * @see sys_cpu_to_le()
 */
ZTEST(byteorder, test_sys_cpu_to_le)
{
	uint8_t val[9] = { 0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab };
	uint8_t expected[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x96, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab),
		(0xab, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87))
	};

	sys_cpu_to_le(val, sizeof(val));

	zassert_mem_equal(val, expected, sizeof(expected), "sys_cpu_to_le() failed");
}

/**
 * @brief Verify sys_be_to_cpu() converts a big-endian buffer to host order.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_be_to_cpu() reorders a multi-byte buffer in place from
 * big-endian into the host's native byte order: a full byte reversal on
 * little-endian hosts and a no-op on big-endian hosts, for an arbitrary-length
 * buffer.
 *
 * Test steps:
 * - Convert a 9-byte big-endian buffer in place with sys_be_to_cpu().
 * - Compare against the layout selected for the build's endianness.
 *
 * Expected result:
 * - The buffer matches the expected host-order byte sequence.
 *
 * @see sys_be_to_cpu()
 */
ZTEST(byteorder, test_sys_be_to_cpu)
{
	uint8_t val[9] = { 0x87, 0x97, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab };
	uint8_t expected[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xab, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x97, 0x87),
		(0x87, 0x97, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab))
	};

	sys_be_to_cpu(val, sizeof(val));

	zassert_mem_equal(val, expected, sizeof(expected), "sys_be_to_cpu() failed");
}

/**
 * @brief Verify sys_cpu_to_be() converts a host-order buffer to big-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_cpu_to_be() reorders a multi-byte buffer in place from the
 * host's native byte order into big-endian: a full byte reversal on
 * little-endian hosts and a no-op on big-endian hosts, for an arbitrary-length
 * buffer.
 *
 * Test steps:
 * - Convert a 9-byte host-order buffer in place with sys_cpu_to_be().
 * - Compare against the layout selected for the build's endianness.
 *
 * Expected result:
 * - The buffer matches the expected big-endian byte sequence.
 *
 * @see sys_cpu_to_be()
 */
ZTEST(byteorder, test_sys_cpu_to_be)
{
	uint8_t val[9] = { 0x87, 0x98, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab };
	uint8_t expected[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xab, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x98, 0x87),
		(0x87, 0x98, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xab))
	};

	sys_cpu_to_be(val, sizeof(val));

	zassert_mem_equal(val, expected, sizeof(expected), "sys_cpu_to_be() failed");
}

/**
 * @brief Verify sys_put_le() stores a host-order buffer as little-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_le() copies a host-order source buffer into a
 * destination buffer in little-endian order: an identity copy on little-endian
 * hosts and a byte reversal on big-endian hosts, for an arbitrary length.
 *
 * Test steps:
 * - Store a 9-byte host-order buffer into a destination with sys_put_le().
 * - Compare the destination against the layout selected for the build's
 *   endianness.
 *
 * Expected result:
 * - The destination matches the expected little-endian byte sequence.
 *
 * @see sys_put_le()
 */
ZTEST(byteorder, test_sys_put_le)
{
	uint8_t host[9] = { 0x87, 0x12, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba };
	uint8_t prot[9] = { 0 };
	uint8_t expected[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x12, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba),
		(0xba, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x12, 0x87))
	};

	sys_put_le(prot, host, sizeof(prot));

	zassert_mem_equal(prot, expected, sizeof(expected), "sys_put_le() failed");
}

/**
 * @brief Verify sys_put_be() stores a host-order buffer as big-endian.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_put_be() copies a host-order source buffer into a
 * destination buffer in big-endian order: a byte reversal on little-endian
 * hosts and an identity copy on big-endian hosts, for an arbitrary length.
 *
 * Test steps:
 * - Store a 9-byte host-order buffer into a destination with sys_put_be().
 * - Compare the destination against the layout selected for the build's
 *   endianness.
 *
 * Expected result:
 * - The destination matches the expected big-endian byte sequence.
 *
 * @see sys_put_be()
 */
ZTEST(byteorder, test_sys_put_be)
{
	uint8_t host[9] = { 0x87, 0x13, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba };
	uint8_t prot[9] = { 0 };
	uint8_t expected[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xba, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x13, 0x87),
		(0x87, 0x13, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba))
	};

	sys_put_be(prot, host, sizeof(prot));

	zassert_mem_equal(prot, expected, sizeof(expected), "sys_put_be() failed");
}

/**
 * @brief Verify sys_get_le() reads a little-endian buffer into host order.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_le() copies a little-endian source buffer into a
 * host-order destination buffer: an identity copy on little-endian hosts and a
 * byte reversal on big-endian hosts, for an arbitrary length.
 *
 * Test steps:
 * - Read a 9-byte little-endian buffer into a destination with sys_get_le().
 * - Compare the destination against the layout selected for the build's
 *   endianness.
 *
 * Expected result:
 * - The destination matches the expected host-order byte sequence.
 *
 * @see sys_get_le()
 */
ZTEST(byteorder, test_sys_get_le)
{
	uint8_t prot[9] = { 0x87, 0x14, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba };
	uint8_t host[9] = { 0 };
	uint8_t expected[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0x87, 0x14, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba),
		(0xba, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x14, 0x87))
	};

	sys_get_le(host, prot, sizeof(host));

	zassert_mem_equal(host, expected, sizeof(expected), "sys_get_le() failed");
}

/**
 * @brief Verify sys_get_be() reads a big-endian buffer into host order.
 *
 * @ingroup kernel_byteorder_tests
 *
 * @details
 * Confirms that sys_get_be() copies a big-endian source buffer into a
 * host-order destination buffer: a byte reversal on little-endian hosts and an
 * identity copy on big-endian hosts, for an arbitrary length.
 *
 * Test steps:
 * - Read a 9-byte big-endian buffer into a destination with sys_get_be().
 * - Compare the destination against the layout selected for the build's
 *   endianness.
 *
 * Expected result:
 * - The destination matches the expected host-order byte sequence.
 *
 * @see sys_get_be()
 */
ZTEST(byteorder, test_sys_get_be)
{
	uint8_t prot[9] = { 0x87, 0x15, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba };
	uint8_t host[9] = { 0 };
	uint8_t expected[9] = {
		COND_CODE_1(CONFIG_LITTLE_ENDIAN,
		(0xba, 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x15, 0x87),
		(0x87, 0x15, 0xa5, 0xb4, 0xc3, 0xd2, 0xe1, 0xf0, 0xba))
	};

	sys_get_be(host, prot, sizeof(host));

	zassert_mem_equal(host, expected, sizeof(expected), "sys_get_be() failed");
}

/**
 * @}
 */


extern void *common_setup(void);
ZTEST_SUITE(byteorder, NULL, common_setup, NULL, NULL, NULL);
