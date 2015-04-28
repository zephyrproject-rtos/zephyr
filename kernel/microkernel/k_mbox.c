/* mailbox kernel services */

/*
 * Copyright (c) 1997-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* includes */

#include <microkernel.h>
#include <string_s.h>
#include <toolchain.h>
#include <sections.h>

#include <minik.h>
#include <kmail.h>
#include <kmemcpy.h>
#include <kticks.h>
#include <ksema.h>

#include <misc/__assert.h>

/*******************************************************************************
*
* copy_packet - copy a packet
*
* RETURNS: N/A
*/

static void copy_packet(struct k_args **out, struct k_args *in)
{
	GETARGS(*out);

	/*
	 * Copy the data from <in> to <*out> and create
	 * a backpointer to the original packet.
	 */

	k_memcpy_s(*out, sizeof(struct k_args),
			   in, sizeof(struct k_args));
	(*out)->Ctxt.args = in;
}

/*******************************************************************************
*
* match - determine if there is a match between the mailbox sender and receiver
*
* RETURNS: matched message size, or -1 if no match
*/

static int match(struct k_args *Reader, struct k_args *Writer)
{
	if ((Reader->Args.m1.mess.tx_task == ANYTASK ||
	     Reader->Args.m1.mess.tx_task == Writer->Args.m1.mess.tx_task) &&
	    (Writer->Args.m1.mess.rx_task == ANYTASK ||
	     Writer->Args.m1.mess.rx_task == Reader->Args.m1.mess.rx_task)) {
		if (!ISASYNCMSG(&(Writer->Args.m1.mess))) {
			int32_t info;

			Reader->Args.m1.mess.tx_task =
				Writer->Args.m1.mess.tx_task;

			Writer->Args.m1.mess.rx_task =
				Reader->Args.m1.mess.rx_task;

			info = Reader->Args.m1.mess.info;
			Reader->Args.m1.mess.info = Writer->Args.m1.mess.info;
			Writer->Args.m1.mess.info = info;
		} else {
			Reader->Args.m1.mess.tx_task =
				Writer->Args.m1.mess.tx_task;
			Reader->Args.m1.mess.tx_data = NULL;
			Reader->Args.m1.mess.tx_block =
				Writer->Args.m1.mess.tx_block;
			Reader->Args.m1.mess.info = Writer->Args.m1.mess.info;
		}

		if (Reader->Args.m1.mess.size > Writer->Args.m1.mess.size) {
			Reader->Args.m1.mess.size = Writer->Args.m1.mess.size;
		} else {
			Writer->Args.m1.mess.size = Reader->Args.m1.mess.size;
		}

		/*
		 * The __ASSERT_NO_MSG() statements are used to verify that
		 * the -1 will not be returned when there is a match.
		 */

		__ASSERT_NO_MSG(Writer->Args.m1.mess.size ==
			Reader->Args.m1.mess.size);

		__ASSERT_NO_MSG((uint32_t)(-1) != Reader->Args.m1.mess.size);

		return Reader->Args.m1.mess.size;
	}

	return -1; /* There was no match */
}

/*******************************************************************************
*
* prepare_transfer -
*
* RETURNS: TRUE or FALSE
*/

static BOOL prepare_transfer(struct k_args *move,
					    struct k_args *reader,
					    struct k_args *writer)
{
	/* extract info from writer and reader before they change: */

	/*
	 * prepare writer and reader cmd packets for 'return':
	 * (this is shared code, irrespective of the value of 'move')
	 */
	__ASSERT_NO_MSG(NULL == reader->Forw);
	reader->Comm = RECV_ACK;
	reader->Time.rcode = RC_OK;

	__ASSERT_NO_MSG(NULL == writer->Forw);
	writer->alloc = true;

	writer->Comm = SEND_ACK;
	writer->Time.rcode = RC_OK;

