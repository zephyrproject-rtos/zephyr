/* mailbox kernel services */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
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

#include <microkernel.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>

#include <micro_private.h>

#include <misc/__assert.h>
#include <misc/util.h>

/**
 *
 * @brief Determines if mailbox message is synchronous or asynchronous
 *
 * Returns a non-zero value if the specified message contains a valid pool ID,
 * indicating that it is an asynchronous message.
 */
#define ISASYNCMSG(message) ((message)->tx_block.pool_id != 0)

/**
 *
 * @brief Copy a packet
 *
 * @param in the packet to be copied
 * @param out the packet to copy to
 *
 * @return N/A
 */
static void copy_packet(struct k_args **out, struct k_args *in)
{
	GETARGS(*out);

	/*
	 * Copy the data from <in> to <*out> and create
	 * a backpointer to the original packet.
	 */

	memcpy(*out, in, sizeof(struct k_args));
	(*out)->Ctxt.args = in;
}

/**
 *
 * @brief Determine if there is a match between the mailbox sender and receiver
 *
 * @return matched message size, or -1 if no match
 */
static int match(struct k_args *Reader, struct k_args *Writer)
{
	if ((Reader->args.m1.mess.tx_task == ANYTASK ||
	     Reader->args.m1.mess.tx_task == Writer->args.m1.mess.tx_task) &&
	    (Writer->args.m1.mess.rx_task == ANYTASK ||
	     Writer->args.m1.mess.rx_task == Reader->args.m1.mess.rx_task)) {
		if (!ISASYNCMSG(&(Writer->args.m1.mess))) {
			int32_t info;

			Reader->args.m1.mess.tx_task =
				Writer->args.m1.mess.tx_task;

			Writer->args.m1.mess.rx_task =
				Reader->args.m1.mess.rx_task;

			info = Reader->args.m1.mess.info;
			Reader->args.m1.mess.info = Writer->args.m1.mess.info;
			Writer->args.m1.mess.info = info;
		} else {
			Reader->args.m1.mess.tx_task =
				Writer->args.m1.mess.tx_task;
			Reader->args.m1.mess.tx_data = NULL;
			Reader->args.m1.mess.tx_block =
				Writer->args.m1.mess.tx_block;
			Reader->args.m1.mess.info = Writer->args.m1.mess.info;
		}

		if (Reader->args.m1.mess.size > Writer->args.m1.mess.size) {
			Reader->args.m1.mess.size = Writer->args.m1.mess.size;
		} else {
			Writer->args.m1.mess.size = Reader->args.m1.mess.size;
		}

		/*
		 * The __ASSERT_NO_MSG() statements are used to verify that
		 * the -1 will not be returned when there is a match.
		 */

		__ASSERT_NO_MSG(Writer->args.m1.mess.size ==
			Reader->args.m1.mess.size);

		__ASSERT_NO_MSG((uint32_t)(-1) != Reader->args.m1.mess.size);

		return Reader->args.m1.mess.size;
	}

	return -1; /* There was no match */
}

/**
 * @brief Prepare transfer
 *
 * @return true or false
 */
static bool prepare_transfer(struct k_args *move,
					    struct k_args *reader,
					    struct k_args *writer)
{
	/* extract info from writer and reader before they change: */

	/*
	 * prepare writer and reader cmd packets for 'return':
	 * (this is shared code, irrespective of the value of 'move')
	 */
	__ASSERT_NO_MSG(NULL == reader->next);
	reader->Comm = _K_SVC_MBOX_RECEIVE_ACK;
	reader->Time.rcode = RC_OK;

	__ASSERT_NO_MSG(NULL == writer->next);
	writer->alloc = true;

	writer->Comm = _K_SVC_MBOX_SEND_ACK;
	writer->Time.rcode = RC_OK;

