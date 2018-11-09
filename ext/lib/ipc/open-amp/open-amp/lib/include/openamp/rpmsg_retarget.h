#ifndef RPMSG_RETARGET_H
#define RPMSG_RETARGET_H

#include <metal/mutex.h>
#include <openamp/open_amp.h>
#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

/* File Operations System call definitions */
#define OPEN_SYSCALL_ID  0x1UL
#define CLOSE_SYSCALL_ID 0x2UL
#define WRITE_SYSCALL_ID 0x3UL
#define READ_SYSCALL_ID  0x4UL
#define ACK_STATUS_ID    0x5UL

#define TERM_SYSCALL_ID  0x6UL

#define DEFAULT_PROXY_ENDPOINT  0xFFUL

struct rpmsg_rpc_data;

typedef int (*rpmsg_rpc_poll)(void *arg);
typedef void (*rpmsg_rpc_shutdown_cb)(struct rpmsg_rpc_data *rpc);

struct rpmsg_rpc_syscall_header {
	int32_t int_field1;
	int32_t int_field2;
	uint32_t data_len;
};

struct rpmsg_rpc_syscall {
	uint32_t id;
	struct rpmsg_rpc_syscall_header args;
};

struct rpmsg_rpc_data {
	struct rpmsg_endpoint ept;
	int ept_destroyed;
	atomic_int nacked;
	void *respbuf;
	size_t respbuf_len;
	rpmsg_rpc_poll poll;
	void *poll_arg;
	rpmsg_rpc_shutdown_cb shutdown_cb;
	metal_mutex_t lock;
	struct metal_spinlock buflock;
};

/**
 * rpmsg_rpc_init - initialize RPMsg remote procedure call
 *
 * This function is to intialize the remote procedure call
 * global data. RPMsg RPC will send request to remote and
 * wait for callback.
 *
 * @rpc: pointer to the global remote procedure call data
 * @rdev: pointer to the rpmsg device
 * @ept_name: name of the endpoint used by RPC
 * @ept_addr: address of the endpoint used by RPC
 * @ept_raddr: remote address of the endpoint used by RPC
 * @poll_arg: pointer to poll function argument
 * @poll: poll function
 * @shutdown_cb: shutdown callback function
 *
 * return 0 for success, and negative value for failure.
 */
int rpmsg_rpc_init(struct rpmsg_rpc_data *rpc,
		   struct rpmsg_device *rdev,
		   const char *ept_name, uint32_t ept_addr,
		   uint32_t ept_raddr,
		   void *poll_arg, rpmsg_rpc_poll poll,
		   rpmsg_rpc_shutdown_cb shutdown_cb);

/**
 * rpmsg_rpc_release - release RPMsg remote procedure call
 *
 * This function is to release remoteproc procedure call
 * global data.
 *
 * @rpc: pointer to the globacl remote procedure call
 */
void rpmsg_rpc_release(struct rpmsg_rpc_data *rpc);

/**
 * rpmsg_rpc_send - Request RPMsg RPC call
 *
 * This function sends RPC request it will return with the length
 * of data and the response buffer.
 *
 * @rpc: pointer to remoteproc procedure call data struct
 * @req: pointer to request buffer
 * @len: length of the request data
 * @resp: pointer to where store the response buffer
 * @resp_len: length of the response buffer
 *
 * return length of the received response, negative value for failure.
 */
int rpmsg_rpc_send(struct rpmsg_rpc_data *rpc,
		   void *req, size_t len,
		   void *resp, size_t resp_len);

/**
 * rpmsg_set_default_rpc - set default RPMsg RPC data
 *
 * The default RPC data is used to redirect standard C file operations
 * to RPMsg channels.
 *
 * @rpc: pointer to remoteproc procedure call data struct
 */
void rpmsg_set_default_rpc(struct rpmsg_rpc_data *rpc);

#if defined __cplusplus
}
#endif

#endif /* RPMSG_RETARGET_H */