	if (move) {
		/* { move != NULL, which means full data exchange } */

		BOOL all_data_present = TRUE;
		move->Comm = MVD_REQ;
		/*
		 * transfer the data with the highest
		 * priority of reader and writer
		 */
		move->Prio = max(writer->Prio, reader->Prio);
		move->Ctxt.proc = NULL;
		move->Args.MovedReq.Action =
			(MovedAction)(MVDACT_SNDACK | MVDACT_RCVACK);
		move->Args.MovedReq.iTotalSize = writer->Args.m1.mess.size;
		move->Args.MovedReq.Extra.Setup.ContSnd = NULL;
		move->Args.MovedReq.Extra.Setup.ContRcv = NULL;

		/* reader: */
		if (reader->Args.m1.mess.rx_data == NULL) {
			all_data_present = FALSE;
			__ASSERT_NO_MSG(0 == reader->Args.m1.mess.extra
					    .transfer); /* == extra.sema */
			reader->Args.m1.mess.extra.transfer = move;
			/*SENDARGS(reader); */
		} else {
			move->Args.MovedReq.destination =
				reader->Args.m1.mess.rx_data;
			writer->Args.m1.mess.rx_data =
				reader->Args.m1.mess.rx_data;

			/* chain the reader */
			move->Args.MovedReq.Extra.Setup.ContRcv = reader;
		}

		/* writer: */
		if (ISASYNCMSG(&(writer->Args.m1.mess))) {
			move->Args.MovedReq.source =
				writer->Args.m1.mess.tx_block.pointer_to_data;
			reader->Args.m1.mess.tx_block =
				writer->Args.m1.mess.tx_block;
		} else {
			__ASSERT_NO_MSG(NULL != writer->Args.m1.mess.tx_data);
			move->Args.MovedReq.source =
				writer->Args.m1.mess.tx_data;
			reader->Args.m1.mess.tx_data =
				writer->Args.m1.mess.tx_data;
		}
		/* chain the writer */
		move->Args.MovedReq.Extra.Setup.ContSnd = writer;

		return all_data_present;
	} else {
		/* { NULL == move, which means header exchange only } */
		return 0; /* == don't care actually */
	}
}

/*******************************************************************************
*
* transfer -
*
* RETURNS: N/A
*/

static void transfer(struct k_args *pMvdReq)
{
	__ASSERT_NO_MSG(NULL != pMvdReq->Args.MovedReq.source);
	__ASSERT_NO_MSG(NULL != pMvdReq->Args.MovedReq.destination);

	_k_movedata_request(pMvdReq);
	FREEARGS(pMvdReq);
}

/*******************************************************************************
*
* _k_mbox_send_ack - process the acknowledgement to a mailbox send request
*
* RETURNS: N/A
*/

void _k_mbox_send_ack(struct k_args *pCopyWriter)
{
	if (ISASYNCMSG(&(pCopyWriter->Args.m1.mess))) {
		if (pCopyWriter->Args.m1.mess.extra.sema) {
			/*
			 * Signal the semaphore.  Alternatively, this could
			 * be done using the continuation mechanism.
			 */

			struct k_args A;
#ifndef NO_KARG_CLEAR
			k_memset(&A, 0xfd, sizeof(struct k_args));
#endif
			A.Comm = SIGNALS;
			A.Args.s1.sema = pCopyWriter->Args.m1.mess.extra.sema;
			_k_sem_signal(&A);
		}

		/*
		 * release the block from the memory pool
		 * unless this an asynchronous transfer.
		 */

		if ((uint32_t)(-1) !=
		    pCopyWriter->Args.m1.mess.tx_block.poolid) {
			/*
			 * special value to tell if block should be
			 * freed or not
			 */
			pCopyWriter->Comm = REL_BLOCK;
			pCopyWriter->Args.p1.poolid =
				pCopyWriter->Args.m1.mess.tx_block.poolid;
			pCopyWriter->Args.p1.rep_poolptr =
				pCopyWriter->Args.m1.mess.tx_block
					.address_in_pool;
			pCopyWriter->Args.p1.rep_dataptr =
				pCopyWriter->Args.m1.mess.tx_block
					.pointer_to_data;
			pCopyWriter->Args.p1.req_size =
				pCopyWriter->Args.m1.mess.tx_block.req_size;
			SENDARGS(pCopyWriter);
			return;
		} else {
			FREEARGS(pCopyWriter);
			return;
		}
	} else {
		struct k_args *Starter;

		/*
		 * Get a pointer to the original command packet of the sender
		 * and copy both the result as well as the message information
		 * from the received packet of the sender before resetting the
		 * TF_SEND and TF_SENDDATA state bits.
		 */

		Starter = pCopyWriter->Ctxt.args;
		Starter->Time.rcode = pCopyWriter->Time.rcode;
		Starter->Args.m1.mess = pCopyWriter->Args.m1.mess;
		reset_state_bit(Starter->Ctxt.proc, TF_SEND | TF_SENDDATA);

		FREEARGS(pCopyWriter);
	}
}