	if (move) {
		/* { move != NULL, which means full data exchange } */
		bool all_data_present = true;

		move->Comm = _K_SVC_MOVEDATA_REQ;
		/*
		 * transfer the data with the highest
		 * priority of reader and writer
		 */
		move->priority = max(writer->priority, reader->priority);
		move->Ctxt.task = NULL;
		move->args.moved_req.action =
			(MovedAction)(MVDACT_SNDACK | MVDACT_RCVACK);
		move->args.moved_req.total_size = writer->args.m1.mess.size;
		move->args.moved_req.extra.setup.continuation_send = NULL;
		move->args.moved_req.extra.setup.continuation_receive = NULL;

		/* reader: */
		if (reader->args.m1.mess.rx_data == NULL) {
			all_data_present = false;
			__ASSERT_NO_MSG(0 == reader->args.m1.mess.extra
					    .transfer); /* == extra.sema */
			reader->args.m1.mess.extra.transfer = move;
			/*SENDARGS(reader); */
		} else {
			move->args.moved_req.destination =
				reader->args.m1.mess.rx_data;
			writer->args.m1.mess.rx_data =
				reader->args.m1.mess.rx_data;

			/* chain the reader */
			move->args.moved_req.extra.setup.continuation_receive = reader;
		}

		/* writer: */
		if (ISASYNCMSG(&(writer->args.m1.mess))) {
			move->args.moved_req.source =
				writer->args.m1.mess.tx_block.pointer_to_data;
			reader->args.m1.mess.tx_block =
				writer->args.m1.mess.tx_block;
		} else {
			__ASSERT_NO_MSG(NULL != writer->args.m1.mess.tx_data);
			move->args.moved_req.source =
				writer->args.m1.mess.tx_data;
			reader->args.m1.mess.tx_data =
				writer->args.m1.mess.tx_data;
		}
		/* chain the writer */
		move->args.moved_req.extra.setup.continuation_send = writer;

		return all_data_present;
	}

	/* { NULL == move, which means header exchange only } */
	return 0; /* == don't care actually */
}

/**
 * @brief Do transfer
 *
 * @return N/A
 */
static void transfer(struct k_args *pMvdReq)
{
	__ASSERT_NO_MSG(NULL != pMvdReq->args.moved_req.source);
	__ASSERT_NO_MSG(NULL != pMvdReq->args.moved_req.destination);

	_k_movedata_request(pMvdReq);
	FREEARGS(pMvdReq);
}

/**
 * @brief Process the acknowledgment to a mailbox send request
 *
 * @return N/A
 */
void _k_mbox_send_ack(struct k_args *pCopyWriter)
{
	struct k_args *Starter;

	if (ISASYNCMSG(&(pCopyWriter->args.m1.mess))) {
		if (pCopyWriter->args.m1.mess.extra.sema) {
			/*
			 * Signal the semaphore.  Alternatively, this could
			 * be done using the continuation mechanism.
			 */

			struct k_args A;
#ifndef NO_KARG_CLEAR
			memset(&A, 0xfd, sizeof(struct k_args));
#endif
			A.Comm = _K_SVC_SEM_SIGNAL;
			A.args.s1.sema = pCopyWriter->args.m1.mess.extra.sema;
			_k_sem_signal(&A);
		}

		/*
		 * release the block from the memory pool
		 * unless this an asynchronous transfer.
		 */

		if ((uint32_t)(-1) !=
		    pCopyWriter->args.m1.mess.tx_block.pool_id) {
			/*
			 * special value to tell if block should be
			 * freed or not
			 */
			pCopyWriter->Comm = _K_SVC_MEM_POOL_BLOCK_RELEASE;
			pCopyWriter->args.p1.pool_id =
				pCopyWriter->args.m1.mess.tx_block.pool_id;
			pCopyWriter->args.p1.rep_poolptr =
				pCopyWriter->args.m1.mess.tx_block
					.address_in_pool;
			pCopyWriter->args.p1.rep_dataptr =
				pCopyWriter->args.m1.mess.tx_block
					.pointer_to_data;
			pCopyWriter->args.p1.req_size =
				pCopyWriter->args.m1.mess.tx_block.req_size;
			SENDARGS(pCopyWriter);
			return;
		}

		FREEARGS(pCopyWriter);
		return;
	}

	/*
	 * Get a pointer to the original command packet of the sender
	 * and copy both the result as well as the message information
	 * from the received packet of the sender before resetting the
	 * TF_SEND and TF_SENDDATA state bits.
	 */

	Starter = pCopyWriter->Ctxt.args;
	Starter->Time.rcode = pCopyWriter->Time.rcode;
	Starter->args.m1.mess = pCopyWriter->args.m1.mess;
	_k_state_bit_reset(Starter->Ctxt.task, TF_SEND | TF_SENDDATA);

	FREEARGS(pCopyWriter);
}

