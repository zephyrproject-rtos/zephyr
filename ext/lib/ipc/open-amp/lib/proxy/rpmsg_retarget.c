#include <openamp/open_amp.h>
#include <openamp/rpmsg_retarget.h>
#include <metal/alloc.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

/*************************************************************************
 *	Description
 *	This files contains rpmsg based redefinitions for C RTL system calls
 *	such as _open, _read, _write, _close.
 *************************************************************************/
static struct _rpc_data *rpc_data = 0;

int send_rpc(void *data, int len);

void rpc_cb(struct rpmsg_channel *rtl_rp_chnl, void *data, int len, void *priv,
	    unsigned long src)
{
	(void)priv;
	(void)src;

	memcpy(rpc_data->rpc_response, data, len);

	atomic_flag_clear(&rpc_data->sync);
	if (rpc_data->rpc_response->id == TERM_SYSCALL_ID) {
		/* Application terminate signal is received from the proxy app,
		 * so let the application know of terminate message.
		 */
		rpc_data->shutdown_cb(rtl_rp_chnl);
	}
}

int send_rpc(void *data, int len)
{
	int retval;

	retval = rpmsg_sendto(rpc_data->rpmsg_chnl, data, len, PROXY_ENDPOINT);
	return retval;
}

int rpmsg_retarget_init(struct rpmsg_channel *rp_chnl, rpc_shutdown_cb cb)
{
	/* Allocate memory for rpc control block */
	rpc_data = (struct _rpc_data *)metal_allocate_memory(sizeof(struct _rpc_data));

	/* Create a mutex for synchronization */
	metal_mutex_init(&rpc_data->rpc_lock);

	/* Create a mutex for synchronization */
	atomic_store(&rpc_data->sync, 1);

	/* Create a endpoint to handle rpc response from master */
	rpc_data->rpmsg_chnl = rp_chnl;
	rpc_data->rp_ept = rpmsg_create_ept(rpc_data->rpmsg_chnl, rpc_cb,
					    RPMSG_NULL, PROXY_ENDPOINT);
	rpc_data->rpc = metal_allocate_memory(RPC_BUFF_SIZE);
	rpc_data->rpc_response = metal_allocate_memory(RPC_BUFF_SIZE);
	rpc_data->shutdown_cb = cb;

	return 0;
}

int rpmsg_retarget_deinit(struct rpmsg_channel *rp_chnl)
{
	(void)rp_chnl;

	metal_free_memory(rpc_data->rpc);
	metal_free_memory(rpc_data->rpc_response);
	metal_mutex_deinit(&rpc_data->rpc_lock);
	rpmsg_destroy_ept(rpc_data->rp_ept);
	metal_free_memory(rpc_data);
	rpc_data = NULL;

	return 0;
}

int rpmsg_retarget_send(void *data, int len)
{
	return send_rpc(data, len);
}

static inline void rpmsg_retarget_wait(struct _rpc_data *rpc)
{
	struct hil_proc *proc = rpc->rpmsg_chnl->rdev->proc;
	while (atomic_flag_test_and_set(&rpc->sync)) {
		hil_poll(proc, 0);
	}
}

/*************************************************************************
 *
 *   FUNCTION
 *
 *       _open
 *
 *   DESCRIPTION
 *
 *       Open a file.  Minimal implementation
 *
 *************************************************************************/
int _open(const char *filename, int flags, int mode)
{
	int filename_len = strlen(filename) + 1;
	int payload_size = sizeof(struct _sys_rpc) + filename_len;
	int retval = -1;

	if ((!filename) || (filename_len > FILE_NAME_LEN)) {
		return -1;
	}

	if (!rpc_data)
		return retval;

	/* Construct rpc payload */
	rpc_data->rpc->id = OPEN_SYSCALL_ID;
	rpc_data->rpc->sys_call_args.int_field1 = flags;
	rpc_data->rpc->sys_call_args.int_field2 = mode;
	rpc_data->rpc->sys_call_args.data_len = filename_len;
	memcpy(&rpc_data->rpc->sys_call_args.data, filename, filename_len);

	/* Transmit rpc request */
	metal_mutex_acquire(&rpc_data->rpc_lock);
	send_rpc((void *)rpc_data->rpc, payload_size);
	metal_mutex_release(&rpc_data->rpc_lock);

	/* Wait for response from proxy on master */
	rpmsg_retarget_wait(rpc_data);

	/* Obtain return args and return to caller */
	if (rpc_data->rpc_response->id == OPEN_SYSCALL_ID) {
		retval = rpc_data->rpc_response->sys_call_args.int_field1;
	}

	return retval;
}

