/*
 * Copyright (c) 2025 Chengyi Lux Zhang <iansseijelly@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "tacit.h"

void l_trace_sink_dma_read(LTraceSinkDmaType *sink_dma, uint8_t *buffer) {
  sink_dma->TR_SK_DMA_FLUSH = 1;
  while (sink_dma->TR_SK_DMA_FLUSH_DONE == 0) {
    // printf("waiting for flush done\n");
  }
  // printf("[l_trace_sink_dma_read] flush done\n");
  uint64_t count = sink_dma->TR_SK_DMA_COUNT;
  printf("[l_trace_sink_dma_read] count: %lld\n", count);
  for (uint8_t i = 0; i < count; i++) {
    printf("%02x ", buffer[i]);
  }
  printf("\n");
}