/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/crypto/crypto_otp_mem.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/drivers/fpga.h>
#include <zephyr/drivers/misc/rapidsi/rapidsi_ofe.h>

#include <rapidsi_scu.h>

#define ATTR_INF "\x1b[32;1m" // ANSI_COLOR_GREEN
#define ATTR_ERR "\x1b[31;1m" // ANSI_COLOR_RED
#define ATTR_RST "\x1b[37;1m" // ANSI_COLOR_RESET
#define ATTR_HIL "\x1b[33;1m" // ANSI_COLOR_HIGHLIGHT

#define DEBUG_PRINT false

#if (CONFIG_SPI_ANDES_ATCSPI200 && CONFIG_SPI_NOR)
	#define FLASH_RW_SIZE   255
	#define FLASH_BASE_ADDR 0xB0000000

	void Flash_Test(const struct device *flash, const struct device *dma, uint32_t FLASH_ADDR,
			uint8_t EVEN_ODD_MUL, uint8_t FORMATTER)
	{
		int errorcode = 0;
		uint8_t flash_data_write[FLASH_RW_SIZE] = {0};
		uint8_t flash_data_read[FLASH_RW_SIZE] = {0};
		errorcode = flash_read(flash, FLASH_ADDR, flash_data_read, FLASH_RW_SIZE);
		if (errorcode < 0) {
			printf("%s Error reading from flash with code:%d%s\n", ATTR_ERR, errorcode,
				ATTR_RST);
		} else {
			printf("%s Reading Back Before Erasing Flash%s\n", ATTR_INF, ATTR_RST);
			#if DEBUG_PRINT
				for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
					printf("%s %d%s", ATTR_INF, flash_data_read[i],i%FORMATTER==0?"\n":"");
				} printf("\n");
			#endif
		}
		errorcode = flash_erase(flash, FLASH_ADDR, 0x1000);
		if (errorcode < 0) {
			printf("%s\nError Erasing the flash at 0x%08x offset errorcode:%d%s\n", ATTR_ERR,
				FLASH_ADDR, errorcode, ATTR_RST);
		} else {
			printf("%s\nSuccessfully Erased Flash\n", ATTR_RST);
			errorcode = flash_read(flash, FLASH_ADDR, flash_data_read, FLASH_RW_SIZE);
			if (errorcode < 0) {
				printf("%s Error reading from flash with code:%d%s\n", ATTR_ERR, errorcode,
					ATTR_RST);
			} else {
				printf("%s Reading Back After Erasing Area of Flash%s\n", ATTR_INF,
					ATTR_RST);
				for (uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
					if (flash_data_read[i] != 0xff) {
						errorcode = -1;
					}
				}
			}
			if (errorcode == -1) {
				printf("%s\nFlash erase at 0x%08x did not produce correct results%s\n",
					ATTR_ERR, FLASH_ADDR, ATTR_RST);
				#if DEBUG_PRINT
					for(uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
						printf("%s 0x%02x%s", ATTR_INF,
					flash_data_read[i],i%FORMATTER==0?"\n":""); } printf("\n");
				#endif
			} else {
				printf("%s\nSuccessfully performed erase to flash with code:%d%s\n",
					ATTR_INF, errorcode, ATTR_RST);
			}
			errorcode = 0;
		}

		if (errorcode == 0) {
			printf("%s Writing the following data After Erasing Flash%s\n", ATTR_INF, ATTR_RST);
			for (uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
				flash_data_write[i] = ((rand() % (FLASH_RW_SIZE - 1 + 1)) + 1) +
							((rand() % (FLASH_RW_SIZE - 1 + 1)) + 1);
				#if DEBUG_PRINT
					printf("%s %d%s", ATTR_INF, flash_data_write[i],i%FORMATTER==0?"\n":"");
				#endif
			}
			#if DEBUG_PRINT
				printf("\n");
			#endif
			errorcode = flash_write(flash, FLASH_ADDR, flash_data_write, FLASH_RW_SIZE);
		}

		if (errorcode < 0) {
			printf("%s \nError writing to flash with code:%d%s\n", ATTR_ERR, errorcode,
				ATTR_RST);
		} else {
			printf("%s \nSuccessfully written to flash with code:%d Resetting the reading "
				"buffer....%s\n",
				ATTR_INF, errorcode, ATTR_RST);
			memset(flash_data_read, 0, FLASH_RW_SIZE);
			errorcode = flash_read(flash, FLASH_ADDR, flash_data_read, FLASH_RW_SIZE);
			if (errorcode < 0) {
				printf("%s Error reading from flash with code:%d%s\n", ATTR_ERR, errorcode,
					ATTR_RST);
			} else {
				printf("%s Successfully Read from flash with code:%d%s\n", ATTR_INF,
					errorcode, ATTR_RST);
				bool Data_Validated = true;
				uint8_t Mismatch_count = 0;
				for (uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
					if (flash_data_read[i] != flash_data_write[i]) {
						Data_Validated = false;
						Mismatch_count++;
						printf("%s %d - Read:%d != Write:%d%s\n", ATTR_ERR, i,
							flash_data_read[i], flash_data_write[i], ATTR_RST);
					}
				}
				if (!Data_Validated) {
					printf("%s Flash Integrity Check Failed With %d Mismatched "
						"Entries%s\n",
						ATTR_ERR, Mismatch_count, ATTR_RST);
				} else {
					printf("%s Flash Integrity Check Passed!!!%s\n", ATTR_INF,
						ATTR_RST);
				}
			}
		}

		if (errorcode == 0) {
			uint8_t *MemMapReadAddr = (uint8_t *)(FLASH_BASE_ADDR + FLASH_ADDR);
			uint8_t MemMapReadDestAddr[FLASH_RW_SIZE];
			{
				printf("CPU Memory Mapped Reading Test from address:%p\n", MemMapReadAddr);
				bool memmaptestfail = false;
				for (uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
					if (MemMapReadAddr[i] != flash_data_write[i]) {
						printf("%s %d%s", ATTR_ERR, MemMapReadAddr[i],
							i % FORMATTER == 0 ? "\n" : "");
						memmaptestfail = true;
					}
					#if DEBUG_PRINT
						else {
							printf("%s %d%s", ATTR_INF,
							MemMapReadAddr[i],i%FORMATTER==0?"\n":"");
						}
					#endif
				}
				printf("\n");
				if (memmaptestfail) {
					printf("%s CPU Memory mapped read failed\n", ATTR_ERR);
				} else {
					printf("%s CPU Memory mapped read passed\n", ATTR_INF);
				}
			}

			if (dma != NULL) {
				printf("%s DMA Memory Mapped Reading Test from address:%p\n", ATTR_RST,
					MemMapReadAddr);
				// Configure DMA Block
				struct dma_block_config lv_dma_block_config = {
					.block_size = FLASH_RW_SIZE,
					.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
					.dest_address = (uint32_t)&MemMapReadDestAddr[0],
					.dest_reload_en = 0,
					.dest_scatter_count = 0,
					.dest_scatter_en = 0,
					.dest_scatter_interval = 0,
					.fifo_mode_control = 0,
					.flow_control_mode = 0,
					.next_block = NULL,
					.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
					.source_address = (uint32_t)&MemMapReadAddr[0],
					.source_gather_count = 0,
					.source_gather_en = 0,
					.source_gather_interval = 0,
					.source_reload_en = 0,
					._reserved = 0};
				// Configure DMA
				struct dma_config lv_Dma_Config = {.block_count = 1,
								.channel_direction = MEMORY_TO_MEMORY,
								.channel_priority = 1,
								.complete_callback_en = 0,
								.cyclic = 0,
								.dest_burst_length = 1,
								.dest_chaining_en = 0,
								.dest_data_size = 1,
								.dest_handshake = 1,
								.dma_callback = NULL,
								.dma_slot = 0,
								.error_callback_dis = 1,
								.head_block = &lv_dma_block_config,
								.linked_channel = 0,
								.source_burst_length = 1,
								.source_chaining_en = 0,
								.source_data_size = 1,
								.user_data = NULL};
				struct dma_status stat = {0};
				printf("%s DMA Configuration...\n", ATTR_RST);
				int dma_error = dma_config(dma, 1, &lv_Dma_Config);
				k_msleep(10);
				if (dma_error) {
					printf("%s Error %d Configuration...\n", ATTR_RST, dma_error);
				} else {
					// Call DMA Read
					printf("%s DMA Starting...\n", ATTR_INF);
					dma_error = dma_start(dma, 1);
					k_msleep(10);
				}
				(void)dma_get_status(dma, 1, &stat);
				while (stat.pending_length > 1) {
					dma_error = dma_get_status(dma, 1, &stat);
					if (dma_error) {
						printf("Error %d dma_status...\n", dma_error);
						break;
					} else {
						printf("dma_pending_length:%d \n", stat.pending_length);
					}
				}

				if (!dma_error) {
					bool memmaptestfail = false;
					for (uint8_t i = 0; i < FLASH_RW_SIZE; i++) {
						if (MemMapReadDestAddr[i] != flash_data_write[i]) {
							printf("%s %d%s", ATTR_ERR, MemMapReadDestAddr[i],
								i % FORMATTER == 0 ? "\n" : "");
							memmaptestfail = true;
						}
						#if DEBUG_PRINT
							else {
								printf("%s %d%s", ATTR_INF,
								MemMapReadDestAddr[i],i%FORMATTER==0?"\n":"");
							}
						#endif
					}
					printf("\n");
					if (memmaptestfail) {
						printf("%s DMA Memory mapped read failed\n", ATTR_ERR);
					} else {
						printf("%s DMA Memory mapped read passed\n", ATTR_INF);
					}
				} else {
					printf("Error %d DMA Start...\n", dma_error);
				}
			}
		}
	}
