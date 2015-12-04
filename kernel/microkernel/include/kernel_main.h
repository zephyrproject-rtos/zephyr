/*
 * Copyright (c) 1997-2012, 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Non-public microkernel APIs referenced by kernel_main.c.
 */

#ifndef _ENTRIES_H
#define _ENTRIES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Format of a task's entry routine and (optional) abort routine */

typedef void (*taskstartfunction)(void);
typedef void (*taskabortfunction)(void);

/* APIs referenced by generated data structures */

extern int _k_ticker(int event);

/* APIs referenced by generated routines */

extern void _k_pipe_init(void);
extern void _k_mem_map_init(void);
extern void _k_mem_pool_init(void);

/* Format of routines invoked by microkernel server */

typedef void (*kernelfunc)(struct k_args *);

/* Jumptable entrypoints */

extern void _k_nop(struct k_args *);
extern void _k_offload_to_fiber(struct k_args *);
extern void _k_workload_get(struct k_args *);

/**
 *
 * @brief Handle semaphore signal request
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_signal(struct k_args *);

/**
 *
 * @brief Handle signal semaphore group request
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_group_signal(struct k_args *);

/**
 *
 * @brief Handle semaphore reset request
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_reset(struct k_args *);

/**
 *
 * @brief Handle semaphore group reset request
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_group_reset(struct k_args *);

/**
 *
 * @brief Handle internal wait request on a semaphore involved in a
 *              semaphore group wait request
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_wait_request(struct k_args *);

/**
 *
 * @brief Reply to a semaphore wait request
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_wait_reply(struct k_args *);

/**
 *
 * @brief Reply to a semaphore wait request with timeout.
 *
 * @param A Pointer to a k_args structure.
 *
 * @return N/A
 *
 * @sa _k_sem_wait_reply
 */
extern void _k_sem_wait_reply_timeout(struct k_args *A);

/**
 *
 * @brief Handle semaphore group wait request
 *
 * This routine splits the single semaphore group wait request into several
 * internal wait requests--one for each semaphore in the group.
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_group_wait_any(struct k_args *);

/**
 *
 * @brief Handle semaphore test and wait request
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_group_wait_request(struct k_args *);

/**
 *
 * @brief Handle semaphore ready request
 *
 * This routine only applies to semaphore group wait requests.  It identifies
 * the one semaphore in the group that "won" the semaphore group wait request
 * before triggering the semaphore group timeout handler.
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_group_ready(struct k_args *);

/**
 *
 * @brief Handle cancellation of a semaphore involved in a
 *              semaphore group wait request
 *
 * This routine only applies to semaphore group wait requests.  It is invoked
 * for each semaphore in the semaphore group that "lost" the semaphore group
 * wait request.
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_group_wait_cancel(struct k_args *);

/**
 *
 * @brief Handle acceptance of the ready semaphore request
 *
 * This routine only applies to semaphore group wait requests.  It handles
 * the request for the one semaphore in the group that "wins" the semaphore
 * group wait request.
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */

extern void _k_sem_group_wait_accept(struct k_args *);

/**
 *
 * @brief Finish handling incomplete waits on semaphores
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_group_wait(struct k_args *);

/**
 *
 * @brief Handle semaphore group timeout request
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_group_wait_timeout(struct k_args *);

/**
 *
 * @brief Handle semaphore inquiry request
 *
 * @param k_args Pointer to a k_args structure.
 *
 * @return N/A
 */
extern void _k_sem_inquiry(struct k_args *);

extern void _k_mutex_lock_request(struct k_args *);
extern void _k_mutex_lock_reply(struct k_args *);
extern void _k_mutex_lock_reply_timeout(struct k_args *);
extern void _k_mutex_unlock(struct k_args *);
extern void _k_fifo_enque_request(struct k_args *);
extern void _k_fifo_enque_reply(struct k_args *);
extern void _k_fifo_enque_reply_timeout(struct k_args *);
extern void _k_fifo_deque_request(struct k_args *);
extern void _k_fifo_deque_reply(struct k_args *);
extern void _k_fifo_deque_reply_timeout(struct k_args *);
extern void _k_fifo_ioctl(struct k_args *);
extern void _k_mbox_send_request(struct k_args *);
extern void _k_mbox_send_reply(struct k_args *);
extern void _k_mbox_send_ack(struct k_args *);
extern void _k_mbox_send_data(struct k_args *);
extern void _k_mbox_receive_request(struct k_args *);
extern void _k_mbox_receive_reply(struct k_args *);
extern void _k_mbox_receive_ack(struct k_args *);
extern void _k_mbox_receive_data(struct k_args *);
extern void _k_task_sleep(struct k_args *);
extern void _k_task_wakeup(struct k_args *);
extern void _k_task_op(struct k_args *);
extern void _k_task_group_op(struct k_args *);
extern void _k_task_priority_set(struct k_args *);
extern void _k_task_yield(struct k_args *);
extern void _k_mem_map_alloc(struct k_args *);
extern void _k_mem_map_dealloc(struct k_args *);
extern void _k_timer_alloc(struct k_args *);
extern void _k_timer_dealloc(struct k_args *);
extern void _k_timer_start(struct k_args *);
extern void _k_timer_stop(struct k_args *);
extern void _k_mem_map_alloc_timeout(struct k_args *);
extern void _k_event_test(struct k_args *);
extern void _k_event_handler_set(struct k_args *);
extern void _k_event_signal(struct k_args *);
extern void _k_mem_pool_block_get(struct k_args *);
extern void _k_mem_pool_block_release(struct k_args *);
extern void _k_block_waiters_get(struct k_args *);
extern void _k_mem_pool_block_get_timeout_handle(struct k_args *);
extern void _k_defrag(struct k_args *);

extern void _k_movedata_request(struct k_args *Req);
extern void K_mvdsndreq(struct k_args *SndReq);
extern void K_mvdrcvreq(struct k_args *RcvReq);
extern void K_rawdata(struct k_args *DataPacket);
extern void K_mvdsndack(struct k_args *SndDAck);
extern void K_mvdrcvack(struct k_args *RcvDAck);

extern void _k_pipe_put_request(struct k_args *Writer);
extern void _k_pipe_put_timeout(struct k_args *Writer);
extern void _k_pipe_put_reply(struct k_args *Writer);
extern void _k_pipe_put_ack(struct k_args *Writer);
extern void _k_pipe_get_request(struct k_args *Reader);
extern void _k_pipe_get_timeout(struct k_args *Reader);
extern void _k_pipe_get_reply(struct k_args *Reader);
extern void _k_pipe_get_ack(struct k_args *Reader);
extern void _k_pipe_movedata_ack(struct k_args *pEOXfer);
extern void _k_event_test_timeout(struct k_args *A);

#ifdef __cplusplus
}
#endif

#endif /* _ENTRIES_H */