/**
 *
 * @brief Process the timeout for a mailbox send request
 *
 * @return N/A
 */
void _k_mbox_send_reply(struct k_args *pCopyWriter)
{
	FREETIMER(pCopyWriter->Time.timer);
	REMOVE_ELM(pCopyWriter);
	pCopyWriter->Time.rcode = RC_TIME;
	pCopyWriter->Comm = _K_SVC_MBOX_SEND_ACK;
	SENDARGS(pCopyWriter);
}

/**
 *
 * @brief Process a mailbox send request
 *
 * @return N/A
 */
void _k_mbox_send_request(struct k_args *Writer)
{
	kmbox_t MailBoxId = Writer->args.m1.mess.mailbox;
	struct _k_mbox_struct *MailBox;
	struct k_args *CopyReader;
	struct k_args *CopyWriter;
	struct k_args *temp;
	bool bAsync;

	bAsync = ISASYNCMSG(&Writer->args.m1.mess);

	struct k_task *sender = NULL;

	/*
	 * Only deschedule the task if it is not a poster
	 * (not an asynchronous request).
	 */

	if (!bAsync) {
		sender = _k_current_task;
		_k_state_bit_set(sender, TF_SEND);
	}

	Writer->Ctxt.task = sender;

	MailBox = (struct _k_mbox_struct *)MailBoxId;

	copy_packet(&CopyWriter, Writer);

	if (bAsync) {
		/*
		 * Clear the [Ctxt] field in an asynchronous request as the
		 * original packet will not be available later.
		 */

		CopyWriter->Ctxt.args = NULL;
	}

	/*
	 * The [next] field can be changed later when added to the Writer's
	 * list, but when not listed, [next] must be NULL.
	 */

	CopyWriter->next = NULL;

	for (CopyReader = MailBox->readers, temp = NULL; CopyReader != NULL;
	     temp = CopyReader, CopyReader = CopyReader->next) {
		uint32_t u32Size;

		u32Size = match(CopyReader, CopyWriter);

		if ((uint32_t)(-1) != u32Size) {
#ifdef CONFIG_OBJECT_MONITOR
			MailBox->count++;
#endif

			/*
			 * There is a match.  Remove the chosen reader from the
			 * list.
			 */

			if (temp != NULL) {
				temp->next = CopyReader->next;
			} else {
				MailBox->readers = CopyReader->next;
			}
			CopyReader->next = NULL;

#ifdef CONFIG_SYS_CLOCK_EXISTS
			if (CopyReader->Time.timer != NULL) {
				/*
				 * The reader was trying to handshake with
				 * timeout
				 */
				_k_timer_delist(CopyReader->Time.timer);
				FREETIMER(CopyReader->Time.timer);
			}
#endif

			if (0 == u32Size) {
				/* No data exchange--header only */
				prepare_transfer(NULL, CopyReader, CopyWriter);
				SENDARGS(CopyReader);
				SENDARGS(CopyWriter);
			} else {
				struct k_args *Moved_req;

				GETARGS(Moved_req);

				if (prepare_transfer(Moved_req,
						     CopyReader, CopyWriter)) {
					/*
					 * <Moved_req> will be cleared as well
					 */
					transfer(Moved_req);
				} else {
					SENDARGS(CopyReader);
				}
			}
			return;
		}
	}

	/* There is no matching receiver for this message. */

	if (bAsync) {
		/*
		 * For asynchronous requests, just post the message into the
		 * list and continue.  No further action is required.
		 */

		INSERT_ELM(MailBox->writers, CopyWriter);
		return;
	}

	if (CopyWriter->Time.ticks != TICKS_NONE) {
		/*
		 * The writer specified a wait or wait with timeout operation.
		 *
		 * Note: Setting the command to SEND_TMO is only necessary in
		 * the wait with timeout case.  However, it is more efficient
		 * to blindly set it rather than waste time on a comparison.
		 */

		CopyWriter->Comm = _K_SVC_MBOX_SEND_REPLY;

		/* Put the letter into the mailbox */
		INSERT_ELM(MailBox->writers, CopyWriter);

#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (CopyWriter->Time.ticks == TICKS_UNLIMITED) {
			/* This is a wait operation; there is no timer. */
			CopyWriter->Time.timer = NULL;
		} else {
			/*
			 * This is a wait with timeout operation.
			 * Enlist a new timeout.
			 */
			_k_timeout_alloc(CopyWriter);
		}
#endif
	} else {
		/*
		 * This is a no-wait operation.
		 * Notify the sender of failure.
		 */
		CopyWriter->Comm = _K_SVC_MBOX_SEND_ACK;
		CopyWriter->Time.rcode = RC_FAIL;
		SENDARGS(CopyWriter);
	}
}