#endif

#if CONFIG_COUNTER_ANDES_ATCPIT100
void CounterCallBack(const struct device *dev, void *UserData)
{
	uint32_t *lvData = ((uint32_t *)UserData);
	printf("\nandestech_atcpit100 %s # %d\n", __func__, *lvData);
	*lvData += 1;
}

void CounterAlarmCallBack(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *UserData)
{
	uint32_t *lvData = ((uint32_t *)UserData);
	printf("\nandestech_atcpit100 %s # %d\n", __func__, *lvData);
}

static uint32_t s_CallBackData = 0;

void CounterTest(const struct device *pit)
{
	struct counter_alarm_cfg lvCntAlarmCfg = {0};
	lvCntAlarmCfg.callback = CounterAlarmCallBack;
	lvCntAlarmCfg.flags = COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	lvCntAlarmCfg.ticks = 24000;
	lvCntAlarmCfg.user_data = (void *)&s_CallBackData;

	struct counter_top_cfg lvCntTopCfg = {0};

	lvCntTopCfg.callback = CounterCallBack;
	lvCntTopCfg.user_data = (void *)&s_CallBackData;
	lvCntTopCfg.flags = COUNTER_TOP_CFG_DONT_RESET;
	lvCntTopCfg.ticks = 6000;

	int lvErrorCode = 0;

	lvErrorCode = counter_set_channel_alarm(pit, 0, &lvCntAlarmCfg);
	if (lvErrorCode < 0) {
		printf("%s%s(%d) counter Alarm set error code:%d %s\n", ATTR_ERR, __func__,
		       __LINE__, lvErrorCode, ATTR_RST);
	}
	/*
	lvErrorCode = counter_set_top_value(pit, &lvCntTopCfg);
	if(lvErrorCode < 0) {
		printf("%s%s(%d) counter setting top error code:%d %s\n", \
		ATTR_ERR,__func__,__LINE__,lvErrorCode,ATTR_RST);
	} else {
		int lvErrorCode = counter_start(pit);
		if(lvErrorCode < 0) {
			printf("%s%s(%d) counter start error code:%d %s\n", \
			ATTR_ERR,__func__,__LINE__,lvErrorCode,ATTR_RST);
		}
	}
	*/
}
#endif

