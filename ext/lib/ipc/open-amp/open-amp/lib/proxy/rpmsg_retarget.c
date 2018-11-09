#include <metal/mutex.h>
#include <metal/spinlock.h>
#include <metal/utilities.h>
#include <openamp/open_amp.h>
#include <openamp/rpmsg_retarget.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

/*************************************************************************
 *	Description
 *	This files contains rpmsg based redefinitions for C RTL system calls
 *	such as _open, _read, _write, _close.
 *************************************************************************/
static struct rpmsg_rpc_data *rpmsg_default_rpc;

static int rpmsg_rpc_ept_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
			    uint32_t src, void *priv)
{
	struct rpmsg_rpc_syscall *syscall;

	(void)priv;
	(void)src;

	if (data != NULL && ept != NULL) {
		syscall = data;
		if (syscall->id == TERM_SYSCALL_ID) {
			rpmsg_destroy_ept(ept);
		} else {
			struct rpmsg_rpc_data *rpc;

			rpc = metal_container_of(ept,
						 struct rpmsg_rpc_data,
						 ept);
			metal_spinlock_acquire(&rpc->buflock);
			if (rpc->respbuf != NULL && rpc->respbuf_len != 0) {
				if (len > rpc->respbuf_len)
					len = rpc->respbuf_len;
				memcpy(rpc->respbuf, data, len);
			}
			atomic_flag_clear(&rpc->nacked);
			metal_spinlock_release(&rpc->buflock);
		}
	}

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ept)
{
	struct rpmsg_rpc_data *rpc;

	rpc = metal_container_of(ept, struct rpmsg_rpc_data, ept);
	rpc->ept_destroyed = 1;
	rpmsg_destroy_ept(ept);
	atomic_flag_clear(&rpc->nacked);
	if (rpc->shutdown_cb)
		rpc->shutdown_cb(rpc);
}


int rpmsg_rpc_init(struct rpmsg_rpc_data *rpc,
		   struct rpmsg_device *rdev,
		   const char *ept_name, uint32_t ept_addr,
		   uint32_t ept_raddr,
		   void *poll_arg, rpmsg_rpc_poll poll,
		   rpmsg_rpc_shutdown_cb shutdown_cb)
{
	int ret;

	if (rpc == NULL || rdev == NULL)
		return -EINVAL;
	metal_spinlock_init(&rpc->buflock);
	metal_mutex_init(&rpc->lock);
	rpc->shutdown_cb = shutdown_cb;
	rpc->poll_arg = poll_arg;
	rpc->poll = poll;
	rpc->ept_destroyed = 0;
	rpc->respbuf = NULL;
	rpc->respbuf_len = 0;
	atomic_init(&rpc->nacked, 1);
	ret = rpmsg_create_ept(&rpc->ept, rdev,
			       ept_name, ept_addr, ept_raddr,
			       rpmsg_rpc_ept_cb, rpmsg_service_unbind);
	if (ret != 0) {
		metal_mutex_release(&rpc->lock);
		return -EINVAL;
	}
	while (!is_rpmsg_ept_ready(&rpc->ept)) {
		if (rpc->poll)
			rpc->poll(rpc->poll_arg);
	}
	return 0;
}

void rpmsg_rpc_release(struct rpmsg_rpc_data *rpc)
{
	if (rpc == NULL)
		return;
	if (rpc->ept_destroyed == 0)
		rpmsg_destroy_ept(&rpc->ept);
	metal_mutex_acquire(&rpc->lock);
	metal_spinlock_acquire(&rpc->buflock);
	rpc->respbuf = NULL;
	rpc->respbuf_len = 0;
	metal_spinlock_release(&rpc->buflock);
	metal_mutex_release(&rpc->lock);
	metal_mutex_deinit(&rpc->lock);

	return;
}

int rpmsg_rpc_send(struct rpmsg_rpc_data *rpc,
		   void *req, size_t len,
		   void *resp, size_t resp_len)
{
	int ret;

	if (rpc == NULL)
		return -EINVAL;
	metal_spinlock_acquire(&rpc->buflock);
	rpc->respbuf = resp;
	rpc->respbuf_len = resp_len;
	metal_spinlock_release(&rpc->buflock);
	(void)atomic_flag_test_and_set(&rpc->nacked);
	ret = rpmsg_send(&rpc->ept, req, len);
	if (ret < 0)
		return -EINVAL;
	if (!resp)
		return ret;
	while((atomic_flag_test_and_set(&rpc->nacked))) {
		if (rpc->poll)
			rpc->poll(rpc->poll_arg);
	}
	return ret;
}

