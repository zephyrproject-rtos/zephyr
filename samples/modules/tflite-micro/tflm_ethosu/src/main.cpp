/*
 * Copyright 2019-2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include "inference_process.hpp"

#include <inttypes.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <zephyr/kernel.h>

/* Model data */
#include "input.h"
#include "model.h"
#include "output.h"

using namespace std;
using namespace InferenceProcess;

/****************************************************************************
 * Defines
 ****************************************************************************/

/* Number of worker threads running the inferences. There should typically be
 * one worker thread per NPU. */
#ifndef NUM_INFERENCE_TASKS
#define NUM_INFERENCE_TASKS 1
#endif

/* Number of sender tasks, that post inference requests to the worker threads. */
#ifndef NUM_JOB_TASKS
#define NUM_JOB_TASKS 2
#endif

/* Number of inferences per sender task. */
#ifndef NUM_JOBS_PER_TASK
#define NUM_JOBS_PER_TASK 2
#endif

/* Tensor arena size */
#ifdef TENSOR_ARENA_SIZE /* If defined in model.h */
#define TENSOR_ARENA_SIZE_PER_INFERENCE TENSOR_ARENA_SIZE
#else /* If not defined, use maximum available */
#define TENSOR_ARENA_SIZE_PER_INFERENCE 2000000 / NUM_INFERENCE_TASKS
#endif

/****************************************************************************
 * InferenceJob
 ****************************************************************************/

namespace
{
struct InferenceProcessParams {
	InferenceProcessParams() : tensorArena(nullptr), arenaSize(0)
	{
	}

	InferenceProcessParams(k_queue *_queue, uint8_t *_tensorArena, size_t _arenaSize)
		: queueHandle(_queue), tensorArena(_tensorArena), arenaSize(_arenaSize)
	{
	}

	k_queue *queueHandle;
	uint8_t *tensorArena;
	size_t arenaSize;
};

/* Wrapper around InferenceProcess::InferenceJob. Adds responseQueue and status
 * for Zephyr multi-tasking purposes.& */
struct xInferenceJob : public InferenceJob {
	xInferenceJob() : InferenceJob(), responseQueue(nullptr), status(false)
	{
	}

	xInferenceJob(const string &_name, const DataPtr &_networkModel,
		      const vector<DataPtr> &_input, const vector<DataPtr> &_output,
		      const vector<DataPtr> &_expectedOutput, k_queue *_queue)
		: InferenceJob(_name, _networkModel, _input, _output, _expectedOutput),
		  responseQueue(_queue), status(false)
	{
	}

	k_queue *responseQueue;
	bool status;
};

/* Number of total completed jobs, needed to exit application correctly if
 * NUM_JOB_TASKS > 1 */
volatile int totalCompletedJobs = 0;

/* TensorArena static initialisation */
const size_t arenaSize = TENSOR_ARENA_SIZE_PER_INFERENCE;

__attribute__((section("tflm_arena"), aligned(16)))
uint8_t inferenceProcessTensorArena[NUM_INFERENCE_TASKS][arenaSize];

/* Allocate and initialize heap */
void *allocateHeap(const size_t size)
{
	k_heap *heap = static_cast<k_heap *>(k_malloc(sizeof(k_heap)));
	uint8_t *buf = static_cast<uint8_t *>(k_malloc(size));

	if ((buf == nullptr) || (heap == nullptr)) {
		printk("Heap allocation failed. heap=%p, buf=%p, size=%zu\n", heap, buf, size);
		exit(1);
	}

	k_heap_init(heap, buf, size);

	return static_cast<void *>(heap);
}

/* inferenceProcessTask - Run jobs from queue with available driver */
void inferenceProcessTask(void *_name, void *heap, void *_params)
{
	string *name = static_cast<string *>(_name);
	InferenceProcessParams *params = static_cast<InferenceProcessParams *>(_params);

	/* Assign the pre allocated heap - used in the k_queue_alloc_append */
	k_thread_heap_assign(k_current_get(), static_cast<k_heap *>(heap));

	class InferenceProcess inferenceProcess(params->tensorArena, params->arenaSize);

	for (;;) {
		/* Receive inference job */
		xInferenceJob *job =
			static_cast<xInferenceJob *>(k_queue_get(params->queueHandle, Z_FOREVER));

		printk("%s: Received inference job. job=%p\n", name->c_str(), job);

		/* Run inference */
		job->status = inferenceProcess.runJob(*job);

		printk("%s: Sending inference response. job=%p\n", name->c_str(), job);

		/* Return inference message */
		int ret = k_queue_alloc_append(job->responseQueue, job);
		if (0 != ret) {
			printk("%s: Failed to send message\n", name->c_str());
			exit(1);
		}
	}

	k_thread_abort(k_current_get());
}

/* inferenceSenderTask - Creates NUM_INFERENCE_JOBS jobs, queues them, and then
 * listens for completion status */
void inferenceSenderTask(void *_name, void *heap, void *_queue)
{
	string *name = static_cast<string *>(_name);
	k_queue *inferenceQueue = static_cast<k_queue *>(_queue);
	int ret = 0;

	/* Assign the pre allocated heap - used in the k_queue_alloc_append */
	k_thread_heap_assign(k_current_get(), static_cast<k_heap *>(heap));

	/* Create queue for response messages */
	k_queue senderQueue;
	k_queue_init(&senderQueue);

	/* Loop over all jobs and push them to inference queue */
	xInferenceJob jobs[NUM_JOBS_PER_TASK];
	for (int n = 0; n < NUM_JOBS_PER_TASK; n++) {
		auto &job = jobs[n];
		job = xInferenceJob(modelName, DataPtr(networkModelData, sizeof(networkModelData)),
				    { DataPtr(inputData, sizeof(inputData)) }, {},
				    { DataPtr(expectedOutputData, sizeof(expectedOutputData)) },
				    &senderQueue);

		printk("%s: Sending inference. job=%p, name=%s\n", name->c_str(), &job,
		       job.name.c_str());

		/* Queue job */
		ret = k_queue_alloc_append(inferenceQueue, &job);
		if (0 != ret) {
			printk("%s: Failed to send message\n", name->c_str());
			exit(1);
		}
	}

	/* Listen for completion status */
	do {
		xInferenceJob *job =
			static_cast<xInferenceJob *>(k_queue_get(&senderQueue, Z_FOREVER));

		printk("%s: Received job response. job=%p, status=%u\n", name->c_str(), job,
		       job->status);

		totalCompletedJobs++;

		ret += job->status;
		if (job->status != 0) {
			break;
		}
	} while (totalCompletedJobs < NUM_JOBS_PER_TASK * NUM_JOB_TASKS);

	exit(ret);
}

} /* namespace */