#if CONFIG_CRYPTO_PUF_SECURITY_OTP
void pufs_otp_test(const struct device *pufs_otp)
{
	uint8_t slot_data[32] = {0};
	enum crypto_otp_lock lvLock = CRYPTO_OTP_RW;
	uint16_t totalSlots = 0, bytesPerSlot = 0;

	if (otp_info(pufs_otp, &totalSlots, &bytesPerSlot) != 0) {
		printf("%s%s(%d) OTP_Info Failed%s\n", ATTR_ERR, __func__, __LINE__, ATTR_RST);
	} else {
		printf("%s%s(%d) OTP_Info... Slots:%d Bytes Per Slot:%d%s\n", ATTR_INF, __func__,
		       __LINE__, totalSlots, bytesPerSlot, ATTR_RST);
	}

	if (otp_get_lock(pufs_otp, 12, &lvLock) != 0) {
		printf("%s%s(%d) otp_get_lock Failed%s\n", ATTR_ERR, __func__, __LINE__, ATTR_RST);
	} else {
		printf("%s%s(%d) otp_lock: %d %s\n", ATTR_INF, __func__, __LINE__, lvLock,
		       ATTR_RST);
	}

	for (uint8_t i = 0; i < bytesPerSlot; i++) {
		slot_data[i] = (i + i) * 2;
	}
	if (otp_write(pufs_otp, 12, slot_data, 18) != 0) {
		if (lvLock != CRYPTO_OTP_RW) {
			printf("%s%s(%d) otp_write Failed as Expected %s\n", ATTR_RST, __func__,
			       __LINE__, ATTR_RST);
		} else {
			printf("%s%s(%d) otp_write Failed unExpectedly %s\n", ATTR_ERR, __func__,
			       __LINE__, ATTR_RST);
		}
	} else {
		printf("%s%s(%d) otp_write Passed %s\n", ATTR_RST, __func__, __LINE__, ATTR_RST);
		memset(slot_data, 0, bytesPerSlot);
		if (otp_read(pufs_otp, 12, slot_data, 18) != 0) {
			if (lvLock == CRYPTO_OTP_NA) {
				printf("%s%s(%d) otp_read Failed as Expected %s\n", ATTR_RST,
				       __func__, __LINE__, ATTR_RST);
			} else {
				printf("%s%s(%d) otp_read Failed unExpectedly %s\n", ATTR_ERR,
				       __func__, __LINE__, ATTR_RST);
			}
		} else {
			printf("%s", ATTR_INF);
			for (uint8_t i = 0; i < bytesPerSlot; i++) {
				printf("%d ", slot_data[i]);
			}
			printf("\n");
			printf("%s", ATTR_RST);
		}
	}
	if (otp_set_lock(pufs_otp, 12, 18, CRYPTO_OTP_RO) != 0) {
		printf("%s%s(%d) otp_set_lock Failed%s\n", ATTR_ERR, __func__, __LINE__, ATTR_RST);
	} else {
		if (otp_get_lock(pufs_otp, 12, &lvLock) != 0) {
			printf("%s%s(%d) otp_get_lock Failed%s\n", ATTR_ERR, __func__, __LINE__,
			       ATTR_RST);
		} else {
			printf("%s%s(%d) otp_lock: %d %s\n", ATTR_INF, __func__, __LINE__, lvLock,
			       ATTR_RST);
		}
		for (uint8_t i = 0; i < bytesPerSlot; i++) {
			slot_data[i] = (i + i) * 2;
		}
		if (otp_write(pufs_otp, 12, slot_data, 18) != 0) {
			if (lvLock != CRYPTO_OTP_RW) {
				printf("%s%s(%d) otp_write Failed as Expected %s\n", ATTR_RST,
				       __func__, __LINE__, ATTR_RST);
			} else {
				printf("%s%s(%d) otp_write Failed unExpectedly %s\n", ATTR_ERR,
				       __func__, __LINE__, ATTR_RST);
			}
		} else {
			printf("%s%s(%d) otp_write Passed UnExpectedly %s\n", ATTR_ERR, __func__,
			       __LINE__, ATTR_RST);
		}
	}
}
#endif
#if CONFIG_CRYPTO_PUF_SECURITY
void pufs_hash_sg_test(const struct device *pufs)
{
	int status = 0;
	static uint8_t pufs_sample_data1[] = {
		"My name is Junaid and I work at the Rapid Silicon and WorkFromH."};
	static uint8_t pufs_sample_data2[] = {
		"My name is Junaid and I work at the Rapid Silicon and WorkFromH."};
	static uint8_t pufs_sample_data3[] = {"123456789123456789"};

	static uint8_t pufs_sample_data_sha256_out[32] = {0};
	static const uint8_t pufs_sample_data_sha256[] = {
		0xb2, 0x69, 0xec, 0xc6, 0x0f, 0x13, 0x37, 0xf3, 0xf3, 0xf8, 0x29,
		0xc1, 0x0e, 0xc5, 0x83, 0xa6, 0x82, 0x25, 0x97, 0x32, 0x71, 0x78,
		0xd1, 0xb8, 0xe5, 0x95, 0x05, 0xd1, 0xbe, 0xe7, 0x43, 0xca};
	const uint8_t pufs_sample_data1_len = strlen(pufs_sample_data1);
	const uint8_t pufs_sample_data2_len = strlen(pufs_sample_data2);
	const uint8_t pufs_sample_data3_len = strlen(pufs_sample_data3);

	printf("%s%s(%d) Length of sample data to operate on:%d %s\n", ATTR_INF, __func__, __LINE__,
	       pufs_sample_data1_len + pufs_sample_data2_len + pufs_sample_data3_len, ATTR_RST);

	struct hash_ctx lvHashCtx = {.device = pufs,
				     .drv_sessn_state = NULL,
				     .flags = CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
				     .started = true};

	struct hash_pkt lvHashPkt1 = {0}, lvHashPkt2 = {0}, lvHashPkt3 = {0};

	lvHashPkt1.ctx = &lvHashCtx;
	lvHashPkt1.head = true;
	lvHashPkt1.tail = true;
	lvHashPkt1.in_buf = pufs_sample_data1;
	lvHashPkt1.in_hash = NULL;
	lvHashPkt1.in_hash_len = 0;
	lvHashPkt1.in_len = pufs_sample_data1_len;
	lvHashPkt1.next = &lvHashPkt2;
	lvHashPkt1.out_buf = pufs_sample_data_sha256_out;
	lvHashPkt1.out_len = 0;
	lvHashPkt1.prev_len = 0;

	lvHashPkt2.ctx = &lvHashCtx;
	lvHashPkt2.head = true;
	lvHashPkt2.tail = true;
	lvHashPkt2.in_buf = pufs_sample_data2;
	lvHashPkt2.in_hash = NULL;
	lvHashPkt2.in_hash_len = 0;
	lvHashPkt2.in_len = pufs_sample_data2_len;
	lvHashPkt2.next = &lvHashPkt3;
	lvHashPkt2.out_buf = pufs_sample_data_sha256_out;
	lvHashPkt2.out_len = 0;
	lvHashPkt2.prev_len = 0;

	lvHashPkt3.ctx = &lvHashCtx;
	lvHashPkt3.head = true;
	lvHashPkt3.tail = true;
	lvHashPkt3.in_buf = pufs_sample_data3;
	lvHashPkt3.in_hash = NULL;
	lvHashPkt3.in_hash_len = 0;
	lvHashPkt3.in_len = pufs_sample_data3_len;
	lvHashPkt3.next = NULL;
	lvHashPkt3.out_buf = pufs_sample_data_sha256_out;
	lvHashPkt3.out_len = 0;
	lvHashPkt3.prev_len = 0;

	printf("Pkt1.rd_addr:0x%08x Pkt2.rd_addr:0x%08x Pkt3.rd_addr:0x%08x\r\n",
	       (uint32_t)lvHashPkt1.in_buf, (uint32_t)lvHashPkt2.in_buf,
	       (uint32_t)lvHashPkt3.in_buf);
	printf("Size struct hash pkt:%d\r\n", sizeof(struct hash_pkt));
	printf("data1_size:%d data2_size:%d data3_size:%d\r\n", pufs_sample_data1_len,
	       pufs_sample_data2_len, pufs_sample_data3_len);

	status = hash_begin_session(pufs, &lvHashCtx, CRYPTO_HASH_ALGO_SHA256);
	if (status != 0) {
		printf("%s%s(%d) hash_begin_session status = %d %s\n", ATTR_ERR, __func__, __LINE__,
		       status, ATTR_RST);
	} else {
		status = hash_compute(&lvHashCtx, &lvHashPkt1);
	}
	if (status != 0) {
		printf("%s%s(%d) hash_compute status = %d %s\n", ATTR_ERR, __func__, __LINE__,
		       status, ATTR_RST);
	} else {
		for (uint8_t i = 0; i < lvHashPkt1.out_len; i++) {
			if (lvHashPkt1.out_buf[i] != pufs_sample_data_sha256[i]) {
				#if DEBUG_PRINT
					printf("%s%s(%d) out_buf[%d]0x%02x != in_buf[%d]:0x%02x %s\n",
					ATTR_ERR,__func__,__LINE__,
					i,lvHashPkt1.out_buf[i],i,pufs_sample_data_sha256[i],ATTR_RST);
				#endif
				status = -EINVAL;
			} else {
				#if DEBUG_PRINT
					printf("%s%s(%d) out_buf[%d]0x%02x == in_buf[%d]:0x%02x %s\n",
					ATTR_INF,__func__,__LINE__,
					i,lvHashPkt1.out_buf[i],i,pufs_sample_data_sha256[i],ATTR_RST);
				#endif
			}
		}
		if (status != 0) {
			printf("%s%s(%d) hash_comparison failed%s\n", ATTR_ERR, __func__, __LINE__,
			       ATTR_RST);
		} else {
			printf("%s%s(%d) hash_comparison Passed = %d %s\n", ATTR_INF, __func__,
			       __LINE__, status, ATTR_RST);
		}
		status = hash_free_session(pufs, &lvHashCtx);
		if (status != 0) {
			printf("%s%s(%d) hash_free_session status = %d %s\n", ATTR_ERR, __func__,
			       __LINE__, status, ATTR_RST);
		} else {
			printf("%s%s(%d) hash_free_session Passed = %d %s\n", ATTR_INF, __func__,
			       __LINE__, status, ATTR_RST);
		}
	}
}