/**
 * @brief Send a message to a mailbox
 *
 * This routine sends a message to a mailbox and looks for a matching receiver.
 *
 * @param mbox mailbox
 * @param prio priority of data transfer
 * @param M pointer to message to send
 * @param time maximum number of ticks to wait
 *
 * @return RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
 */
int _task_mbox_put(kmbox_t mbox,
	       kpriority_t prio,
	       struct k_msg *M,
	       int32_t time)
{
	struct k_args A;

	__ASSERT((0 == M->size) || (NULL != M->tx_data),
			 "Invalid mailbox data specification\n");

	if (unlikely((uint32_t)(-1) == M->size)) {
		/* the sender side cannot specify a size of -1 == 0xfff..ff */
		return RC_FAIL;
	}

	M->tx_task = _k_current_task->id;
	M->tx_block.pool_id = 0; /* NO ASYNC POST */
	M->extra.sema = 0;
	M->mailbox = mbox;

	A.priority = prio;
	A.Comm = _K_SVC_MBOX_SEND_REQUEST;
	A.Time.ticks = time;
	A.args.m1.mess = *M;

	KERNEL_ENTRY(&A);

	*M = A.args.m1.mess;
	return A.Time.rcode;
}

/**
 *
 * @brief Process a mailbox receive acknowledgment
 *
 * This routine processes a mailbox receive acknowledgment.
 *
 * INTERNAL:  This routine frees the <pCopyReader> packet
 *
 * @return N/A
 */
void _k_mbox_receive_ack(struct k_args *pCopyReader)
{
	struct k_args *Starter;

	/* Get a pointer to the original command packet of the sender */
	Starter = pCopyReader->Ctxt.args;

	/* Copy result from received packet */
	Starter->Time.rcode = pCopyReader->Time.rcode;

	/* And copy the message information from the received packet. */
	Starter->args.m1.mess = pCopyReader->args.m1.mess;

	/* Reschedule the sender task */
	_k_state_bit_reset(Starter->Ctxt.task, TF_RECV | TF_RECVDATA);

	FREEARGS(pCopyReader);
}

/**
 * @brief Process the timeout for a mailbox receive request
 *
 * @return N/A
 */
