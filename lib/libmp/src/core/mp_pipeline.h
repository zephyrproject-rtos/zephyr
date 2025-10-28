/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpPipeline.
 */

#ifndef __MP_PIPELINE_H__
#define __MP_PIPELINE_H__

#include <stdint.h>

#include "mp_bin.h"
#include "mp_bus.h"
#include "mp_element.h"

/**
 * @{
 */

typedef struct _MpPipeline MpPipeline;

#define MP_PIPELINE(self) ((MpPipeline *)self)

/**
 * @brief MpPipeline structure
 *
 * Contains all the state and timing information needed to manage
 * a complete media processing pipeline.
 */
struct _MpPipeline {
	/** Base bin container */
	MpBin bin;
	/** Message bus for pipeline communication */
	MpBus bus;
	/** The running time - total time spent in PLAYING state without being flushed */
	uint64_t stream_time;
	/** Extra delay added to base_time to compensate for computing delays when setting
	 * elements to PLAYING */
	uint64_t delay;
};

/**
 * @brief Initialize a pipeline
 *
 * Initializes the pipeline structure, including the base bin and message bus.
 *
 * @param self Pointer to the @ref MpElement to initialize as a pipeline
 */
void mp_pipeline_init(MpElement *self);

/**
 * @brief Create a new pipeline
 *
 * Creates a new pipeline instance. The pipeline serves as the top-level container for media
 * processing elements.
 *
 * @param name Name to assign to the new pipeline element
 * @return Pointer to the newly created @ref MpElement pipeline, or NULL on failure
 */
MpElement *mp_pipeline_new(const char *name);

/**
 * @}
 */

#endif /* __MP_PIPELINE_H__ */