void pufs_hash_test(const struct device *pufs)
{
	int status = 0;
	static uint8_t *pufs_sample_data = "This is Rapid Silicon'z Zephyr Port";
	static uint8_t pufs_sample_data_sha256_out[32] = {0};
	static const uint8_t pufs_sample_data_sha256[] = {
		0xfa, 0x71, 0xc2, 0x19, 0xea, 0x58, 0x4d, 0xac, 0x36, 0xd5, 0x3e,
		0xca, 0xe4, 0x2a, 0x8c, 0x14, 0x4b, 0xc1, 0xc0, 0x03, 0xfd, 0x36,
		0x3f, 0x71, 0xd9, 0x30, 0x96, 0xc4, 0xaa, 0x64, 0xe0, 0x4c};
	const uint8_t pufs_sample_data_len = strlen(pufs_sample_data);
	printf("%s%s(%d) Length of sample data to operate on:%d %s\n", ATTR_INF, __func__, __LINE__,
	       pufs_sample_data_len, ATTR_RST);
	struct hash_ctx lvHashCtx = {.device = pufs,
				     .drv_sessn_state = NULL,
				     .flags = CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
				     .started = false};

	struct hash_pkt lvHashPkt = {.ctx = &lvHashCtx,
				     .head = true,
				     .tail = true,
				     .in_buf = pufs_sample_data,
				     .in_hash = NULL,
				     .in_len = pufs_sample_data_len,
				     .next = NULL,
				     .out_buf = pufs_sample_data_sha256_out,
				     .prev_len = 0,
				     .out_len = 0};
	status = hash_begin_session(pufs, &lvHashCtx, CRYPTO_HASH_ALGO_SHA256);
	if (status != 0) {
		printf("%s%s(%d) hash_begin_session status = %d %s\n", ATTR_ERR, __func__, __LINE__,
		       status, ATTR_RST);
	} else {
		status = hash_compute(&lvHashCtx, &lvHashPkt);
	}
	if (status != 0) {
		printf("%s%s(%d) hash_compute status = %d %s\n", ATTR_ERR, __func__, __LINE__,
		       status, ATTR_RST);
	} else {
		for (uint8_t i = 0; i < lvHashPkt.out_len; i++) {
			if (lvHashPkt.out_buf[i] != pufs_sample_data_sha256[i]) {
			#if DEBUG_PRINT
				printf("%s%s(%d) out_buf[%d]0x%02x != in_buf[%d]:0x%02x %s\n",
				ATTR_ERR,__func__,__LINE__,
				i,lvHashPkt.out_buf[i],i,pufs_sample_data_sha256[i],ATTR_RST);
			#endif
				status = -EINVAL;
			} else {
			#if DEBUG_PRINT
				printf("%s%s(%d) out_buf[%d]0x%02x == in_buf[%d]:0x%02x %s\n",
				ATTR_INF,__func__,__LINE__,
				i,lvHashPkt.out_buf[i],i,pufs_sample_data_sha256[i],ATTR_RST);
			#endif
			}
		}
		if (status != 0) {
			printf("%s%s(%d) hash_comparison failed%s\n", ATTR_ERR, __func__, __LINE__,
			       ATTR_RST);
		} else {
			printf("%s%s(%d) hash_comparison Passed = %d %s\n", ATTR_INF, __func__,
			       __LINE__, status, ATTR_RST);
		}
		status = hash_free_session(pufs, &lvHashCtx);
		if (status != 0) {
			printf("%s%s(%d) hash_free_session status = %d %s\n", ATTR_ERR, __func__,
			       __LINE__, status, ATTR_RST);
		} else {
			printf("%s%s(%d) hash_free_session Passed = %d %s\n", ATTR_INF, __func__,
			       __LINE__, status, ATTR_RST);
		}
	}
}