void _k_mbox_receive_reply(struct k_args *pCopyReader)
{
#ifdef CONFIG_SYS_CLOCK_EXISTS
	FREETIMER(pCopyReader->Time.timer);
	REMOVE_ELM(pCopyReader);
	pCopyReader->Time.rcode = RC_TIME;
	pCopyReader->Comm = _K_SVC_MBOX_RECEIVE_ACK;
	SENDARGS(pCopyReader);
#endif
}

/**
 * @brief Process a mailbox receive request
 *
 * @return N/A
 */
void _k_mbox_receive_request(struct k_args *Reader)
{
	kmbox_t MailBoxId = Reader->args.m1.mess.mailbox;
	struct _k_mbox_struct *MailBox;
	struct k_args *CopyWriter;
	struct k_args *temp;
	struct k_args *CopyReader;

	Reader->Ctxt.task = _k_current_task;
	_k_state_bit_set(Reader->Ctxt.task, TF_RECV);

	copy_packet(&CopyReader, Reader);

	/*
	 * The [next] field can be changed later when added to the Reader's
	 * list, but when not listed, [next] must be NULL.
	 */

	CopyReader->next = NULL;

	MailBox = (struct _k_mbox_struct *)MailBoxId;

	for (CopyWriter = MailBox->writers, temp = NULL; CopyWriter != NULL;
	     temp = CopyWriter, CopyWriter = CopyWriter->next) {
		uint32_t u32Size;

		u32Size = match(CopyReader, CopyWriter);

		if ((uint32_t)(-1) != u32Size) {
#ifdef CONFIG_OBJECT_MONITOR
			MailBox->count++;
#endif

			/*
			 * There is a match.  Remove the chosen reader
			 * from the list.
			 */

			if (temp != NULL) {
				temp->next = CopyWriter->next;
			} else {
				MailBox->writers = CopyWriter->next;
			}
			CopyWriter->next = NULL;

#ifdef CONFIG_SYS_CLOCK_EXISTS
			if (CopyWriter->Time.timer != NULL) {
				/*
				 * The writer was trying to handshake with
				 * timeout.
				 */
				_k_timer_delist(CopyWriter->Time.timer);
				FREETIMER(CopyWriter->Time.timer);
			}
#endif

			if (0 == u32Size) {
				/* No data exchange--header only */
				prepare_transfer(NULL, CopyReader, CopyWriter);
				SENDARGS(CopyReader);
				SENDARGS(CopyWriter);
			} else {
				struct k_args *Moved_req;

				GETARGS(Moved_req);

				if (prepare_transfer(Moved_req,
						     CopyReader, CopyWriter)) {
					/*
					 * <Moved_req> will be
					 * cleared as well
					 */
					transfer(Moved_req);
				} else {
					SENDARGS(CopyReader);
				}
			}
			return;
		}
	}

	/* There is no matching writer for this message. */

	if (Reader->Time.ticks != TICKS_NONE) {
		/*
		 * The writer specified a wait or wait with timeout operation.
		 *
		 * Note: Setting the command to RECV_TMO is only necessary in
		 * the wait with timeout case.  However, it is more efficient
		 * to blindly set it rather than waste time on a comparison.
		 */

		CopyReader->Comm = _K_SVC_MBOX_RECEIVE_REPLY;

		/* Put the letter into the mailbox */
		INSERT_ELM(MailBox->readers, CopyReader);

#ifdef CONFIG_SYS_CLOCK_EXISTS
		if (CopyReader->Time.ticks == TICKS_UNLIMITED) {
			/* This is a wait operation; there is no timer. */
			CopyReader->Time.timer = NULL;
		} else {
			/*
			 * This is a wait with timeout operation.
			 * Enlist a new timeout.
			 */
			_k_timeout_alloc(CopyReader);
		}
#endif
	} else {
		/*
		 * This is a no-wait operation.
		 * Notify the receiver of failure.
		 */
		CopyReader->Comm = _K_SVC_MBOX_RECEIVE_ACK;
		CopyReader->Time.rcode = RC_FAIL;
		SENDARGS(CopyReader);
	}
}


