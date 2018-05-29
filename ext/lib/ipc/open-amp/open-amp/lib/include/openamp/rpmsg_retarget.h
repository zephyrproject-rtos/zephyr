#include <openamp/open_amp.h>
#include <metal/mutex.h>
#include <metal/atomic.h>

#ifndef RPMSG_RETARGET_H
#define RPMSG_RETARGET_H

#if defined __cplusplus
extern "C" {
#endif

/* RPC response buffer size */
#define RPC_BUFF_SIZE 512

/* System call definitions */
#define OPEN_SYSCALL_ID     1
#define CLOSE_SYSCALL_ID    2
#define WRITE_SYSCALL_ID    3
#define READ_SYSCALL_ID     4
#define ACK_STATUS_ID       5
#define TERM_SYSCALL_ID     6

#define FILE_NAME_LEN       50

/* Proxy device endpoint ID */
#define PROXY_ENDPOINT      127

typedef void (*rpc_shutdown_cb) (struct rpmsg_channel *);

struct _rpc_data {
	struct rpmsg_channel *rpmsg_chnl;
	struct rpmsg_endpoint *rp_ept;
	metal_mutex_t rpc_lock;
	atomic_int sync;
	struct _sys_rpc *rpc;
	struct _sys_rpc *rpc_response;
	rpc_shutdown_cb shutdown_cb;
};

struct _sys_call_args {
	int int_field1;
	int int_field2;
	unsigned int data_len;
	char data[0];
};

/* System call rpc data structure */
struct _sys_rpc {
	unsigned int id;
	struct _sys_call_args sys_call_args;
};

/* API prototypes */
int rpmsg_retarget_init(struct rpmsg_channel *rp_chnl, rpc_shutdown_cb cb);
int rpmsg_retarget_deinit(struct rpmsg_channel *rp_chnl);
int rpmsg_retarget_send(void *data, int len);

#if defined __cplusplus
}
#endif

#endif /* RPMSG_RETARGET_H */