void rpmsg_set_default_rpc(struct rpmsg_rpc_data *rpc)
{
	if (rpc == NULL)
		return;
	rpmsg_default_rpc = rpc;
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
#define MAX_BUF_LEN 496UL

int _open(const char *filename, int flags, int mode)
{
	struct rpmsg_rpc_data *rpc = rpmsg_default_rpc;
	struct rpmsg_rpc_syscall *syscall;
	struct rpmsg_rpc_syscall resp;
	int filename_len = strlen(filename) + 1;
	int payload_size = sizeof(*syscall) + filename_len;
	unsigned char tmpbuf[MAX_BUF_LEN];
	int ret;

	if (filename == NULL || payload_size > (int)MAX_BUF_LEN) {
		return -EINVAL;
	}

	if (rpc == NULL)
		return -EINVAL;

	/* Construct rpc payload */
	syscall = (struct rpmsg_rpc_syscall *)tmpbuf;
	syscall->id = OPEN_SYSCALL_ID;
	syscall->args.int_field1 = flags;
	syscall->args.int_field2 = mode;
	syscall->args.data_len = filename_len;
	memcpy(tmpbuf + sizeof(*syscall), filename, filename_len);

	resp.id = 0;
	ret = rpmsg_rpc_send(rpc, tmpbuf, payload_size,
			     (void *)&resp, sizeof(resp));
	if (ret >= 0) {
		/* Obtain return args and return to caller */
		if (resp.id == OPEN_SYSCALL_ID)
			ret = resp.args.int_field1;
		else
			ret = -EINVAL;
	}

	return ret;
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
	struct rpmsg_rpc_syscall syscall;
	struct rpmsg_rpc_syscall *resp;
	struct rpmsg_rpc_data *rpc = rpmsg_default_rpc;
	int payload_size = sizeof(syscall);
	unsigned char tmpbuf[MAX_BUF_LEN];
	int ret;

	if (rpc == NULL || buffer == NULL || buflen == 0)
		return -EINVAL;

	/* Construct rpc payload */
	syscall.id = READ_SYSCALL_ID;
	syscall.args.int_field1 = fd;
	syscall.args.int_field2 = buflen;
	syscall.args.data_len = 0;	/*not used */

	resp = (struct rpmsg_rpc_syscall *)tmpbuf;
	resp->id = 0;
	ret = rpmsg_rpc_send(rpc, (void *)&syscall, payload_size,
			     tmpbuf, sizeof(tmpbuf));

	/* Obtain return args and return to caller */
	if (ret >= 0) {
		if (resp->id == READ_SYSCALL_ID) {
			if (resp->args.int_field1 > 0) {
				int tmplen = resp->args.data_len;
				unsigned char *tmpptr = tmpbuf;

				tmpptr += sizeof(*resp);
				if (tmplen > buflen)
					tmplen = buflen;
				memcpy(buffer, tmpptr, tmplen);
			}
			ret = resp->args.int_field1;
		} else {
			ret = -EINVAL;
		}
	}

	return ret;
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
	int ret;
	struct rpmsg_rpc_syscall *syscall;
	struct rpmsg_rpc_syscall resp;
	int payload_size = sizeof(*syscall) + len;
	struct rpmsg_rpc_data *rpc = rpmsg_default_rpc;
	unsigned char tmpbuf[MAX_BUF_LEN];
	unsigned char *tmpptr;
	int null_term = 0;

	if (rpc == NULL)
		return -EINVAL;
	if (fd == 1)
		null_term = 1;

	syscall = (struct rpmsg_rpc_syscall *)tmpbuf;
	syscall->id = WRITE_SYSCALL_ID;
	syscall->args.int_field1 = fd;
	syscall->args.int_field2 = len;
	syscall->args.data_len = len + null_term;
	tmpptr = tmpbuf + sizeof(*syscall);
	memcpy(tmpptr, ptr, len);
	if (null_term == 1) {
		*(char *)(tmpptr + len + null_term) = 0;
		payload_size += 1;
	}
	resp.id = 0;
	ret = rpmsg_rpc_send(rpc, tmpbuf, payload_size,
			     (void *)&resp, sizeof(resp));

	if (ret >= 0) {
		if (resp.id == WRITE_SYSCALL_ID)
			ret = resp.args.int_field1;
		else
			ret = -EINVAL;
	}

	return ret;

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
	int ret;
	struct rpmsg_rpc_syscall syscall;
	struct rpmsg_rpc_syscall resp;
	int payload_size = sizeof(syscall);
	struct rpmsg_rpc_data *rpc = rpmsg_default_rpc;

	if (rpc == NULL)
		return -EINVAL;
	syscall.id = CLOSE_SYSCALL_ID;
	syscall.args.int_field1 = fd;
	syscall.args.int_field2 = 0;	/*not used */
	syscall.args.data_len = 0;	/*not used */

	resp.id = 0;
	ret = rpmsg_rpc_send(rpc, (void*)&syscall, payload_size,
			     (void*)&resp, sizeof(resp));

	if (ret >= 0) {
		if (resp.id == CLOSE_SYSCALL_ID)
			ret = resp.args.int_field1;
		else
			ret = -EINVAL;
	}

	return ret;
}