int _task_mbox_get(kmbox_t mbox,
	       struct k_msg *M,
	       int32_t time)
{
	struct k_args A;

	M->rx_task = _k_current_task->id;
	M->mailbox = mbox;
	M->extra.transfer = 0;

	/*
	 * NOTE: to make sure there is no conflict with extra.sema,
	 * there is an assertion check in prepare_transfer() if equal to 0
	 */

	A.priority = _k_current_task->priority;
	A.Comm = _K_SVC_MBOX_RECEIVE_REQUEST;
	A.Time.ticks = time;
	A.args.m1.mess = *M;

	KERNEL_ENTRY(&A);
	*M = A.args.m1.mess;
	return A.Time.rcode;
}


void _task_mbox_block_put(kmbox_t mbox,
		 kpriority_t prio,
		 struct k_msg *M,
		 ksem_t sema)
{
	struct k_args A;

	__ASSERT(0xFFFFFFFF != M->size, "Invalid mailbox data specification\n");

	if (0 == M->size) {
		/*
		 * trick: special value to indicate that tx_block
		 * should NOT be released in the SND_ACK
		 */
		M->tx_block.pool_id = (uint32_t)(-1);
	}

	M->tx_task = _k_current_task->id;
	M->tx_data = NULL;
	M->mailbox = mbox;
	M->extra.sema = sema;

#ifdef CONFIG_SYS_CLOCK_EXISTS
	A.Time.timer = NULL;
#endif
	A.priority = prio;
	A.Comm = _K_SVC_MBOX_SEND_REQUEST;
	A.args.m1.mess = *M;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Process a mailbox receive data request
 *
 * @return N/A
 */
void _k_mbox_receive_data(struct k_args *Starter)
{
	struct k_args *CopyStarter;
	struct k_args *MoveD;
	struct k_args *Writer;

	Starter->Ctxt.task = _k_current_task;
	_k_state_bit_set(_k_current_task, TF_RECVDATA);

	GETARGS(CopyStarter);
	memcpy(CopyStarter, Starter, sizeof(struct k_args));
	CopyStarter->Ctxt.args = Starter;

	MoveD = CopyStarter->args.m1.mess.extra.transfer;
	CopyStarter->Comm = _K_SVC_MBOX_RECEIVE_ACK;
	CopyStarter->Time.rcode = RC_OK;

	MoveD->args.moved_req.extra.setup.continuation_receive = CopyStarter;
	CopyStarter->next = NULL;
	MoveD->args.moved_req.destination = CopyStarter->args.m1.mess.rx_data;

	MoveD->args.moved_req.total_size = CopyStarter->args.m1.mess.size;

	Writer = MoveD->args.moved_req.extra.setup.continuation_send;
	if (Writer != NULL) {
		if (ISASYNCMSG(&(Writer->args.m1.mess))) {
			CopyStarter->args.m1.mess.tx_block =
				Writer->args.m1.mess.tx_block;
		} else {
			Writer->args.m1.mess.rx_data =
				CopyStarter->args.m1.mess.rx_data;
			CopyStarter->args.m1.mess.tx_data =
				Writer->args.m1.mess.tx_data;
		}
		transfer(MoveD); /* and MoveD will be cleared as well */
	}
}


void _task_mbox_data_get(struct k_msg *M)
{
	struct k_args A;

	/* sanity checks */
	if (unlikely(NULL == M->extra.transfer)) {
		/*
		 * protection: if a user erroneously calls this function after
		 * a task_mbox_get(), we should not run into trouble
		 */
		return;
	}

	A.args.m1.mess = *M;
	A.Comm = _K_SVC_MBOX_RECEIVE_DATA;

	KERNEL_ENTRY(&A);
}


int _task_mbox_data_block_get(struct k_msg *message,
			  struct k_block *rxblock,
			  kmemory_pool_t pool_id,
			  int32_t time)
{
	int retval;
	struct k_args *MoveD;

	/* sanity checks: */
	if (NULL == message->extra.transfer) {
		/*
		 * If a user erroneously calls this function after a
		 * task_mbox_get(), we should not run into trouble.
		 * Return RC_OK instead of RC_FAIL to be downwards compatible.
		 */

		return RC_OK;
	}

	/* special flow to check for possible optimisations: */

	if (ISASYNCMSG(message)) {
		/* First transfer block */
		__ASSERT_NO_MSG(-1 != message->tx_block.pool_id);
		*rxblock = message->tx_block;

		/* This is the MOVED packet */
		MoveD = message->extra.transfer;

		/* Then release sender (writer) */

		struct k_args *Writer;

		/*
		 * This is the first of the continuation packets for
		 * continuation on send.  It should be the only one.
		 * That is, it should not have any followers.  To
		 * prevent [tx_block] from being released when the
		 * SEND_ACK is processed, change its [pool_id] to -1.
		 */

		Writer = MoveD->args.moved_req.extra.setup.continuation_send;
		__ASSERT_NO_MSG(NULL != Writer);
		__ASSERT_NO_MSG(NULL == Writer->next);

		Writer->args.m1.mess.tx_block.pool_id = (uint32_t)(-1);
		nano_task_stack_push(&_k_command_stack, (uint32_t)Writer);

#ifdef ACTIV_ASSERTS
		struct k_args *dummy;

		/*
		 * Confirm that there are not any continuation packets
		 * for continuation on receive.
		 */

		dummy = MoveD->args.moved_req.extra.setup.continuation_receive;
		__ASSERT_NO_MSG(NULL == dummy);
#endif

		FREEARGS(MoveD); /* Clean up MOVED */

		return RC_OK;
	}

	/* 'normal' flow of task_mbox_data_block_get(): */

	if (0 != message->size) {
		retval = _task_mem_pool_alloc(rxblock, pool_id,
					message->size, time);
		if (retval != RC_OK) {
			return retval;
		}
		message->rx_data = rxblock->pointer_to_data;
	} else {
		rxblock->pool_id = (kmemory_pool_t) -1;
	}

	/*
	 * Invoke task_mbox_data_get() core without sanity checks, as they have
	 * already been performed.
	 */

	struct k_args A;

	A.args.m1.mess = *message;
	A.Comm = _K_SVC_MBOX_RECEIVE_DATA;
	KERNEL_ENTRY(&A);

	return RC_OK; /* task_mbox_data_get() doesn't return anything */
}

/**
 * @brief Process a mailbox send data request
 *
 * @return N/A
 */
void _k_mbox_send_data(struct k_args *Starter)
{
	struct k_args *CopyStarter;
	struct k_args *MoveD;
	struct k_args *Reader;

	Starter->Ctxt.task = _k_current_task;
	_k_state_bit_set(_k_current_task, TF_SENDDATA);

	GETARGS(CopyStarter);
	memcpy(CopyStarter, Starter, sizeof(struct k_args));
	CopyStarter->Ctxt.args = Starter;

	MoveD = CopyStarter->args.m1.mess.extra.transfer;

	CopyStarter->Time.rcode = RC_OK;
	CopyStarter->Comm = _K_SVC_MBOX_SEND_ACK;

	MoveD->args.moved_req.extra.setup.continuation_send = CopyStarter;
	CopyStarter->next = NULL;
	MoveD->args.moved_req.source = CopyStarter->args.m1.mess.rx_data;

	Reader = MoveD->args.moved_req.extra.setup.continuation_receive;
	if (Reader != NULL) {
		Reader->args.m1.mess.rx_data =
			CopyStarter->args.m1.mess.rx_data;
		CopyStarter->args.m1.mess.tx_data =
			Reader->args.m1.mess.tx_data;

		transfer(MoveD); /* and MoveD will be cleared as well */
	}
}
