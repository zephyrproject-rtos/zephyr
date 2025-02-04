/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <ctf_top.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket_poll.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/debug/cpu_load.h>

static void _get_thread_name(struct k_thread *thread,
			     ctf_bounded_string_t *name)
{
	const char *tname = k_thread_name_get(thread);

	if (tname != NULL && tname[0] != '\0') {
		strncpy(name->buf, tname, sizeof(name->buf));
		/* strncpy may not always null-terminate */
		name->buf[sizeof(name->buf) - 1] = 0;
	}
}

void sys_trace_k_thread_switched_out(void)
{
	ctf_bounded_string_t name = { "unknown" };
	struct k_thread *thread;

	thread = k_sched_current_thread_query();
	_get_thread_name(thread, &name);

	ctf_top_thread_switched_out((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_user_mode_enter(void)
{
	struct k_thread *thread;
	ctf_bounded_string_t name = { "unknown" };

	thread = k_sched_current_thread_query();
	_get_thread_name(thread, &name);
	ctf_top_thread_user_mode_enter((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_wakeup(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_wakeup((uint32_t)(uintptr_t)thread, name);
}


void sys_trace_k_thread_switched_in(void)
{
	struct k_thread *thread;
	ctf_bounded_string_t name = { "unknown" };

	thread = k_sched_current_thread_query();
	_get_thread_name(thread, &name);

	ctf_top_thread_switched_in((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_priority_set(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_priority_set((uint32_t)(uintptr_t)thread,
				    thread->base.prio, name);
}

void sys_trace_k_thread_create(struct k_thread *thread, size_t stack_size, int prio)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_create(
		(uint32_t)(uintptr_t)thread,
		thread->base.prio,
		name
		);

#if defined(CONFIG_THREAD_STACK_INFO)
	ctf_top_thread_info(
		(uint32_t)(uintptr_t)thread,
		name,
		thread->stack_info.start,
		thread->stack_info.size
		);
#endif
}

void sys_trace_k_thread_abort(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_abort((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_suspend(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_suspend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_resume(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);

	ctf_top_thread_resume((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_ready(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);

	ctf_top_thread_ready((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_start(struct k_thread *thread)
{

}

void sys_trace_k_thread_pend(struct k_thread *thread)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_pend((uint32_t)(uintptr_t)thread, name);
}

void sys_trace_k_thread_info(struct k_thread *thread)
{
#if defined(CONFIG_THREAD_STACK_INFO)
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_info(
		(uint32_t)(uintptr_t)thread,
		name,
		thread->stack_info.start,
		thread->stack_info.size
		);
#endif
}

void sys_trace_k_thread_name_set(struct k_thread *thread, int ret)
{
	ctf_bounded_string_t name = { "unknown" };

	_get_thread_name(thread, &name);
	ctf_top_thread_name_set(
		(uint32_t)(uintptr_t)thread,
		name
		);

}

void sys_trace_isr_enter(void)
{
	ctf_top_isr_enter();
}

void sys_trace_isr_exit(void)
{
	ctf_top_isr_exit();
}

void sys_trace_isr_exit_to_scheduler(void)
{
	ctf_top_isr_exit_to_scheduler();
}

void sys_trace_idle(void)
{
	ctf_top_idle();
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_enter_idle();
	}
}

void sys_trace_idle_exit(void)
{
	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		cpu_load_on_exit_idle();
	}
}

/* Semaphore */
void sys_trace_k_sem_init(struct k_sem *sem, int ret)
{
	ctf_top_semaphore_init(
		(uint32_t)(uintptr_t)sem,
		(int32_t)ret
		);
}

void sys_trace_k_sem_take_enter(struct k_sem *sem, k_timeout_t timeout)
{
	ctf_top_semaphore_take_enter(
		(uint32_t)(uintptr_t)sem,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}


void sys_trace_k_sem_take_blocking(struct k_sem *sem, k_timeout_t timeout)
{
	ctf_top_semaphore_take_blocking(
		(uint32_t)(uintptr_t)sem,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}

void sys_trace_k_sem_take_exit(struct k_sem *sem, k_timeout_t timeout, int ret)
{
	ctf_top_semaphore_take_exit(
		(uint32_t)(uintptr_t)sem,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks),
		(uint32_t)ret
		);
}

void sys_trace_k_sem_reset(struct k_sem *sem)
{
	ctf_top_semaphore_reset(
		(uint32_t)(uintptr_t)sem
		);
}

void sys_trace_k_sem_give_enter(struct k_sem *sem)
{
	ctf_top_semaphore_give_enter(
		(uint32_t)(uintptr_t)sem
		);
}

void sys_trace_k_sem_give_exit(struct k_sem *sem)
{
	ctf_top_semaphore_give_exit(
		(uint32_t)(uintptr_t)sem
		);
}

/* Mutex */
void sys_trace_k_mutex_init(struct k_mutex *mutex, int ret)
{
	ctf_top_mutex_init(
		(uint32_t)(uintptr_t)mutex,
		(int32_t)ret
		);
}

void sys_trace_k_mutex_lock_enter(struct k_mutex *mutex, k_timeout_t timeout)
{
	ctf_top_mutex_lock_enter(
		(uint32_t)(uintptr_t)mutex,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}

void sys_trace_k_mutex_lock_blocking(struct k_mutex *mutex, k_timeout_t timeout)
{
	ctf_top_mutex_lock_blocking(
		(uint32_t)(uintptr_t)mutex,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}

void sys_trace_k_mutex_lock_exit(struct k_mutex *mutex, k_timeout_t timeout, int ret)
{
	ctf_top_mutex_lock_exit(
		(uint32_t)(uintptr_t)mutex,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks),
		(int32_t)ret
		);
}

void sys_trace_k_mutex_unlock_enter(struct k_mutex *mutex)
{
	ctf_top_mutex_unlock_enter(
		(uint32_t)(uintptr_t)mutex
		);
}

void sys_trace_k_mutex_unlock_exit(struct k_mutex *mutex, int ret)
{
	ctf_top_mutex_unlock_exit(
		(uint32_t)(uintptr_t)mutex,
		(int32_t)ret
		);
}

/* Timer */
void sys_trace_k_timer_init(struct k_timer *timer)
{
	ctf_top_timer_init(
		(uint32_t)(uintptr_t)timer);
}

void sys_trace_k_timer_start(struct k_timer *timer, k_timeout_t duration,
			     k_timeout_t period)
{
	ctf_top_timer_start(
		(uint32_t)(uintptr_t)timer,
		k_ticks_to_us_floor32((uint32_t)duration.ticks),
		k_ticks_to_us_floor32((uint32_t)period.ticks)
		);
}

void sys_trace_k_timer_stop(struct k_timer *timer)
{
	ctf_top_timer_stop(
		(uint32_t)(uintptr_t)timer
		);
}

void sys_trace_k_timer_status_sync_enter(struct k_timer *timer)
{
	ctf_top_timer_status_sync_enter(
		(uint32_t)(uintptr_t)timer
		);
}

void sys_trace_k_timer_status_sync_blocking(struct k_timer *timer, k_timeout_t timeout)
{
	ctf_top_timer_status_sync_blocking(
		(uint32_t)(uintptr_t)timer,
		k_ticks_to_us_floor32((uint32_t)timeout.ticks)
		);
}

void sys_trace_k_timer_status_sync_exit(struct k_timer *timer, uint32_t result)
{
	ctf_top_timer_status_sync_exit(
		(uint32_t)(uintptr_t)timer,
		result
		);
}

/* Network socket */
void sys_trace_socket_init(int sock, int family, int type, int proto)
{
	ctf_top_socket_init(sock, family, type, proto);
}

void sys_trace_socket_close_enter(int sock)
{
	ctf_top_socket_close_enter(sock);
}

void sys_trace_socket_close_exit(int sock, int ret)
{
	ctf_top_socket_close_exit(sock, ret);
}

void sys_trace_socket_shutdown_enter(int sock, int how)
{
	ctf_top_socket_shutdown_enter(sock, how);
}

void sys_trace_socket_shutdown_exit(int sock, int ret)
{
	ctf_top_socket_shutdown_exit(sock, ret);
}

void sys_trace_socket_bind_enter(int sock, const struct sockaddr *addr, size_t addrlen)
{
	ctf_net_bounded_string_t addr_str;

	(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr,
			    addr_str.buf, sizeof(addr_str.buf));

	ctf_top_socket_bind_enter(sock, addr_str, addrlen, ntohs(net_sin(addr)->sin_port));
}

void sys_trace_socket_bind_exit(int sock, int ret)
{
	ctf_top_socket_bind_exit(sock, ret);
}

void sys_trace_socket_connect_enter(int sock, const struct sockaddr *addr, size_t addrlen)
{
	ctf_net_bounded_string_t addr_str;

	(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr,
			    addr_str.buf, sizeof(addr_str.buf));

	ctf_top_socket_connect_enter(sock, addr_str, addrlen);
}

void sys_trace_socket_connect_exit(int sock, int ret)
{
	ctf_top_socket_connect_exit(sock, ret);
}

void sys_trace_socket_listen_enter(int sock, int backlog)
{
	ctf_top_socket_listen_enter(sock, backlog);
}

void sys_trace_socket_listen_exit(int sock, int ret)
{
	ctf_top_socket_listen_exit(sock, ret);
}

void sys_trace_socket_accept_enter(int sock)
{
	ctf_top_socket_accept_enter(sock);
}

void sys_trace_socket_accept_exit(int sock, const struct sockaddr *addr,
				  const size_t *addrlen, int ret)
{
	ctf_net_bounded_string_t addr_str = { "unknown" };
	uint32_t addr_len = 0U;
	uint16_t port = 0U;

	if (addr != NULL) {
		(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr,
				    addr_str.buf, sizeof(addr_str.buf));
		port = net_sin(addr)->sin_port;
	}

	if (addrlen != NULL) {
		addr_len = *addrlen;
	}

	ctf_top_socket_accept_exit(sock, addr_str, addr_len, port, ret);
}

void sys_trace_socket_sendto_enter(int sock, int len, int flags,
				   const struct sockaddr *dest_addr, size_t addrlen)
{
	ctf_net_bounded_string_t addr_str = { "unknown" };

	if (dest_addr != NULL) {
		(void)net_addr_ntop(dest_addr->sa_family, &net_sin(dest_addr)->sin_addr,
				    addr_str.buf, sizeof(addr_str.buf));
	}

	ctf_top_socket_sendto_enter(sock, len, flags, addr_str, addrlen);
}

void sys_trace_socket_sendto_exit(int sock, int ret)
{
	ctf_top_socket_sendto_exit(sock, ret);
}

void sys_trace_socket_sendmsg_enter(int sock, const struct msghdr *msg, int flags)
{
	ctf_net_bounded_string_t addr = { "unknown" };
	uint32_t len = 0;

	for (int i = 0; msg->msg_iov != NULL && i < msg->msg_iovlen; i++) {
		len += msg->msg_iov[i].iov_len;
	}

	if (msg->msg_name != NULL) {
		(void)net_addr_ntop(((struct sockaddr *)msg->msg_name)->sa_family,
				    &net_sin((struct sockaddr *)msg->msg_name)->sin_addr,
				    addr.buf, sizeof(addr.buf));
	}

	ctf_top_socket_sendmsg_enter(sock, flags, (uint32_t)(uintptr_t)msg, addr, len);
}

void sys_trace_socket_sendmsg_exit(int sock, int ret)
{
	ctf_top_socket_sendmsg_exit(sock, ret);
}

void sys_trace_socket_recvfrom_enter(int sock, int max_len, int flags,
				     struct sockaddr *addr, size_t *addrlen)
{
	ctf_top_socket_recvfrom_enter(sock, max_len, flags,
				      (uint32_t)(uintptr_t)addr,
				      (uint32_t)(uintptr_t)addrlen);
}

void sys_trace_socket_recvfrom_exit(int sock, const struct sockaddr *src_addr,
				    const size_t *addrlen, int ret)
{
	ctf_net_bounded_string_t addr_str = { "unknown" };
	int len = 0;

	if (src_addr != NULL) {
		(void)net_addr_ntop(src_addr->sa_family, &net_sin(src_addr)->sin_addr,
				    addr_str.buf, sizeof(addr_str.buf));
	}

	if (addrlen != NULL) {
		len = *addrlen;
	}

	ctf_top_socket_recvfrom_exit(sock, addr_str, len, ret);
}

void sys_trace_socket_recvmsg_enter(int sock, const struct msghdr *msg, int flags)
{
	uint32_t max_len = 0;

	for (int i = 0; msg->msg_iov != NULL && i < msg->msg_iovlen; i++) {
		max_len += msg->msg_iov[i].iov_len;
	}

	ctf_top_socket_recvmsg_enter(sock, (uint32_t)(uintptr_t)msg, max_len, flags);
}

void sys_trace_socket_recvmsg_exit(int sock, const struct msghdr *msg, int ret)
{
	uint32_t len = 0;
	ctf_net_bounded_string_t addr = { "unknown" };

	for (int i = 0; msg->msg_iov != NULL && i < msg->msg_iovlen; i++) {
		len += msg->msg_iov[i].iov_len;
	}

	if (msg->msg_name != NULL) {
		(void)net_addr_ntop(((struct sockaddr *)msg->msg_name)->sa_family,
				    &net_sin((struct sockaddr *)msg->msg_name)->sin_addr,
				    addr.buf, sizeof(addr.buf));
	}

	ctf_top_socket_recvmsg_exit(sock, len, addr, ret);
}

void sys_trace_socket_fcntl_enter(int sock, int cmd, int flags)
{
	ctf_top_socket_fcntl_enter(sock, cmd, flags);
}

void sys_trace_socket_fcntl_exit(int sock, int ret)
{
	ctf_top_socket_fcntl_exit(sock, ret);
}

void sys_trace_socket_ioctl_enter(int sock, int req)
{
	ctf_top_socket_ioctl_enter(sock, req);
}

void sys_trace_socket_ioctl_exit(int sock, int ret)
{
	ctf_top_socket_ioctl_exit(sock, ret);
}

void sys_trace_socket_poll_value(int fd, int events)
{
	ctf_top_socket_poll_value(fd, events);
}

void sys_trace_socket_poll_enter(const struct zsock_pollfd *fds, int nfds, int timeout)
{
	ctf_top_socket_poll_enter((uint32_t)(uintptr_t)fds, nfds, timeout);

	for (int i = 0; i < nfds; i++) {
		sys_trace_socket_poll_value(fds[i].fd, fds[i].events);
	}
}

void sys_trace_socket_poll_exit(const struct zsock_pollfd *fds, int nfds, int ret)
{
	ctf_top_socket_poll_exit((uint32_t)(uintptr_t)fds, nfds, ret);

	for (int i = 0; i < nfds; i++) {
		sys_trace_socket_poll_value(fds[i].fd, fds[i].revents);
	}
}

void sys_trace_socket_getsockopt_enter(int sock, int level, int optname)
{
	ctf_top_socket_getsockopt_enter(sock, level, optname);
}

void sys_trace_socket_getsockopt_exit(int sock, int level, int optname,
				      void *optval, size_t optlen, int ret)
{
	ctf_top_socket_getsockopt_exit(sock, level, optname,
				       (uint32_t)(uintptr_t)optval, optlen, ret);
}

void sys_trace_socket_setsockopt_enter(int sock, int level, int optname,
				       const void *optval, size_t optlen)
{
	ctf_top_socket_setsockopt_enter(sock, level, optname,
					(uint32_t)(uintptr_t)optval, optlen);
}

void sys_trace_socket_setsockopt_exit(int sock, int ret)
{
	ctf_top_socket_setsockopt_exit(sock, ret);
}

void sys_trace_socket_getpeername_enter(int sock)
{
	ctf_top_socket_getpeername_enter(sock);
}

void sys_trace_socket_getpeername_exit(int sock,  struct sockaddr *addr,
				       const size_t *addrlen, int ret)
{
	ctf_net_bounded_string_t addr_str;

	(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr,
			    addr_str.buf, sizeof(addr_str.buf));

	ctf_top_socket_getpeername_exit(sock, addr_str, *addrlen, ret);
}

void sys_trace_socket_getsockname_enter(int sock)
{
	ctf_top_socket_getsockname_enter(sock);
}

void sys_trace_socket_getsockname_exit(int sock, const struct sockaddr *addr,
				       const size_t *addrlen, int ret)
{
	ctf_net_bounded_string_t addr_str;

	(void)net_addr_ntop(addr->sa_family, &net_sin(addr)->sin_addr,
			    addr_str.buf, sizeof(addr_str.buf));

	ctf_top_socket_getsockname_exit(sock, addr_str, *addrlen, ret);
}

void sys_trace_socket_socketpair_enter(int family, int type, int proto, int *sv)
{
	ctf_top_socket_socketpair_enter(family, type, proto, (uint32_t)(uintptr_t)sv);
}

void sys_trace_socket_socketpair_exit(int sock_A, int sock_B, int ret)
{
	ctf_top_socket_socketpair_exit(sock_A, sock_B, ret);
}

void sys_trace_net_recv_data_enter(struct net_if *iface, struct net_pkt *pkt)
{
	ctf_top_net_recv_data_enter((int32_t)net_if_get_by_iface(iface),
				    (uint32_t)(uintptr_t)iface,
				    (uint32_t)(uintptr_t)pkt,
				    (uint32_t)net_pkt_get_len(pkt));
}

void sys_trace_net_recv_data_exit(struct net_if *iface, struct net_pkt *pkt, int ret)
{
	ctf_top_net_recv_data_exit((int32_t)net_if_get_by_iface(iface),
				   (uint32_t)(uintptr_t)iface,
				   (uint32_t)(uintptr_t)pkt,
				   (int32_t)ret);
}

void sys_trace_net_send_data_enter(struct net_pkt *pkt)
{
	struct net_if *iface;
	int ifindex;

	iface = net_pkt_iface(pkt);
	if (iface == NULL) {
		ifindex = -1;
	} else {
		ifindex = net_if_get_by_iface(iface);
	}

	ctf_top_net_send_data_enter((int32_t)ifindex,
				    (uint32_t)(uintptr_t)iface,
				    (uint32_t)(uintptr_t)pkt,
				    (uint32_t)net_pkt_get_len(pkt));
}

void sys_trace_net_send_data_exit(struct net_pkt *pkt, int ret)
{
	struct net_if *iface;
	int ifindex;

	iface = net_pkt_iface(pkt);
	if (iface == NULL) {
		ifindex = -1;
	} else {
		ifindex = net_if_get_by_iface(iface);
	}

	ctf_top_net_send_data_exit((int32_t)ifindex,
				   (uint32_t)(uintptr_t)iface,
				   (uint32_t)(uintptr_t)pkt,
				   (int32_t)ret);
}

void sys_trace_net_rx_time(struct net_pkt *pkt, uint32_t end_time)
{
	struct net_if *iface;
	int ifindex;
	uint32_t diff;
	int tc;
	uint32_t duration_us;

	iface = net_pkt_iface(pkt);
	if (iface == NULL) {
		ifindex = -1;
		tc = 0;
		duration_us = 0;
	} else {
		ifindex = net_if_get_by_iface(iface);
		diff = end_time - net_pkt_create_time(pkt);
		tc = net_rx_priority2tc(net_pkt_priority(pkt));
		duration_us = k_cyc_to_ns_floor64(diff) / 1000U;
	}

	ctf_top_net_rx_time((int32_t)ifindex,
			    (uint32_t)(uintptr_t)iface,
			    (uint32_t)(uintptr_t)pkt,
			    (uint32_t)net_pkt_priority(pkt),
			    (uint32_t)tc,
			    (uint32_t)duration_us);
}

void sys_trace_net_tx_time(struct net_pkt *pkt, uint32_t end_time)
{
	struct net_if *iface;
	int ifindex;
	uint32_t diff;
	int tc;
	uint32_t duration_us;

	iface = net_pkt_iface(pkt);
	if (iface == NULL) {
		ifindex = -1;
		tc = 0;
		duration_us = 0;
	} else {
		ifindex = net_if_get_by_iface(iface);
		diff = end_time - net_pkt_create_time(pkt);
		tc = net_rx_priority2tc(net_pkt_priority(pkt));
		duration_us = k_cyc_to_ns_floor64(diff) / 1000U;
	}

	ctf_top_net_tx_time((int32_t)ifindex,
			    (uint32_t)(uintptr_t)iface,
			    (uint32_t)(uintptr_t)pkt,
			    (uint32_t)net_pkt_priority(pkt),
			    (uint32_t)tc,
			    (uint32_t)duration_us);
}

void sys_trace_named_event(const char *name, uint32_t arg0, uint32_t arg1)
{
	ctf_bounded_string_t ctf_name = {""};

	strncpy(ctf_name.buf, name, CTF_MAX_STRING_LEN);
	/* Make sure buffer is NULL terminated */
	ctf_name.buf[CTF_MAX_STRING_LEN - 1] = '\0';

	ctf_named_event(ctf_name, arg0, arg1);
}

/* GPIO */
void sys_port_trace_gpio_pin_interrupt_configure_enter(const struct device *port, gpio_pin_t pin,
						       gpio_flags_t flags)
{
	ctf_top_gpio_pin_interrupt_configure_enter((uint32_t)(uintptr_t)port, (uint32_t)pin,
						   (uint32_t)flags);
}

void sys_port_trace_gpio_pin_interrupt_configure_exit(const struct device *port, gpio_pin_t pin,
						      int ret)
{
	ctf_top_gpio_pin_interrupt_configure_exit((uint32_t)(uintptr_t)port, (uint32_t)pin,
						  (int32_t)ret);
}

void sys_port_trace_gpio_pin_configure_enter(const struct device *port, gpio_pin_t pin,
					     gpio_flags_t flags)
{
	ctf_top_gpio_pin_configure_enter((uint32_t)(uintptr_t)port, (uint32_t)pin, (uint32_t)flags);
}

void sys_port_trace_gpio_pin_configure_exit(const struct device *port, gpio_pin_t pin, int ret)
{
	ctf_top_gpio_pin_configure_exit((uint32_t)(uintptr_t)port, (uint32_t)pin, (int32_t)ret);
}

void sys_port_trace_gpio_port_get_direction_enter(const struct device *port, gpio_port_pins_t map,
						  gpio_port_pins_t *inputs,
						  gpio_port_pins_t *outputs)
{
	ctf_top_gpio_port_get_direction_enter((uint32_t)(uintptr_t)port, (uint32_t)map,
					      (uint32_t)(uintptr_t)inputs,
					      (uint32_t)(uintptr_t)outputs);
}

void sys_port_trace_gpio_port_get_direction_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_get_direction_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_pin_get_config_enter(const struct device *port, gpio_pin_t pin,
					      gpio_flags_t flags)
{
	ctf_top_gpio_pin_get_config_enter((uint32_t)(uintptr_t)port, (uint32_t)pin,
					  (uint32_t)flags);
}

void sys_port_trace_gpio_pin_get_config_exit(const struct device *port, gpio_pin_t pin, int ret)
{
	ctf_top_gpio_pin_get_config_exit((uint32_t)(uintptr_t)port, (uint32_t)pin, (int32_t)ret);
}

void sys_port_trace_gpio_port_get_raw_enter(const struct device *port, gpio_port_value_t *value)
{
	ctf_top_gpio_port_get_raw_enter((uint32_t)(uintptr_t)port, (uint32_t)(uintptr_t)value);
}

void sys_port_trace_gpio_port_get_raw_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_get_raw_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_port_set_masked_raw_enter(const struct device *port, gpio_port_pins_t mask,
						   gpio_port_value_t value)
{
	ctf_top_gpio_port_set_masked_raw_enter((uint32_t)(uintptr_t)port, (uint32_t)mask,
					       (uint32_t)value);
}

void sys_port_trace_gpio_port_set_masked_raw_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_set_masked_raw_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_port_set_bits_raw_enter(const struct device *port, gpio_port_pins_t pins)
{
	ctf_top_gpio_port_set_bits_raw_enter((uint32_t)(uintptr_t)port, (uint32_t)pins);
}

void sys_port_trace_gpio_port_set_bits_raw_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_set_bits_raw_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_port_clear_bits_raw_enter(const struct device *port, gpio_port_pins_t pins)
{
	ctf_top_gpio_port_clear_bits_raw_enter((uint32_t)(uintptr_t)port, (uint32_t)pins);
}

void sys_port_trace_gpio_port_clear_bits_raw_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_clear_bits_raw_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_port_toggle_bits_enter(const struct device *port, gpio_port_pins_t pins)
{
	ctf_top_gpio_port_toggle_bits_enter((uint32_t)(uintptr_t)port, (uint32_t)pins);
}

void sys_port_trace_gpio_port_toggle_bits_exit(const struct device *port, int ret)
{
	ctf_top_gpio_port_toggle_bits_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_init_callback_enter(struct gpio_callback *callback,
					     gpio_callback_handler_t handler,
					     gpio_port_pins_t pin_mask)
{
	ctf_top_gpio_init_callback_enter((uint32_t)(uintptr_t)callback,
					 (uint32_t)(uintptr_t)handler, (uint32_t)pin_mask);
}

void sys_port_trace_gpio_init_callback_exit(struct gpio_callback *callback)
{
	ctf_top_gpio_init_callback_exit((uint32_t)(uintptr_t)callback);
}

void sys_port_trace_gpio_add_callback_enter(const struct device *port,
					    struct gpio_callback *callback)
{
	ctf_top_gpio_add_callback_enter((uint32_t)(uintptr_t)port, (uint32_t)(uintptr_t)callback);
}

void sys_port_trace_gpio_add_callback_exit(const struct device *port, int ret)
{
	ctf_top_gpio_add_callback_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_remove_callback_enter(const struct device *port,
					       struct gpio_callback *callback)
{
	ctf_top_gpio_remove_callback_enter((uint32_t)(uintptr_t)port,
					   (uint32_t)(uintptr_t)callback);
}

void sys_port_trace_gpio_remove_callback_exit(const struct device *port, int ret)
{
	ctf_top_gpio_remove_callback_exit((uint32_t)(uintptr_t)port, (int32_t)ret);
}

void sys_port_trace_gpio_get_pending_int_enter(const struct device *dev)
{
	ctf_top_gpio_get_pending_int_enter((uint32_t)(uintptr_t)dev);
}

void sys_port_trace_gpio_get_pending_int_exit(const struct device *dev, int ret)
{
	ctf_top_gpio_get_pending_int_exit((uint32_t)(uintptr_t)dev, (int32_t)ret);
}

void sys_port_trace_gpio_fire_callbacks_enter(sys_slist_t *list, const struct device *port,
					      gpio_port_pins_t pins)
{
	ctf_top_gpio_fire_callbacks_enter((uint32_t)(uintptr_t)list, (uint32_t)(uintptr_t)port,
					  (uint32_t)pins);
}

void sys_port_trace_gpio_fire_callback(const struct device *port, struct gpio_callback *cb)
{
	ctf_top_gpio_fire_callback((uint32_t)(uintptr_t)port, (uint32_t)(uintptr_t)cb);
}