/* Zephyr application. NOTE: Additional tasks may require increased heap size. */
int main()
{
	struct {
		k_thread thread;
		k_tid_t id;
	} threads[NUM_JOB_TASKS + NUM_INFERENCE_TASKS];
	size_t nthreads = 0;

	/* Allocate one global heap for all threads */
	void *heapPtr = allocateHeap(256);

	k_queue inferenceQueue;
	k_queue_init(&inferenceQueue);

	/* inferenceSender tasks to create and queue the jobs */
	for (int n = 0; n < NUM_JOB_TASKS; n++) {
		const size_t stackSize = 2048;
		k_thread_stack_t *stack = static_cast<k_thread_stack_t *>(k_malloc(stackSize));
		if (stack == nullptr) {
			printk("Failed to allocate stack to 'inferenceSenderTask%i'\n", n);
			exit(1);
		}

		auto &thread = threads[nthreads];
		string *name = new string("sender " + to_string(n));

		thread.id = k_thread_create(&thread.thread, stack, stackSize, inferenceSenderTask,
					    name, heapPtr, &inferenceQueue, 3, 0, K_FOREVER);
		if (thread.id == 0) {
			printk("Failed to create 'inferenceSenderTask%i'\n", n);
			exit(1);
		}

		nthreads++;
	}

	/* Create inferenceProcess tasks to process the queued jobs */
	InferenceProcessParams taskParams[NUM_INFERENCE_TASKS];
	for (int n = 0; n < NUM_INFERENCE_TASKS; n++) {
		const size_t stackSize = 8192;
		k_thread_stack_t *stack = static_cast<k_thread_stack_t *>(k_malloc(stackSize));
		if (stack == nullptr) {
			printk("Failed to allocate stack to 'inferenceSenderTask%i'\n", n);
			exit(1);
		}

		auto &thread = threads[nthreads];
		auto &taskParam = taskParams[n];
		taskParam = InferenceProcessParams(&inferenceQueue, inferenceProcessTensorArena[n],
						   arenaSize);
		string *name = new string("runner " + to_string(n));

		thread.id = k_thread_create(&thread.thread, stack, stackSize, inferenceProcessTask,
					    name, heapPtr, &taskParam, 2, 0, K_FOREVER);
		if (thread.id == 0) {
			printk("Failed to create 'inferenceProcessTask%i'\n", n);
			exit(1);
		}

		nthreads++;
	}

	/* start Scheduler */
	for (size_t n = 0; n < nthreads; n++) {
		k_thread_start(threads[n].id);
	}

	/* put the task in the lowest priority */
	k_thread_priority_set(k_current_get(), 4);

	/* Safety belt */
	k_thread_suspend(k_current_get());

	printk("Zephyr application failed to initialise \n");

	return 1;
}
