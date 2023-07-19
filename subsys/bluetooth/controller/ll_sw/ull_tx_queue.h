/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ull_tx_q {
	uint8_t pause_data; /* Data pause state of the tx queue */

	sys_slist_t tx_list; /* Data and control node_tx list */
	sys_slist_t data_list; /* Data node_tx wait list */
};

/* Forward declaration of node_tx */
struct node_tx;

/**
 * @brief Initialize a tx queue.
 *
 * @param ull_tx_q Address of tx queue.
 */
void ull_tx_q_init(struct ull_tx_q *queue);

/**
 * @brief Pause the data path of a tx queue.
 *
 * @param ull_tx_q Address of tx queue.
 */
void ull_tx_q_pause_data(struct ull_tx_q *queue);

/**
 * @brief Resume the data path of a tx queue
 *
 * @param ull_tx_q Address of tx queue.
 */
void ull_tx_q_resume_data(struct ull_tx_q *queue);

/**
 * @brief Enqueue a tx node in the data path of a tx queue
 *
 * @param ull_tx_q Address of tx queue.
 * @param tx Address of tx node to enqueue.
 */
void ull_tx_q_enqueue_data(struct ull_tx_q *queue, struct node_tx *tx);

/**
 * @brief Enqueue a tx node in the control path of a tx queue
 *
 * @param ull_tx_q Address of tx queue.
 * @param tx Address of tx node to enqueue.
 */
void ull_tx_q_enqueue_ctrl(struct ull_tx_q *queue, struct node_tx *tx);

/**
 * @brief Peek head tx node of tx queue.
 *
 * @param ull_tx_q Address of tx queue.
 *
 * @return Head tx node of the tx queue.
 */
struct node_tx *ull_tx_q_peek(struct ull_tx_q *queue);

/**
 * @brief Dequeue a tx node from a tx queue.
 *
 * @param ull_tx_q Address of tx queue.
 *
 * @return Head tx node of the tx queue.
 */
struct node_tx *ull_tx_q_dequeue(struct ull_tx_q *queue);