void pufs_decryption_test(const struct device *pufs)
{
	int lvStatus = 0;

	struct cipher_ctx lvCipherCtx = {0};
	struct cipher_pkt lvCipherPkt = {0};

	uint8_t enc32_key[] = {0x30, 0x13, 0x53, 0x33, 0x44, 0x12, 0xff, 0x22, 0x31, 0x53, 0xbb,
			       0x13, 0x07, 0x19, 0x84, 0x0d, 0xd5, 0x3c, 0x3f, 0xc0, 0x09, 0x83,
			       0x11, 0x35, 0x30, 0x2d, 0xdc, 0xc0, 0x0b, 0xb9, 0x4e, 0x30};
	uint8_t iv[] = {0xaa, 0x34, 0x55, 0x32, 0xe9, 0x58, 0xaa, 0xf3,
			0xc0, 0xde, 0x20, 0xa6, 0xa9, 0x21, 0x44, 0xe8};

	uint8_t cipher_text[] = {
		0xd0, 0x07, 0x29, 0x7b, 0x56, 0x2c, 0x2c, 0xa7, 0x43, 0x24, 0x68, 0xd8, 0x58,
		0xb4, 0xc1, 0x9f, 0x7c, 0x05, 0x7c, 0x98, 0x03, 0x28, 0x19, 0x9d, 0x93, 0xff,
		0x85, 0xd7, 0xdb, 0x8f, 0xf1, 0x07, 0x18, 0xa5, 0x1f, 0x99, 0x04, 0x73, 0x84,
		0x4f, 0x4f, 0x79, 0xba, 0x2e, 0x70, 0x82, 0xb1, 0x39, 0x5d, 0x3f, 0x3e, 0xf8,
		0x44, 0x81, 0xf6, 0x89, 0x17, 0x3a, 0x29, 0x43, 0x0a, 0x41, 0x24, 0xff, 0x57,
		0xa8, 0xbb, 0x0b, 0x0e, 0xcf, 0xb3, 0xed, 0x8e, 0x1d, 0x49, 0x03, 0x86, 0x77,
		0x61, 0x68, 0x59, 0x7c, 0xf0, 0x61, 0x98, 0x11, 0x1b, 0xe5, 0xe3, 0x95, 0x6d,
		0x81, 0x49, 0xdc, 0xa0, 0x4b, 0x97, 0xb4, 0xe6, 0x40, 0xd5, 0x26, 0xb6, 0xb7,
		0xd8, 0xab, 0x61, 0xc1, 0xa7, 0x5a, 0xdc, 0x5e, 0xa9, 0x25, 0x6f, 0xcd, 0x25,
		0xb6, 0xd0, 0x42, 0xa1, 0xd8, 0xd5, 0xdc, 0xcf, 0xc6, 0xaa, 0xdf};

	uint8_t *plain_text =
		"Plain Text: This is Junaid Aslam from Rapid Silicon doing Decryption Test...\n";
	uint8_t decrypted_text[256] = {0};

	lvCipherCtx.app_sessn_state = NULL;
	lvCipherCtx.device = pufs;
	lvCipherCtx.flags = (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX |
			     CAP_NO_ENCRYPTION);
	lvCipherCtx.key.bit_stream = enc32_key;
	lvCipherCtx.key_source = CRYPTO_KEY_SW;
	lvCipherCtx.keylen = 32;
	lvCipherCtx.mode_params.ctr_info.ctr_len = 16;
	lvCipherCtx.mode_params.ctr_info.readback_ctr = false;
	lvCipherCtx.ops.cipher_mode = CRYPTO_CIPHER_MODE_CTR;

	lvCipherPkt.auto_increment = true;
	lvCipherPkt.ctx = &lvCipherCtx;
	lvCipherPkt.in_buf = cipher_text;
	lvCipherPkt.in_len = (sizeof(cipher_text) / sizeof(cipher_text[0]));
	lvCipherPkt.out_buf = decrypted_text;
	lvCipherPkt.prev_len = 0;

	printf("%s Decrypting %d bytes %s\n", ATTR_INF, lvCipherPkt.in_len, ATTR_RST);

	lvStatus = cipher_begin_session(pufs, &lvCipherCtx, CRYPTO_CIPHER_ALGO_AES,
					CRYPTO_CIPHER_MODE_CTR, CRYPTO_CIPHER_OP_DECRYPT);
	if (lvStatus != 0) {
		printf("%s cipher_begin_session Failed! %s\n", ATTR_ERR, ATTR_RST);
	} else {
		printf("%s cipher_begin_session Success! %s\n", ATTR_INF, ATTR_RST);
		lvStatus = cipher_ctr_op(&lvCipherCtx, &lvCipherPkt, (uint8_t *)iv);
	}

	if (lvStatus != 0) {
		printf("%s cipher_ctr_op Failed! Status:%d %s\n", ATTR_ERR, lvStatus, ATTR_RST);
	} else {
		printf("%s cipher_ctr_op Success! %s\n", ATTR_INF, ATTR_RST);
		printf("%s Original: %s%s", ATTR_HIL, plain_text, ATTR_RST);
		printf("%s Result:   %s%s", ATTR_HIL, lvCipherPkt.out_buf, ATTR_RST);
		bool comparison_ok = true;
		for (int i = 0; i < strlen(lvCipherPkt.out_buf); i++) {
			if (lvCipherPkt.out_buf[i] != plain_text[i]) {
				#if DEBUG_PRINT
					printf("Orig[%d]:0x%02x(%c) != Res[%d]:0x%02x(%c)\n", i,
					plain_text[i], plain_text[i], i, lvCipherPkt.out_buf[i],
					lvCipherPkt.out_buf[i]);
				#endif
				comparison_ok = false;
				break;
			}
			#if DEBUG_PRINT
				else {
					printf("Orig[%d]:0x%02x(%c) == Res[%d]:0x%02x(%c)\n", i, plain_text[i],
					plain_text[i], i, lvCipherPkt.out_buf[i], lvCipherPkt.out_buf[i]);
				}
			#endif
		}
		if (!comparison_ok) {
			printf("%s cipher decryption Failed! %s\n", ATTR_ERR, ATTR_RST);
		} else {
			printf("%s cipher decryption Passed! %s\n", ATTR_INF, ATTR_RST);
		}
	}

	lvStatus = cipher_free_session(pufs, &lvCipherCtx);
	if (lvStatus != 0) {
		printf("%s cipher_free_session Failed! %s\n", ATTR_ERR, ATTR_RST);
	} else {
		printf("%s cipher_free_session Success! %s\n", ATTR_INF, ATTR_RST);
	}
}