/*******************************************************************************
*
* _k_mbox_send_reply - process the timeout for a mailbox send request
*
* RETURNS: N/A
*/

void _k_mbox_send_reply(struct k_args *pCopyWriter)
{
	FREETIMER(pCopyWriter->Time.timer);
	REMOVE_ELM(pCopyWriter);
	pCopyWriter->Time.rcode = RC_TIME;
	pCopyWriter->Comm = SEND_ACK;
	SENDARGS(pCopyWriter);
}

/*******************************************************************************
*
* _k_mbox_send_request - process a mailbox send request
*
* RETURNS: N/A
*/

void _k_mbox_send_request(struct k_args *Writer)
{
	kmbox_t MailBoxId = Writer->Args.m1.mess.mailbox;
	struct mbx_struct *MailBox;
	struct k_args *CopyReader;
	struct k_args *CopyWriter;
	struct k_args *temp;
	BOOL bAsync;

	bAsync = ISASYNCMSG(&Writer->Args.m1.mess);

	struct k_proc *sender = NULL;

	/*
	 * Only deschedule the task if it is not a poster
	 * (not an asynchronous request).
	 */

	if (!bAsync) {
		sender = _k_current_task;
		set_state_bit(sender, TF_SEND);
	}

	Writer->Ctxt.proc = sender;

	MailBox = _k_mbox_list + OBJ_INDEX(MailBoxId);

	copy_packet(&CopyWriter, Writer);

	if (bAsync) {
		/*
		 * Clear the [Ctxt] field in an asynchronous request as the
		 * original packet will not be available later.
		 */

		CopyWriter->Ctxt.args = NULL;
	}

	/*
	 * The [Forw] field can be changed later when added to the Writer's
	 * list, but when not listed, [Forw] must be NULL.
	 */

	CopyWriter->Forw = NULL;

	for (CopyReader = MailBox->Readers, temp = NULL; CopyReader != NULL;
	     temp = CopyReader, CopyReader = CopyReader->Forw) {
		uint32_t u32Size;

		u32Size = match(CopyReader, CopyWriter);

		if ((uint32_t)(-1) != u32Size) {
#ifdef CONFIG_OBJECT_MONITOR
			MailBox->Count++;
#endif

			/*
			 * There is a match.  Remove the chosen reader from the
			 * list.
			 */

			if (temp != NULL) {
				temp->Forw = CopyReader->Forw;
			} else {
				MailBox->Readers = CopyReader->Forw;
			}
			CopyReader->Forw = NULL;

			if (CopyReader->Time.timer != NULL) {
				/*
				 * The reader was trying to handshake with
				 * timeout
				 */
				delist_timer(CopyReader->Time.timer);
				FREETIMER(CopyReader->Time.timer);
			}

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

	/* There is no matching receiver for this message. */

	if (bAsync) {
		/*
		 * For asynchronous requests, just post the message into the
		 * list and continue.  No further action is required.
		 */

		INSERT_ELM(MailBox->Writers, CopyWriter);
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

		CopyWriter->Comm = SEND_TMO;

		/* Put the letter into the mailbox */
		INSERT_ELM(MailBox->Writers, CopyWriter);

		if (CopyWriter->Time.ticks == TICKS_UNLIMITED) {
			/* This is a wait operation; there is no timer. */
			CopyWriter->Time.timer = NULL;
		} else {
			/*
			 * This is a wait with timeout operation.
			 * Enlist a new timeout.
			 */
			enlist_timeout(CopyWriter);
		}
	} else {
		/*
		 * This is a no-wait operation.
		 * Notify the sender of failure.
		 */
		CopyWriter->Comm = SEND_ACK;
		CopyWriter->Time.rcode = RC_FAIL;
		SENDARGS(CopyWriter);
	}
}

/*******************************************************************************
*
* _task_mbox_put - send a message to a mailbox
*
* This routine sends a message to a mailbox and looks for a matching receiver.
*
* RETURNS: RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
*/

int _task_mbox_put(kmbox_t mbox, /* mailbox */
	       kpriority_t prio, /* priority of data transfer */
	       struct k_msg *M,    /* pointer to message to send */
	       int32_t time /* maximum number of ticks to wait */
	       )
{
	struct k_args A;

	__ASSERT((0 == M->size) || (NULL != M->tx_data),
			 "Invalid mailbox data specification\n");

	if (unlikely((uint32_t)(-1) == M->size)) {
		/* the sender side cannot specify a size of -1 == 0xfff..ff */
		return RC_FAIL;
	}

	M->tx_task = _k_current_task->Ident;
	M->tx_block.poolid = 0; /* NO ASYNC POST */
	M->extra.sema = 0;
	M->mailbox = mbox;

	A.Prio = prio;
	A.Comm = SEND_REQ;
	A.Time.ticks = time;
	A.Args.m1.mess = *M;

	KERNEL_ENTRY(&A);

	*M = A.Args.m1.mess;
	return A.Time.rcode;
}

/*******************************************************************************
*
* _k_mbox_receive_ack - process a mailbox receive acknowledgement
*
* This routine processes a mailbox receive acknowledgement.
*
* INTERNAL:  This routine frees the <pCopyReader> packet
*
* RETURNS: N/A
*/

void _k_mbox_receive_ack(struct k_args *pCopyReader)
{
	struct k_args *Starter;

	/* Get a pointer to the original command packet of the sender */
	Starter = pCopyReader->Ctxt.args;

	/* Copy result from received packet */
	Starter->Time.rcode = pCopyReader->Time.rcode;

	/* And copy the message information from the received packet. */
	Starter->Args.m1.mess = pCopyReader->Args.m1.mess;

	/* Reschedule the sender task */
	reset_state_bit(Starter->Ctxt.proc, TF_RECV | TF_RECVDATA);

	FREEARGS(pCopyReader);
}

/*******************************************************************************
*
* _k_mbox_receive_reply - process the timeout for a mailbox receive request
*
* RETURNS: N/A
*/

void _k_mbox_receive_reply(struct k_args *pCopyReader)
{
	FREETIMER(pCopyReader->Time.timer);
	REMOVE_ELM(pCopyReader);
	pCopyReader->Time.rcode = RC_TIME;
	pCopyReader->Comm = RECV_ACK;
	SENDARGS(pCopyReader);
}

/*******************************************************************************
*
* _k_mbox_receive_request - process a mailbox receive request
*
* RETURNS: N/A
*/

void _k_mbox_receive_request(struct k_args *Reader)
{
	kmbox_t MailBoxId = Reader->Args.m1.mess.mailbox;
	struct mbx_struct *MailBox;
	struct k_args *CopyWriter;
	struct k_args *temp;
	struct k_args *CopyReader;

	Reader->Ctxt.proc = _k_current_task;
	set_state_bit(Reader->Ctxt.proc, TF_RECV);

	copy_packet(&CopyReader, Reader);

	/*
	 * The [Forw] field can be changed later when added to the Reader's
	 * list, but when not listed, [Forw] must be NULL.
	 */

	CopyReader->Forw = NULL;

	MailBox = _k_mbox_list + OBJ_INDEX(MailBoxId);

	for (CopyWriter = MailBox->Writers, temp = NULL; CopyWriter != NULL;
	     temp = CopyWriter, CopyWriter = CopyWriter->Forw) {
		uint32_t u32Size;

		u32Size = match(CopyReader, CopyWriter);

		if ((uint32_t)(-1) != u32Size) {
#ifdef CONFIG_OBJECT_MONITOR
			MailBox->Count++;
#endif

			/*
			 * There is a match.  Remove the chosen reader
			 * from the list.
			 */

			if (temp != NULL) {
				temp->Forw = CopyWriter->Forw;
			} else {
				MailBox->Writers = CopyWriter->Forw;
			}
			CopyWriter->Forw = NULL;

			if (CopyWriter->Time.timer != NULL) {
				/*
				 * The writer was trying to handshake with
				 * timeout.
				 */
				delist_timer(CopyWriter->Time.timer);
				FREETIMER(CopyWriter->Time.timer);
			}

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

		CopyReader->Comm = RECV_TMO;

		/* Put the letter into the mailbox */
		INSERT_ELM(MailBox->Readers, CopyReader);

		if (CopyReader->Time.ticks == TICKS_UNLIMITED) {
			/* This is a wait operation; there is no timer. */
			CopyReader->Time.timer = NULL;
		} else {
			/*
			 * This is a wait with timeout operation.
			 * Enlist a new timeout.
			 */
			enlist_timeout(CopyReader);
		}
	} else {
		/*
		 * This is a no-wait operation.
		 * Notify the receiver of failure.
		 */
		CopyReader->Comm = RECV_ACK;
		CopyReader->Time.rcode = RC_FAIL;
		SENDARGS(CopyReader);
	}
}

/*******************************************************************************
*
* _task_mbox_get - gets struct k_msg message header structure information
*                  from a mailbox
*
* RETURNS: RC_OK, RC_FAIL, RC_TIME on success, failure, timeout respectively
*/

int _task_mbox_get(kmbox_t mbox, /* mailbox */
	       struct k_msg *M,    /* pointer to message */
	       int32_t time /* maximum number of ticks to wait */
	       )
{
	struct k_args A;

	M->rx_task = _k_current_task->Ident;
	M->mailbox = mbox;
	M->extra.transfer = 0;

	/*
	 * NOTE: to make sure there is no conflict with extra.sema,
	 * there is an assertion check in prepare_transfer() if equal to 0
	 */

	A.Prio = _k_current_task->Prio;
	A.Comm = RECV_REQ;
	A.Time.ticks = time;
	A.Args.m1.mess = *M;

	KERNEL_ENTRY(&A);
	*M = A.Args.m1.mess;
	return A.Time.rcode;
}

/*******************************************************************************
*
* _task_mbox_put_async - send a message asynchronously to a mailbox
*
* This routine sends a message to a mailbox and does not wait for a matching
* receiver. There is no exchange header returned to the sender. When the data
* has been transferred to the receiver, the semaphore signaling is performed.
*
* RETURNS: N/A
*/

void _task_mbox_put_async(kmbox_t mbox, /* mailbox to which to send message */
		 kpriority_t prio, /* priority of data transfer */
		 struct k_msg *M,    /* pointer to message to send */
		 ksem_t sema /* semaphore to signal when transfer is complete */
		 )
{
	struct k_args A;

	__ASSERT(0xFFFFFFFF != M->size, "Invalid mailbox data specification\n");

	if (0 == M->size) {
		/*
		 * trick: special value to indicate that tx_block
		 * should NOT be released in the SND_ACK
		 */
		M->tx_block.poolid = (uint32_t)(-1);
	}

	M->tx_task = _k_current_task->Ident;
	M->tx_data = NULL;
	M->mailbox = mbox;
	M->extra.sema = sema;

	A.Time.timer = NULL;
	A.Prio = prio;
	A.Comm = SEND_REQ;
	A.Args.m1.mess = *M;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* _k_mbox_receive_data - process a mailbox receive data request
*
* RETURNS: N/A
*/

void _k_mbox_receive_data(struct k_args *Starter)
{
	struct k_args *CopyStarter;
	struct k_args *MoveD;
	struct k_args *Writer;

	Starter->Ctxt.proc = _k_current_task;
	set_state_bit(_k_current_task, TF_RECVDATA);

	GETARGS(CopyStarter);
	k_memcpy_s(CopyStarter, sizeof(struct k_args),
			Starter, sizeof(struct k_args));
	CopyStarter->Ctxt.args = Starter;

	MoveD = CopyStarter->Args.m1.mess.extra.transfer;
	CopyStarter->Comm = RECV_ACK;
	CopyStarter->Time.rcode = RC_OK;

	MoveD->Args.MovedReq.Extra.Setup.ContRcv = CopyStarter;
	CopyStarter->Forw = NULL;
	MoveD->Args.MovedReq.destination = CopyStarter->Args.m1.mess.rx_data;

	MoveD->Args.MovedReq.iTotalSize = CopyStarter->Args.m1.mess.size;

	Writer = MoveD->Args.MovedReq.Extra.Setup.ContSnd;
	if (Writer != NULL) {
		if (ISASYNCMSG(&(Writer->Args.m1.mess))) {
			CopyStarter->Args.m1.mess.tx_block =
				Writer->Args.m1.mess.tx_block;
		} else {
			Writer->Args.m1.mess.rx_data =
				CopyStarter->Args.m1.mess.rx_data;
			CopyStarter->Args.m1.mess.tx_data =
				Writer->Args.m1.mess.tx_data;
		}
		transfer(MoveD); /* and MoveD will be cleared as well */
	}
}

/*******************************************************************************
*
* _task_mbox_data_get - get message data
*
* This routine is called for either of the two following purposes:
* 1. To transfer data if the call to task_mbox_get() resulted in a non-zero size
*    field in the struct k_msg header structure.
* 2. To wake up and release a transmitting task that is blocked on a call to
*    task_mbox_put[wait|wait_timeout]().
*
* RETURNS: N/A
*/

void _task_mbox_data_get(struct k_msg *M /* message from which to get data */
		    )
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

	A.Args.m1.mess = *M;
	A.Comm = RECV_DATA;

	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* _task_mbox_data_get_async_block - get the mailbox data and place
*                                   in a memory pool block
*
* RETURNS: RC_OK upon success, RC_FAIL upon failure, or RC_TIME upon timeout
*/

int _task_mbox_data_get_async_block(struct k_msg *message,
			  struct k_block *rxblock,
			  kmemory_pool_t poolid,
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
		__ASSERT_NO_MSG(-1 != message->tx_block.poolid);
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
		 * SEND_ACK is processed, change its [poolid] to -1.
		 */

		Writer = MoveD->Args.MovedReq.Extra.Setup.ContSnd;
		__ASSERT_NO_MSG(NULL != Writer);
		__ASSERT_NO_MSG(NULL == Writer->Forw);

		Writer->Args.m1.mess.tx_block.poolid = (uint32_t)(-1);
		nano_task_stack_push(&_k_command_stack, (uint32_t)Writer);

#ifdef ACTIV_ASSERTS
		struct k_args *Dummy;

		/*
		 * Confirm that there are not any continuation packets
		 * for continuation on receive.
		 */

		Dummy = MoveD->Args.MovedReq.Extra.Setup.ContRcv;
		__ASSERT_NO_MSG(NULL == Dummy);
#endif

		FREEARGS(MoveD); /* Clean up MOVED */

		return RC_OK;
	}

	/* 'normal' flow of task_mbox_data_get_async_block(): */

	if (0 != message->size) {
		retval = _task_mem_pool_alloc(rxblock, poolid,
					message->size, time);
		if (retval != RC_OK) {
			return retval;
		}
		message->rx_data = rxblock->pointer_to_data;
	} else {
		rxblock->poolid = (kmemory_pool_t) -1;
	}

	/*
	 * Invoke task_mbox_data_get() core without sanity checks, as they have
	 * already been performed.
	 */

	struct k_args A;
	A.Args.m1.mess = *message;
	A.Comm = RECV_DATA;
	KERNEL_ENTRY(&A);

	return RC_OK; /* task_mbox_data_get() doesn't return anything */
}

/*******************************************************************************
*
* _k_mbox_send_data - process a mailbox send data request
*
* RETURNS: N/A
*/

void _k_mbox_send_data(struct k_args *Starter)
{
	struct k_args *CopyStarter;
	struct k_args *MoveD;
	struct k_args *Reader;

	Starter->Ctxt.proc = _k_current_task;
	set_state_bit(_k_current_task, TF_SENDDATA);

	GETARGS(CopyStarter);
	k_memcpy_s(CopyStarter, sizeof(struct k_args),
		Starter, sizeof(struct k_args));
	CopyStarter->Ctxt.args = Starter;

	MoveD = CopyStarter->Args.m1.mess.extra.transfer;

	CopyStarter->Time.rcode = RC_OK;
	CopyStarter->Comm = SEND_ACK;

	MoveD->Args.MovedReq.Extra.Setup.ContSnd = CopyStarter;
	CopyStarter->Forw = NULL;
	MoveD->Args.MovedReq.source = CopyStarter->Args.m1.mess.rx_data;

	Reader = MoveD->Args.MovedReq.Extra.Setup.ContRcv;
	if (Reader != NULL) {
		Reader->Args.m1.mess.rx_data =
			CopyStarter->Args.m1.mess.rx_data;
		CopyStarter->Args.m1.mess.tx_data =
			Reader->Args.m1.mess.tx_data;

		transfer(MoveD); /* and MoveD will be cleared as well */
	}
}