/*************************************************************************
 *
 *   FUNCTION
 *
 *       _read
 *
 *   DESCRIPTION
 *
 *       Low level function to redirect IO to serial.
 *
 *************************************************************************/
int _read(int fd, char *buffer, int buflen)
{
	int payload_size = sizeof(struct _sys_rpc);
	int retval = -1;

	if (!buffer || !buflen)
		return retval;
	if (!rpc_data)
		return retval;

	/* Construct rpc payload */
	rpc_data->rpc->id = READ_SYSCALL_ID;
	rpc_data->rpc->sys_call_args.int_field1 = fd;
	rpc_data->rpc->sys_call_args.int_field2 = buflen;
	rpc_data->rpc->sys_call_args.data_len = 0;	/*not used */

	/* Transmit rpc request */
	metal_mutex_acquire(&rpc_data->rpc_lock);
	send_rpc((void *)rpc_data->rpc, payload_size);
	metal_mutex_release(&rpc_data->rpc_lock);

	/* Wait for response from proxy on master */
	rpmsg_retarget_wait(rpc_data);

	/* Obtain return args and return to caller */
	if (rpc_data->rpc_response->id == READ_SYSCALL_ID) {
		if (rpc_data->rpc_response->sys_call_args.int_field1 > 0) {
			memcpy(buffer,
			       rpc_data->rpc_response->sys_call_args.data,
			       rpc_data->rpc_response->sys_call_args.data_len);
		}

		retval = rpc_data->rpc_response->sys_call_args.int_field1;
	}

	return retval;
}

/*************************************************************************
 *
 *   FUNCTION
 *
 *       _write
 *
 *   DESCRIPTION
 *
 *       Low level function to redirect IO to serial.
 *
 *************************************************************************/
int _write(int fd, const char *ptr, int len)
{
	int retval = -1;
	int payload_size = sizeof(struct _sys_rpc) + len;
	int null_term = 0;

	if (fd == 1) {
		null_term = 1;
	}
	if (!rpc_data)
		return retval;

	rpc_data->rpc->id = WRITE_SYSCALL_ID;
	rpc_data->rpc->sys_call_args.int_field1 = fd;
	rpc_data->rpc->sys_call_args.int_field2 = len;
	rpc_data->rpc->sys_call_args.data_len = len + null_term;
	memcpy(rpc_data->rpc->sys_call_args.data, ptr, len);
	if (null_term) {
		*(char *)(rpc_data->rpc->sys_call_args.data + len + null_term) =
		    0;
	}

	metal_mutex_acquire(&rpc_data->rpc_lock);
	send_rpc((void *)rpc_data->rpc, payload_size);
	metal_mutex_release(&rpc_data->rpc_lock);

	/* Wait for response from proxy on master */
	rpmsg_retarget_wait(rpc_data);

	if (rpc_data->rpc_response->id == WRITE_SYSCALL_ID) {
		retval = rpc_data->rpc_response->sys_call_args.int_field1;
	}

	return retval;

}

/*************************************************************************
 *
 *   FUNCTION
 *
 *       _close
 *
 *   DESCRIPTION
 *
 *       Close a file.  Minimal implementation
 *
 *************************************************************************/
int _close(int fd)
{
	int payload_size = sizeof(struct _sys_rpc);
	int retval = -1;

	if (!rpc_data)
		return retval;
	rpc_data->rpc->id = CLOSE_SYSCALL_ID;
	rpc_data->rpc->sys_call_args.int_field1 = fd;
	rpc_data->rpc->sys_call_args.int_field2 = 0;	/*not used */
	rpc_data->rpc->sys_call_args.data_len = 0;	/*not used */

	metal_mutex_acquire(&rpc_data->rpc_lock);
	send_rpc((void *)rpc_data->rpc, payload_size);
	metal_mutex_release(&rpc_data->rpc_lock);

	/* Wait for response from proxy on master */
	rpmsg_retarget_wait(rpc_data);

	if (rpc_data->rpc_response->id == CLOSE_SYSCALL_ID) {
		retval = rpc_data->rpc_response->sys_call_args.int_field1;
	}

	return retval;
}