const uint32_t bop_ec256_image_addr[] = {
	0x4c425346, 0x00010001, 0x00000001, 0x00000540, 0xa020005c, 0xa020005c, 0x00000040,
	0x00000000, 0x00000010, 0x00000040, 0x00000040, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x5bef0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x5a41e854, 0xbd7831f5, 0x8a722abc, 0x0f194645, 0xfca0b124,
	0x4c32ac0f, 0x119da725, 0x017d57f0, 0xc3c72358, 0xf5a403c4, 0x35f48b62, 0xe3bcf592,
	0xe0ae8930, 0xe0ef26dc, 0xad99438b, 0xbc0b8b9e, 0xf746712e, 0x36b64de7, 0x1b2dd12b,
	0x016f428d, 0xa6375c54, 0xb19e1329, 0x29abce2e, 0x01d2ef46, 0x4522360f, 0x4646a2b0,
	0x4b9e632d, 0x65bd20b3, 0xa9a48e98, 0x78cdc0d2, 0xb80be124, 0x72e406e9};

const uint32_t bop_rsa_image_addr[] = {
	0x4c425346, 0x00010001, 0x00000001, 0x00000540, 0xa020005c, 0xa020005c, 0x00000040,
	0x00000000, 0x00000020, 0x00000104, 0x00000100, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x467b0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x37c2cac9, 0x8c32e765, 0xa15e1200, 0x25c486b2, 0x8734d7b4,
	0xfa26c29d, 0x2ff972ae, 0x0adebf22, 0xff6f29e6, 0x3c50778d, 0xfa02d8da, 0x78600d02,
	0xbca88565, 0x163ca9be, 0x73f0da10, 0x3ed20b1c, 0xcce46d11, 0x2a49278f, 0x12248db6,
	0xe50b6755, 0xa431193c, 0x6c308699, 0xe0c41dbf, 0x50c791fe, 0xd3f876b9, 0x7cf1df93,
	0x16941322, 0xaa1ead8f, 0xe3753f54, 0xc9bb7bcc, 0x79200bff, 0x7d76c2d4, 0xa7acc7f7,
	0xff567a99, 0xbfa4f5dc, 0x064238c8, 0xe9699208, 0xa6b5daf3, 0xa3a2ad30, 0x32bafe37,
	0x54e1dacb, 0xda540799, 0x95547760, 0x302cd650, 0x9d428238, 0x62bbdadd, 0xe531a353,
	0x282e8f7d, 0x973e7ba1, 0x0dd65973, 0x4ab77a02, 0x53ca63ae, 0x176010c2, 0x58bc890d,
	0x025f55f8, 0x0e1e579b, 0xe19cdb04, 0x4bb6caf5, 0xad438b2c, 0xeda4466b, 0x3cabf8dc,
	0x0785aca7, 0xd0350f09, 0x79cac3c1, 0x00010001, 0x00000000, 0x00000000, 0x00000000,
	0xebe1a78f, 0x2e6e03ed, 0x4ee1354e, 0x2052aa4d, 0xbb4cb958, 0x230e0232, 0x862cb015,
	0x58dee9e7, 0xfde8f54e, 0xc96e44be, 0x607271f8, 0x983ecea7, 0xe8770c08, 0x0dc7599b,
	0x590e8ff8, 0x690188e6, 0x66dfaf5e, 0xf6335a68, 0x2c0b118f, 0x3733a790, 0x9c938f42,
	0x3ce28852, 0x2c74cda7, 0x7dad40de, 0xe7a80299, 0xa3e92677, 0x4b328cd7, 0x760976e1,
	0x92a152e4, 0x4225f6c3, 0xb3d87236, 0x43a9be7e, 0xbbb12076, 0x66433975, 0x353e3019,
	0x010926e2, 0x00d08453, 0x045cfcd6, 0xc763d656, 0xe873faee, 0xb1fc2dc1, 0x0530f6e3,
	0x7064e3ff, 0xc6062a2f, 0xe1a3d120, 0xf728ad4f, 0xa7159b1e, 0x8f3e3a92, 0x97be047f,
	0x75a4d27b, 0x5fc4d749, 0x194d52ce, 0x0199ccba, 0x0de3e3ec, 0x830b6dd5, 0xff35543e,
	0x5b0b9d15, 0x8258edb7, 0xfee5009e, 0xed692f55, 0x5fbe279a, 0x9fa66ee6, 0x3913674d,
	0x671285c3};

struct rs_bop_header {
	uint32_t bop_id;
	uint16_t bop_hdr_version;
	uint16_t sign_tool_version;
	uint32_t binary_version;
	uint32_t binary_len;
	uint32_t load_addr;
	uint32_t entry_addr;
	uint32_t offset_to_binary;
	uint32_t offset_to_next_header;
	uint8_t sig_algo;
	uint8_t option;
	uint8_t enc_algo;
	uint8_t iv_len;
	uint16_t pub_key_len;
	uint16_t enc_key_len;
	uint16_t sig_len;
	uint8_t compression_algo;
	uint8_t bin_pad_bytes; // Number of padding bytes in the payload binary or
			       // bitstream
	uint16_t xcb_header_size;
	uint8_t padding[16]; // To make BOP header 64 bytes for scatter gather hash
			     // calculation
	uint16_t crc_16;
};

