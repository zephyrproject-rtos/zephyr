
#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include "compute_graph.h"

#define NUM_HARTS CONFIG_MP_MAX_NUM_CPUS
#define TASK_INTERVAL_MS 2000  // Time interval for task generation
#define WORK_QUEUE_SIZE 10      // Maximum tasks per worker queue

K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, NUM_HARTS, 1024);
K_THREAD_STACK_DEFINE(producer_stack, 1024);

struct k_thread worker_threads[NUM_HARTS];
struct k_thread producer_thread;
struct k_mutex print_mutex;

// Message queues for worker threads
struct k_msgq task_queues[NUM_HARTS];

// Statically allocated task buffer (each worker has a buffer of `WORK_QUEUE_SIZE` tasks)
struct k_work task_buffers[NUM_HARTS][WORK_QUEUE_SIZE];

// Message queue storage buffers
struct k_work *task_msg_buffers[NUM_HARTS][WORK_QUEUE_SIZE];

// Task function that prints the executing hart
void print_task(struct k_work *work) {
    int hart_id = arch_curr_cpu()->id;
    k_mutex_lock(&print_mutex, K_FOREVER);
    printf("Task executed on hart %d\n", hart_id);
    k_mutex_unlock(&print_mutex);
}

// Worker function that explicitly fetches and processes work
void worker_func(void *arg1, void *arg2, void *arg3) {
    int hart_id = (int)(uintptr_t)arg1;
    struct k_work *task;

    k_mutex_lock(&print_mutex, K_FOREVER);
    printf("Worker thread started on hart %d\n", hart_id);
    k_mutex_unlock(&print_mutex);

    while (1) {
        // Wait for a task in the queue
        if (k_msgq_get(&task_queues[hart_id], &task, K_FOREVER) == 0) {
            // Execute the task
            task->handler(task);
        }
    }
}

// Producer thread that assigns tasks periodically
void producer_func(void *arg1, void *arg2, void *arg3) {
    static int task_index[NUM_HARTS] = {0};  // Track which task to use per hart

    while (1) {
		k_mutex_lock(&print_mutex, K_FOREVER);
		printf("Producer thread generating tasks\n");
		k_mutex_unlock(&print_mutex);
		for(int j = 0; j < 4; j++) {
			for (int i = 0; i < NUM_HARTS; i++) {
				struct k_work *task = &task_buffers[i][task_index[i]];
				k_work_init(task, print_task);

				// Submit task to worker queue
				if (k_msgq_put(&task_queues[i], &task, K_NO_WAIT) != 0) {
					printf("Worker %d task queue full, skipping task\n", i);
				}

				// Round-robin through preallocated tasks
				task_index[i] = (task_index[i] + 1) % WORK_QUEUE_SIZE;
			}
		}

        k_sleep(K_MSEC(TASK_INTERVAL_MS));
    }
}

int main(void) {
    printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
    k_mutex_init(&print_mutex);

    k_mutex_lock(&print_mutex, K_FOREVER);
    printf("Starting SMP worker threads on %d harts\n", NUM_HARTS);
    k_mutex_unlock(&print_mutex);

    // Initialize worker threads and message queues
    for (int i = 0; i < NUM_HARTS; i++) {
        // Initialize message queue for each worker
        k_msgq_init(&task_queues[i], (char *)task_msg_buffers[i], sizeof(struct k_work *), WORK_QUEUE_SIZE);

        // Start worker thread
        k_tid_t tid = k_thread_create(
            &worker_threads[i],
            worker_stacks[i],
            K_THREAD_STACK_SIZEOF(worker_stacks[i]),
            worker_func,
            (void *)(uintptr_t)i, NULL, NULL,
            K_PRIO_PREEMPT(1), 0, K_FOREVER
        );

        // Pin thread to specific hart
        k_thread_cpu_mask_clear(tid);
        k_thread_cpu_mask_enable(tid, i);
        k_thread_start(tid);
    }

    // Create producer thread (not pinned, higher priority)
    k_thread_create(
        &producer_thread,
        producer_stack,
        K_THREAD_STACK_SIZEOF(producer_stack),
        producer_func,
        NULL, NULL, NULL,
        K_PRIO_PREEMPT(0), 0, K_NO_WAIT
    );

    return 0;
}