void pufs_rsa2048_verify_test(const struct device *pufs)
{
	int lvStatus = 0;

	struct sign_ctx lvSignCtx = {0};
	struct sign_pkt lvSignPkt = {0};

	uint32_t pub_key_padding = 12;

	struct rs_bop_header *bop_hdr = (struct rs_bop_header *)bop_rsa_image_addr;

	uint8_t *pub_key = (uint8_t *)bop_rsa_image_addr + bop_hdr->offset_to_binary +
			   bop_hdr->binary_len + bop_hdr->iv_len;
	uint8_t *sig = (uint8_t *)pub_key + bop_hdr->pub_key_len + pub_key_padding;

	lvSignCtx.app_sessn_state = NULL;
	lvSignCtx.device = pufs;
	lvSignCtx.drv_sessn_state = NULL;
	lvSignCtx.flags = (CAP_INPLACE_OPS | CAP_SYNC_OPS);
	lvSignCtx.pub_key = pub_key;
	lvSignCtx.sig = sig;
	lvSignCtx.ops.signing_algo = CRYPTO_SIGN_ALGO_RSA2048;
	lvSignCtx.ops.signing_mode = CRYPTO_SIGN_VERIFY;

	lvSignPkt.ctx = &lvSignCtx;
	lvSignPkt.in_buf = (uint8_t *)bop_hdr;
	lvSignPkt.in_len = bop_hdr->binary_len + sizeof(struct rs_bop_header) + 272;
	lvSignPkt.next = NULL;

	lvStatus = sign_begin_session(pufs, &lvSignCtx, CRYPTO_SIGN_ALGO_RSA2048);
	if (lvStatus != 0) {
		printf("\n%ssign_begin_session RSA2048 Failed! %s\n", ATTR_ERR, ATTR_RST);
	} else {
		printf("%ssign_begin_session RSA2048 Success! %s\n", ATTR_INF, ATTR_RST);
		lvStatus = sign_op_handler(&lvSignCtx, &lvSignPkt);
	}

	if (lvStatus != 0) {
		printf("%s sign_op_handler RSA2048 Failed! Status:%d %s\n", ATTR_ERR, lvStatus, ATTR_RST);
	} else {
		printf("%s sign_op_handler RSA2048 Success! %s\n", ATTR_INF, ATTR_RST);
	}

	lvStatus = sign_free_session(pufs, &lvSignCtx);
	if (lvStatus != 0) {
		printf("%s sign_free_session RSA2048 Failed! %s\n", ATTR_ERR, ATTR_RST);
	} else {
		printf("%s sign_free_session RSA2048 Success! %s\n", ATTR_INF, ATTR_RST);
	}
}

void pufs_ecdsa256_verify_test(const struct device *pufs)
{
	int lvStatus = 0;

	struct sign_ctx lvSignCtx = {0};
	struct sign_pkt lvSignPkt = {0};

	struct rs_bop_header *bop_hdr = (struct rs_bop_header *)bop_ec256_image_addr;

	uint8_t *pub_key = (uint8_t *)bop_ec256_image_addr + bop_hdr->offset_to_binary +
			   bop_hdr->binary_len + bop_hdr->iv_len;
	uint8_t *sig = (uint8_t *)pub_key + bop_hdr->pub_key_len;

	lvSignCtx.app_sessn_state = NULL;
	lvSignCtx.device = pufs;
	lvSignCtx.drv_sessn_state = NULL;
	lvSignCtx.flags = (CAP_INPLACE_OPS | CAP_SYNC_OPS);
	lvSignCtx.pub_key = pub_key;
	lvSignCtx.sig = sig;
	lvSignCtx.ops.signing_algo = CRYPTO_SIGN_ALGO_ECDSA256;
	lvSignCtx.ops.signing_mode = CRYPTO_SIGN_VERIFY;

	lvSignPkt.ctx = &lvSignCtx;
	lvSignPkt.in_buf = (uint8_t *)bop_hdr;
	lvSignPkt.in_len = bop_hdr->binary_len + sizeof(struct rs_bop_header) + 64; // 64 = public key x(32) + y(32)
	lvSignPkt.next = NULL;

	lvStatus = sign_begin_session(pufs, &lvSignCtx, CRYPTO_SIGN_ALGO_ECDSA256);
	if (lvStatus != 0) {
		printf("\n%ssign_begin_session ECDSA256 Failed! %s\n", ATTR_ERR, ATTR_RST);
	} else {
		printf("%ssign_begin_session ECDSA256 Success! %s\n", ATTR_INF, ATTR_RST);
		lvStatus = sign_op_handler(&lvSignCtx, &lvSignPkt);
	}

	if (lvStatus != 0) {
		printf("%s sign_op_handler ECDSA256 Failed! Status:%d %s\n", ATTR_ERR, lvStatus, ATTR_RST);
	} else {
		printf("%s sign_op_handler ECDSA256 Success! %s\n", ATTR_INF, ATTR_RST);
	}

	lvStatus = sign_free_session(pufs, &lvSignCtx);
	if (lvStatus != 0) {
		printf("%s sign_free_session ECDSA256 Failed! %s\n", ATTR_ERR, ATTR_RST);
	} else {
		printf("%s sign_free_session ECDSA256 Success! %s\n", ATTR_INF, ATTR_RST);
	}
}
#endif

#if CONFIG_RAPIDSI_OFE
void ofe_test(const struct device *ofe)
{
	if(ofe_reset(ofe, OFE_RESET_SUBSYS_FCB, true) == 0) {
		printf("%s Success with basic register read/write test of ofe FCB reset\
		%s\n", ATTR_INF, ATTR_RST);
	} else {
		printf("%s Failure with basic register read/write test of ofe FCB reset\
		%s\n", ATTR_ERR, ATTR_RST);
	}
	if(ofe_set_config_status(ofe, OFE_CFG_STATUS_DONE, true) == 0) {
		printf("%s Success with basic register read/write test of ofe config status\
		%s\n", ATTR_INF, ATTR_RST);
	} else {
		printf("%s Failure with basic register read/write test of ofe config status\
		%s\n", ATTR_ERR, ATTR_RST);
	}
}
#endif

int main(void)
{
	int Cnt = 0;
	uint8_t chip_id = 0, vendor_id = 0;

	#if DT_HAS_RAPIDSI_SCU_ENABLED
		printf("%s SPI_(%d)_IRQ_Reg_Val:[0x%08x]; DMA_(%d)_IRQ_Reg_Val:[0x%08x]\n", ATTR_RST,
			IRQ_ID_SPI, scu_get_irq_reg_val(IRQ_ID_SPI), IRQ_ID_SYSTEM_DMA,
			scu_get_irq_reg_val(IRQ_ID_SYSTEM_DMA));

		soc_get_id(&chip_id, &vendor_id);
		scu_reset();
		printf("%s to perform xCB reset\r\n",scu_xcb_reset()==0?"Success":"Failure");
	#endif

	int errorcode = 0;
	struct sensor_value lvTemp = {0}, lvVolt = {0};

	#if CONFIG_RS_FPGA_FCB
		const struct device *fcb = DEVICE_DT_GET(DT_NODELABEL(fcb));	
	#endif

	#if CONFIG_RS_FPGA_ICB
		const struct device *icb = DEVICE_DT_GET(DT_NODELABEL(icb));
	#endif

	#if CONFIG_RS_FPGA_PCB
		const struct device *pcb = DEVICE_DT_GET(DT_NODELABEL(pcb));
	#endif

	#if CONFIG_RAPIDSI_OFE
		const struct device *ofe = DEVICE_DT_GET(DT_NODELABEL(ofe));
	#endif
	#if CONFIG_DTI_PVT
		const struct device *pvt = DEVICE_DT_GET(DT_NODELABEL(pvt0));
	#endif
	#if (CONFIG_SPI_ANDES_ATCSPI200 && CONFIG_SPI_NOR)
		const struct device *spi = DEVICE_DT_GET(DT_NODELABEL(spi0));
		const struct device *flash = DEVICE_DT_GET(DT_NODELABEL(m25p32));
	#endif
	#if CONFIG_DMA_ANDES_ATCDMAC100
		const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma0));
	#endif
	#if CONFIG_COUNTER_ANDES_ATCPIT100
		const struct device *pit = DEVICE_DT_GET(DT_NODELABEL(pit0));
	#endif
	#if CONFIG_CRYPTO_PUF_SECURITY
		const struct device *pufs = DEVICE_DT_GET(DT_NODELABEL(pufs));
	#endif
	#if CONFIG_CRYPTO_PUF_SECURITY_OTP
		const struct device *pufs_otp = DEVICE_DT_GET(DT_NODELABEL(pufs_otp));
	#endif

	#if CONFIG_RAPIDSI_OFE
		ofe_test(ofe);
	#endif

	#if CONFIG_CRYPTO_PUF_SECURITY
		if ((pufs == NULL) || (!device_is_ready(pufs))) {
			printf("%s pufs has status disabled or driver is not initialized...%s\n", ATTR_ERR,
				ATTR_RST);
		} else {
			printf("%s pufs Object is Created %s\n", ATTR_INF, ATTR_RST);
			pufs_decryption_test(pufs);
			pufs_hash_test(pufs);
			pufs_hash_sg_test(pufs);
			pufs_rsa2048_verify_test(pufs);
			pufs_ecdsa256_verify_test(pufs);
		}
	#endif

	#if CONFIG_CRYPTO_PUF_SECURITY_OTP
		if ((pufs_otp == NULL) || (!device_is_ready(pufs_otp))) {
			printf("%s pufs_otp has status disabled or driver is not initialized...%s\n",
				ATTR_ERR, ATTR_RST);
		} else {
			printf("%s pufs_otp Object is Created %s\n", ATTR_INF, ATTR_RST);
			pufs_otp_test(pufs_otp);
		}
	#endif
	#if CONFIG_DTI_PVT
		if ((pvt == NULL) || (!device_is_ready(pvt))) {
			printf("%s pvt has status disabled or driver is not initialized...%s\n", ATTR_ERR,
				ATTR_RST);
		} else {
			printf("%s pvt Object is Created %s\n", ATTR_INF, ATTR_RST);
			errorcode = sensor_channel_get(pvt, SENSOR_CHAN_DIE_TEMP, &lvTemp);
			if (errorcode == 0) {
				printf("%s Error fetching temperature value. Error code:%u%s\n", ATTR_ERR,
					errorcode, ATTR_RST);
			}
			errorcode = sensor_channel_get(pvt, SENSOR_CHAN_VOLTAGE, &lvVolt);
			if (errorcode == 0) {
				printf("%s Error fetching Voltage value. Error code:%u%s\n", ATTR_ERR,
					errorcode, ATTR_RST);
			}
			printf("%s Die Temperature:%d Voltage:%d%s\n", ATTR_INF, lvTemp.val1, lvVolt.val1,
				ATTR_RST);
		}
	#endif
	#if (CONFIG_SPI_ANDES_ATCSPI200 && CONFIG_SPI_NOR && CONFIG_DMA_ANDES_ATCDMAC100)
		if (spi == NULL) {
			printf("%s spi has status disabled...%s\n", ATTR_ERR, ATTR_RST);
		} else {
			printf("%s spi Object is Created. Test Via DMA\n", ATTR_INF);
			if (flash == NULL) {
				printf("%s flash has status disabled or not initialized properly...%s\n",
					ATTR_ERR, ATTR_RST);
			} else {
				printf("%s flash Object is Created. Initialize to Test Via DMA%s\n",
					ATTR_INF, ATTR_RST);
				Flash_Test(flash, dma, 0x1000, 0, 20);
			}
		}
	#endif
	#if CONFIG_COUNTER_ANDES_ATCPIT100
		if ((pit == NULL) || !device_is_ready(pit)) {
			printf("%s pit has status disabled or driver is not initialized...%s\n", ATTR_ERR,
				ATTR_RST);
		} else {
			CounterTest(pit);
		}
	#endif

	while (true) {
		printf("%s%d - %s [CHIP_ID:0x%02x VENDOR_ID:0x%02x] Build[Date:%s Time:%s]\r",
		       ATTR_RST, Cnt++, CONFIG_BOARD_TARGET, chip_id, vendor_id, __DATE__,
		       __TIME__);
		k_msleep(1000);
	}

	return 0;
}
