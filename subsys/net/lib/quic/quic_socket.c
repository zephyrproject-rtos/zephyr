/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static int quic_connection_init(struct quic_context *ctx,
				const struct net_sockaddr *remote_addr,
				const struct net_sockaddr *local_addr)
{
	struct quic_endpoint *ep;
	int ret;

	k_mutex_lock(&endpoints_lock, K_FOREVER);

	if (remote_addr == NULL) {
		/* Server side, find endpoint by local address only */
		ep = find_local_endpoint(local_addr);
		if (ep == NULL) {
			ep = alloc_local_endpoint(ctx, local_addr);
			if (ep == NULL) {
				k_mutex_unlock(&endpoints_lock);
				return -ENOMEM;
			}

			ep->is_server = true;
			ctx->listen = ep;
			ctx->is_listening = true;

			NET_DBG("[EP:%p/%d] Created new endpoint from %s to %s", ep,
				quic_get_by_ep(ep), "ANY",
				net_sprint_addr(local_addr->sa_family,
						&net_sin(local_addr)->sin_addr));
		}
	} else {
		ep = find_endpoint(remote_addr, local_addr, NULL, 0, NULL, 0);
		if (ep == NULL) {
			ep = alloc_endpoint(ctx, remote_addr, local_addr);
			if (ep == NULL) {
				k_mutex_unlock(&endpoints_lock);
				return -ENOMEM;
			}

			ep->is_server = false;
			ctx->listen = NULL;
			ctx->is_listening = false;

			/* For client, generate random connection IDs:
			 * - peer_cid: DCID sent to server (server's CID from client's perspective)
			 * - my_cid: SCID we use to identify ourselves
			 */
			ep->peer_cid_len = 8;
			sys_rand_get(ep->peer_cid, ep->peer_cid_len);

			ep->my_cid_len = 8;
			sys_rand_get(ep->my_cid, ep->my_cid_len);

			NET_DBG("[EP:%p/%d] Created new endpoint from %s to %s", ep,
				quic_get_by_ep(ep), local_addr == NULL ? "ANY" :
				net_sprint_addr(local_addr->sa_family,
						&net_sin(local_addr)->sin_addr),
				net_sprint_addr(remote_addr->sa_family,
						&net_sin(remote_addr)->sin_addr));
		}
	}

	ctx->stream_id_counter = 0ULL;
	ctx->id = connection_ids++;

	/* Create the actual UDP socket here and do the QUIC handshake.
	 */
	if (ep->sock < 0) {
		ret = endpoint_socket_create(ep);
		if (ret < 0) {
			NET_DBG("[EP:%p/%d] Cannot create endpoint socket (%d)",
				ep, quic_get_by_ep(ep), ret);
			goto fail;
		}

		if (remote_addr == NULL) {
			NET_DBG("[EP:%p/%d] Listening on socket %d for endpoint %s -> %s",
				ep, quic_get_by_ep(ep), ep->sock,
				net_sprint_addr(ep->remote_addr.ss_family,
						&net_sin(net_sad(&ep->remote_addr))->sin_addr),
				net_sprint_addr(ep->local_addr.ss_family,
						&net_sin(net_sad(&ep->local_addr))->sin_addr));
		} else {
			NET_DBG("[EP:%p/%d] Created UDP socket %d for endpoint %s -> %s",
				ep, quic_get_by_ep(ep), ep->sock,
				net_sprint_addr(ep->local_addr.ss_family,
						&net_sin(net_sad(&ep->local_addr))->sin_addr),
				net_sprint_addr(ep->remote_addr.ss_family,
						&net_sin(net_sad(&ep->remote_addr))->sin_addr));
		}
	} else {
		NET_DBG("[EP:%p/%d] Reusing existing socket %d for endpoint %s -> %s",
			ep, quic_get_by_ep(ep), ep->sock,
			net_sprint_addr(ep->local_addr.ss_family,
					&net_sin(net_sad(&ep->local_addr))->sin_addr),
			net_sprint_addr(ep->remote_addr.ss_family,
					&net_sin(net_sad(&ep->remote_addr))->sin_addr));
	}

	/* Initialize TLS context if needed if we are a server. For client, we do
	 * any initialization when the stream is opened.
	 */
	if (!ep->crypto.tls.is_initialized) {
		tls_init(ep);

		if (ep->is_server) {
			ret = quic_tls_init(&ep->crypto.tls, ep->is_server);
			if (ret < 0) {
				NET_DBG("[EP:%p/%d] Cannot %s TLS context for endpoint (%d)",
					ep, quic_get_by_ep(ep), "initialize", ret);
			} else {
				NET_DBG("[EP:%p/%d] TLS context %p initialized for endpoint",
					ep, quic_get_by_ep(ep), &ep->crypto.tls);
			}
		}
	}

	k_mutex_unlock(&endpoints_lock);

	NET_DBG("[CO:%p/%d] Connection initialized", ctx, quic_get_by_conn(ctx));

	return 0;

fail:
	sys_slist_find_and_remove(&ctx->endpoints, &ep->node);
	quic_endpoint_unref(ep);

	k_mutex_unlock(&endpoints_lock);

	NET_DBG("[CO:%p/%d] Connection initialization failed (%d)",
		ctx, quic_get_by_conn(ctx), ret);

	return ret;
}

static int quic_getsockopt_ctx(void *obj, int level, int optname,
			       void *optval, net_socklen_t *optlen)
{
	struct quic_context *ctx = obj;
	struct quic_endpoint *ep;

	/* Get the first endpoint or the listening one */
	if (ctx->listen != NULL) {
		ep = ctx->listen;
	} else {
		ep = SYS_SLIST_PEEK_HEAD_CONTAINER(&ctx->endpoints, ep, node);
	}

	if (level != ZSOCK_SOL_TLS && level != ZSOCK_SOL_SOCKET) {
		/* If we have not created any endpoint yet, the socket options
		 * cannot be applied.
		 */
		if (ep == NULL) {
			errno = ENOENT;
			return -1;
		}

		NET_DBG("[CO:%p/%d] Passing %cetsockopt endpoint %d socket %d (%p) "
			"for level %d, optname %d",
			ctx, quic_get_by_conn(ctx), 'g',
			quic_get_by_ep(ep), ep->sock, ep, level, optname);
		return zsock_getsockopt(ep->sock, level, optname,
					optval, optlen);
	}

	switch (level) {
	case ZSOCK_SOL_TLS:
		switch (optname) {
		case ZSOCK_TLS_ALPN_LIST: {
			const char *alpn;
			size_t alpn_len;

			/* Return the negotiated ALPN protocol */
			if (ep == NULL) {
				errno = ENOTCONN;
				return -1;
			}

			alpn = ep->crypto.tls.negotiated_alpn;
			if (alpn == NULL) {
				/* No ALPN negotiated */
				if (*optlen > 0) {
					((char *)optval)[0] = '\0';
				}

				*optlen = 0;
				return 0;
			}

			alpn_len = strlen(alpn);
			if (*optlen < alpn_len + 1) {
				errno = ENOBUFS;
				return -1;
			}

			memcpy(optval, alpn, alpn_len + 1);
			*optlen = alpn_len + 1;
			return 0;
		}
		}
		break;

	case ZSOCK_SOL_SOCKET:
		switch (optname) {
		case ZSOCK_SO_ERROR: {
			if (*optlen != sizeof(int)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = ctx->error_code;
			return 0;
		}

		case ZSOCK_SO_TYPE: {
			int type = NET_SOCK_STREAM;

			if (*optlen != sizeof(type)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = type;
			return 0;
		}

		case ZSOCK_SO_PROTOCOL: {
			int proto = NET_IPPROTO_QUIC;

			if (*optlen != sizeof(proto)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = proto;
			return 0;
		}

		case ZSOCK_SO_DOMAIN: {
			struct quic_endpoint *ep_tmp;

			if (*optlen != sizeof(int)) {
				errno = EINVAL;
				return -1;
			}

			/* Get domain from listen endpoint or first endpoint */
			if (ctx->listen != NULL) {
				*(int *)optval = ctx->listen->local_addr.ss_family;
				return 0;
			}

			/* For accepted connections, get family from first endpoint */
			ep_tmp = SYS_SLIST_PEEK_HEAD_CONTAINER(&ctx->endpoints, ep_tmp, node);
			if (ep_tmp != NULL) {
				*(int *)optval = ep_tmp->local_addr.ss_family;
				return 0;
			}

			errno = ENOTCONN;
			return -1;
		}

		break;
		}

	break;
	}

	errno = EINVAL;
	return -1;
}

static int quic_setsockopt_ctx(void *obj, int level, int optname,
			       const void *optval, net_socklen_t optlen)
{
	struct quic_context *ctx = obj;
	struct quic_endpoint *ep;
	int err;

	if (sys_slist_is_empty(&ctx->endpoints)) {
		err = -ENOTCONN;
		goto out;
	}

	/* Get the first endpoint. For server it's ctx->listen, for client it's the only one */
	if (ctx->listen != NULL) {
		ep = ctx->listen;
	} else {
		ep = SYS_SLIST_PEEK_HEAD_CONTAINER(&ctx->endpoints, ep, node);
		if (ep == NULL) {
			err = -ENOTCONN;
			goto out;
		}
	}

	if (level == ZSOCK_SOL_QUIC) {
		switch (optname) {
		case ZSOCK_QUIC_SO_CERT_CHAIN_ADD:
			if (optval == NULL || optlen != sizeof(sec_tag_t)) {
				err = -EINVAL;
				break;
			}

			err = quic_tls_add_cert_chain(&ep->crypto.tls,
						     *(const sec_tag_t *)optval);
			if (err == 0) {
				NET_DBG("[CO:%p/%d] Added intermediate cert tag=%d, "
					"chain depth=%zd",
					ctx, quic_get_by_conn(ctx),
					*(const sec_tag_t *)optval,
					ep->crypto.tls.cert_chain_count);
			}

			break;

		case ZSOCK_QUIC_SO_CERT_CHAIN_DEL:
			if (optlen > 0 && optlen != sizeof(sec_tag_t)) {
				err = -EINVAL;
				break;
			}

			if (optval == NULL && optlen > 0) {
				err = -EINVAL;
				break;
			}

			if (optlen == 0) {
				/* If optlen is 0, ignore optval and delete all certs */
				optval = NULL;
			}

			err = quic_tls_del_cert_chain(&ep->crypto.tls,
						     (const sec_tag_t *)optval);
			if (err == 0) {
				if (optval == NULL || optlen == 0) {
					NET_DBG("[CO:%p/%d] Deleted all intermediate certs, "
						"chain depth=%zd",
						ctx, quic_get_by_conn(ctx),
						ep->crypto.tls.cert_chain_count);
				} else {
					NET_DBG("[CO:%p/%d] Deleted intermediate cert tag=%d, "
						"chain depth=%zd",
						ctx, quic_get_by_conn(ctx),
						*(const sec_tag_t *)optval,
						ep->crypto.tls.cert_chain_count);
				}
			}

			break;

		default:
			err = -ENOPROTOOPT;
			break;
		}

		goto out;
	}

	if (level != ZSOCK_SOL_TLS) {
		NET_DBG("[CO:%p/%d] Passing %cetsockopt endpoint %d socket %d (%p) "
			"for level %d, optname %d",
			ctx, quic_get_by_conn(ctx), 's',
			quic_get_by_ep(ep), ep->sock, ep, level, optname);
		return zsock_setsockopt(ep->sock, level, optname,
					optval, optlen);
	}

	switch (optname) {
	case ZSOCK_TLS_SEC_TAG_LIST:
		err =  tls_opt_sec_tag_list_set(&ep->crypto.tls,
						optval, optlen);
		break;

	case ZSOCK_TLS_HOSTNAME:
		err = tls_opt_hostname_set(&ep->crypto.tls,
					   optval, optlen);
		break;

	case ZSOCK_TLS_CIPHERSUITE_LIST:
		err = tls_opt_ciphersuite_list_set(&ep->crypto.tls,
						   optval, optlen);
		break;

	case ZSOCK_TLS_PEER_VERIFY:
		err = tls_opt_peer_verify_set(&ep->crypto.tls,
					      optval, optlen);
		break;

	case ZSOCK_TLS_CERT_NOCOPY:
		err = tls_opt_cert_nocopy_set(&ep->crypto.tls,
					      optval, optlen);
		break;

	case ZSOCK_TLS_ALPN_LIST:
		err = tls_opt_alpn_list_set(&ep->crypto.tls,
					    optval, optlen);
		break;

	case ZSOCK_TLS_CERT_VERIFY_CALLBACK:
		err = tls_opt_cert_verify_callback_set(&ep->crypto.tls,
						       optval, optlen);
		break;

	default:
		/* Unknown or read-only option. */
		err = -ENOPROTOOPT;
		break;
	}

out:
	if (err < 0) {
		errno = -err;
		return -1;
	}

	return 0;
}

static int quic_ctx_close_vmeth(void *obj)
{
	struct quic_context *ctx = obj;

	if (!PART_OF_ARRAY(contexts, ctx)) {
		return -EINVAL;
	}

	NET_DBG("[CO:%p/%d] Releasing context directly (sock %d)", ctx,
		quic_get_by_conn(ctx), ctx->sock);

	(void)sock_obj_core_dealloc(ctx->sock);

	/*
	 * Clear ctx->sock before calling quic_context_unref so that the
	 * ref=0 path in quic_context_unref doesn't call zsock_close() on
	 * the same fd that's already being torn down by the outer zsock_close().
	 * Zephyr's zvfs_close() removes the fd from the table before calling
	 * the vtable close, so zvfs_get_fd_obj() inside quic_connection_close()
	 * would return NULL and silently skip the unref which would leak the
	 * context.
	 */
	ctx->sock = -1;

	quic_context_unref(ctx);

	quic_stats_update_connection_closed();

	return 0;
}

static int quic_conn_poll_prepare_ctx(struct quic_context *ctx,
				      struct zsock_pollfd *pfd,
				      struct k_poll_event **pev,
				      struct k_poll_event *pev_end)
{
	int ret = 0;

	if (pfd->events & ZSOCK_POLLIN) {
		/*
		 * Non-listening contexts need two poll events: one for the
		 * incoming stream FIFO (real stream arrival) and one for the
		 * stream semaphore (connection-close wakeup from
		 * quic_endpoint_notify_streams_closed).
		 */
		int needed = ctx->is_listening ? 1 : 2;

		if (*pev + needed > pev_end) {
			return -ENOMEM;
		}

		/* Wake on either an incoming stream or an incoming connection */
		if (ctx->is_listening) {
			(*pev)->obj = &ctx->pending.accept_q;
		} else {
			(*pev)->obj = &ctx->incoming.stream_q;
		}

		(*pev)->type = K_POLL_TYPE_FIFO_DATA_AVAILABLE;
		(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
		(*pev)->state = K_POLL_STATE_NOT_READY;
		(*pev)++;

		/*
		 * For non-listening contexts also monitor the semaphore.
		 * quic_endpoint_notify_streams_closed() calls k_sem_give() on
		 * stream_sem when the peer sends CONNECTION_CLOSE, even when
		 * no new stream is queued.  Without this second event, poll()
		 * would not wake until the inactivity timer in upper layer
		 * fires.
		 */
		if (!ctx->is_listening) {
			(*pev)->obj = &ctx->incoming.stream_sem;
			(*pev)->type = K_POLL_TYPE_SEM_AVAILABLE;
			(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
			(*pev)->state = K_POLL_STATE_NOT_READY;
			(*pev)++;
		}

		/* If there are already incoming streams queued, mark as ready
		 * immediately so the accept() call wakes up immediately.
		 */
		if (ctx->is_listening) {
			if (!k_fifo_is_empty(&ctx->pending.accept_q)) {
				ret = -EALREADY;
			}
		} else {
			if (!k_fifo_is_empty(&ctx->incoming.stream_q)) {
				ret = -EALREADY;
			} else if (k_sem_count_get(&ctx->incoming.stream_sem) > 0) {
				/* Connection already closed before poll() was called */
				ret = -EALREADY;
			}
		}
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		if (*pev == pev_end) {
			return -ENOMEM;
		}

		ret = -EALREADY; /* Assume always ready to send for simplicity */
	}

	return ret;
}

static bool quic_ctx_is_eof(struct quic_context *ctx)
{
	/* If there are no endpoints, we can consider the connection to be at EOF.
	 * This can happen if the connection was never established or if all
	 * endpoints were closed.
	 */
	if (sys_slist_is_empty(&ctx->endpoints)) {
		return true;
	}

	return false;
}

static int quic_conn_poll_update_ctx(struct quic_context *ctx,
				     struct zsock_pollfd *pfd,
				     struct k_poll_event **pev)
{
	if (pfd->events & ZSOCK_POLLIN) {
		bool fifo_ready = ((*pev)->state != K_POLL_STATE_NOT_READY &&
				   (*pev)->state != K_POLL_STATE_CANCELLED);
		(*pev)++;

		if (ctx->is_listening) {
			if (fifo_ready || quic_ctx_is_eof(ctx)) {
				pfd->revents |= ZSOCK_POLLIN;
			}
		} else {
			bool sem_ready = ((*pev)->state != K_POLL_STATE_NOT_READY &&
					  (*pev)->state != K_POLL_STATE_CANCELLED);
			(*pev)++;

			if (fifo_ready || quic_ctx_is_eof(ctx)) {
				pfd->revents |= ZSOCK_POLLIN;
			} else if (sem_ready && k_fifo_is_empty(&ctx->incoming.stream_q)) {
				/*
				 * Semaphore fired but FIFO is empty: peer sent
				 * CONNECTION_CLOSE. Set POLLHUP so upper layer can
				 * tear down the connection immediately via the existing
				 * POLLHUP handler, with no SO_ERROR probe needed.
				 */
				if (ctx->error_code == 0) {
					ctx->error_code = -ECONNRESET;
				}

				pfd->revents |= ZSOCK_POLLHUP;
			}
		}
	}

	/* Report POLLHUP when the connection has been closed (no endpoints
	 * remain). This allows upper layers (e.g. the HTTP server) to
	 * immediately detect and clean up stale connections instead of
	 * waiting for an inactivity timer. POLLHUP is always reported
	 * regardless of the requested events, per POSIX semantics.
	 */
	if (quic_ctx_is_eof(ctx)) {
		pfd->revents |= ZSOCK_POLLHUP;
	}

	return 0;
}

static int quic_ctx_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	struct quic_context *ctx = obj;

	switch (request) {
	case ZFD_IOCTL_POLL_OFFLOAD:
		return -ENOTSUP;

	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return quic_conn_poll_prepare_ctx(ctx, pfd, pev, pev_end);
	}

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return quic_conn_poll_update_ctx(ctx, pfd, pev);
	}

	default:
		break;
	}

	return 0;
}

static bool quic_stream_can_send(struct quic_stream *stream)
{
	struct quic_endpoint *ep = stream->ep;
	size_t tx_free = sizeof(stream->tx_buf.data) - stream->tx_buf.len;

	/* Check TX retransmit buffer space. We need meaningful space, not just 1 byte */
	if (tx_free < 64) {
		return false;
	}

	/* Check flow control window */
	if (ep == NULL || !ep->crypto.application.initialized) {
		return false;
	}

	/* Use signed comparison to guard against underflow if limits were
	 * ever violated, treat any negative headroom as zero (blocked).
	 */
	if (stream->bytes_sent >= stream->remote_max_data) {
		return false;
	}

	if (ep->tx_fc.bytes_sent >= ep->tx_fc.max_data) {
		return false;
	}

	return true;
}

static int quic_stream_poll_prepare(struct quic_stream *stream,
				    struct zsock_pollfd *pfd,
				    struct k_poll_event **pev,
				    struct k_poll_event *pev_end)
{
	int ret = 0;

	if (pfd->events & ZSOCK_POLLIN) {
		if (*pev == pev_end) {
			return -ENOMEM;
		}

		(*pev)->obj = &stream->recv.signal;
		(*pev)->type = K_POLL_TYPE_SIGNAL;
		(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
		(*pev)->state = K_POLL_STATE_NOT_READY;
		(*pev)++;

		/* If there are already data pending, mark as ready immediately.
		 */
		if (stream->rx_buf.head != stream->rx_buf.tail ||
		    stream->rx_buf.fin_received) {
			ret = -EALREADY;
		}
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		if (*pev == pev_end) {
			return -ENOMEM;
		}

		/* Reset the signal before polling as we only want to wake
		 * on NEW events, not stale signals from previous ACKs
		 */
		k_poll_signal_reset(&stream->send.signal);

		(*pev)->obj = &stream->send.signal;
		(*pev)->type = K_POLL_TYPE_SIGNAL;
		(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
		(*pev)->state = K_POLL_STATE_NOT_READY;
		(*pev)++;

		NET_DBG("[%p] POLLOUT prepare: tx_free=%zu, can_send=%d",
			stream, sizeof(stream->tx_buf.data) - stream->tx_buf.len,
			quic_stream_can_send(stream));
	}

	return ret;
}

static bool quic_stream_is_eof(struct quic_stream *stream)
{
	if (stream->ep == NULL) {
		return true;
	}

	if (stream->rx_buf.head == stream->rx_buf.tail &&
	    stream->rx_buf.fin_received) {
		return true;
	}

	return false;
}

static int quic_stream_poll_update(struct quic_stream *stream,
				   struct zsock_pollfd *pfd,
				   struct k_poll_event **pev)
{
	if (pfd->events & ZSOCK_POLLIN) {
		if (((*pev)->state != K_POLL_STATE_NOT_READY &&
		     (*pev)->state != K_POLL_STATE_CANCELLED) ||
		    quic_stream_is_eof(stream)) {
			pfd->revents |= ZSOCK_POLLIN;
		}
		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		bool can_send = quic_stream_can_send(stream);

		NET_DBG("[%p] POLLOUT update: state=%d, can_send=%d, tx_free=%zu",
			stream, (*pev)->state, can_send,
			sizeof(stream->tx_buf.data) - stream->tx_buf.len);

		/* Only report POLLOUT if we can actually send data.
		 * The signal being raised just means space became available at some
		 * point, but we need to verify current state.
		 */
		if (can_send) {
			pfd->revents |= ZSOCK_POLLOUT;
		}
		(*pev)++;
	}

	if (stream->ep == NULL) {
		pfd->revents |= ZSOCK_POLLHUP;
	}

	return 0;
}

static int quic_stream_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	struct quic_stream *stream = obj;

	switch (request) {
	case ZFD_IOCTL_POLL_OFFLOAD:
		return -ENOTSUP;

	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return quic_stream_poll_prepare(stream, pfd, pev, pev_end);
	}

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return quic_stream_poll_update(stream, pfd, pev);
	}

	default:
		break;
	}

	return 0;
}

/**
 * Send a STREAM frame with the FIN bit set and zero payload.
 * This signals to the peer that we have finished sending on this stream.
 * RFC 9000 Section 19.8: STREAM frame with FIN bit = 1 and Length = 0.
 */
static int quic_send_stream_fin(struct quic_stream *stream)
{
	uint8_t frame[32]; /* 1 type + max 8 stream_id + max 8 offset + max 2 len */
	size_t frame_len = 0;
	int ret;

	if (stream->ep == NULL) {
		NET_DBG("[ST:%p/%d] Cannot send FIN: no endpoint",
			stream, quic_get_by_stream(stream));
		return -ENOTCONN;
	}

	/*
	 * STREAM frame type bits:
	 *   0x04 (OFF) = offset field present
	 *   0x02 (LEN) = length field present
	 *   0x01 (FIN) = final frame for this stream
	 */
	frame[frame_len++] = QUIC_FRAME_TYPE_STREAM_BASE | 0x04 | 0x02 | 0x01;

	/* Stream ID */
	ret = quic_put_len(&frame[frame_len], sizeof(frame) - frame_len, stream->id);
	if (ret != 0) {
		return -EINVAL;
	}
	frame_len += quic_get_varint_size(stream->id);

	/* Offset = bytes_sent (the final size of the stream) */
	ret = quic_put_len(&frame[frame_len], sizeof(frame) - frame_len, stream->bytes_sent);
	if (ret != 0) {
		return -EINVAL;
	}
	frame_len += quic_get_varint_size(stream->bytes_sent);

	/* Length = 0 (no data payload, FIN only) */
	ret = quic_put_len(&frame[frame_len], sizeof(frame) - frame_len, 0ULL);
	if (ret != 0) {
		return -EINVAL;
	}
	frame_len += quic_get_varint_size(0ULL);

	ret = quic_send_packet(stream->ep, QUIC_SECRET_LEVEL_APPLICATION,
			       frame, frame_len);
	if (ret < 0) {
		NET_DBG("[ST:%p/%d] Failed to send FIN for stream %" PRIu64 " : %d",
			stream, quic_get_by_stream(stream), stream->id, ret);
		return ret;
	}

	/* Annotate the packet for loss recovery / retransmission tracking */
	quic_annotate_last_sent_stream(stream->ep, QUIC_SECRET_LEVEL_APPLICATION,
				       stream->id, stream->bytes_sent,
				       0 /* data_len */, true /* stream_fin */);

	NET_DBG("[ST:%p/%d] Sent FIN for stream %" PRIu64 " at offset %" PRIu64,
		stream, quic_get_by_stream(stream), stream->id, stream->bytes_sent);

	return 0;
}

static ssize_t quic_stream_send(struct quic_stream *stream, const uint8_t *buf,
				size_t buf_len, int32_t timeout)
{
	struct quic_endpoint *ep;
	size_t stream_window;
	size_t conn_window;
	size_t available_window;
	struct quic_stream_tx_buffer *tx;
	size_t tx_free;
	uint64_t this_offset;
	size_t to_send;
	uint8_t frame[CONFIG_QUIC_TX_BUFFER_SIZE];
	size_t max_payload_size;
	size_t frame_len = 0;
	uint8_t frame_type;
	int state;
	int ret;

	ARG_UNUSED(timeout);

	ep = stream->ep;

	/* Check if stream has a valid endpoint with crypto context */
	if (ep == NULL) {
		NET_DBG("[ST:%p/%d] No endpoint with crypto context",
			stream, quic_get_by_stream(stream));
		return -ENOTCONN;
	}

	/* Check if the stream is in a state that allows sending */
	state = quic_stream_get_state(stream);
	if (state == STATE_RESET_SENT || state == STATE_DATA_READ) {
		NET_DBG("[ST:%p/%d] Wrong state %d for sending", stream,
			quic_get_by_stream(stream), state);
		return -EPIPE;
	}

	/* Handle QUIC Flow Control.
	 * We cannot send more data than the peer's MAX_STREAM_DATA limit
	 * or the connection-level MAX_DATA limit.
	 * Guard against unsigned underflow if limits are somehow breached.
	 */
	if (stream->bytes_sent >= stream->remote_max_data) {
		stream_window = 0;
	} else {
		stream_window = stream->remote_max_data - stream->bytes_sent;
	}

	if (ep->tx_fc.bytes_sent >= ep->tx_fc.max_data) {
		conn_window = 0;
	} else {
		conn_window = ep->tx_fc.max_data - ep->tx_fc.bytes_sent;
	}

	available_window = MIN(stream_window, conn_window);

	if (available_window == 0) {
		/* Window is full, send blocked frame to request more credit */
		if (stream_window == 0) {
			quic_send_stream_data_blocked(ep, stream);
		}

		if (conn_window == 0) {
			quic_send_data_blocked(ep);
		}

		NET_DBG("[ST:%p/%d] Window is full (stream=%zu, conn=%zu)",
			stream, quic_get_by_stream(stream), stream_window, conn_window);
		return -EAGAIN;
	}

	max_payload_size = MIN(sizeof(frame), ep->max_tx_payload_size);

	/* Subtract QUIC packet-level overhead from max_payload_size.
	 * max_tx_payload_size is the max UDP payload (MTU - IP - UDP headers),
	 * but the QUIC packet adds:
	 *   - Short header: 1 (first byte) + my_cid_len + 4 (max PN length)
	 *   - AEAD authentication tag: 16 bytes
	 * Additionally, the STREAM frame has its own header:
	 *   - 1 (frame type) + varint(stream_id) + varint(offset) + varint(len)
	 */
	{
		size_t quic_overhead = 1 + ep->my_cid_len + 4 + QUIC_AEAD_TAG_LEN;
		size_t stream_hdr = 1 + quic_get_varint_size(stream->id) +
				    quic_get_varint_size(stream->bytes_sent);

		/* Reserve space for the length varint too (estimate 2 bytes) */
		stream_hdr += 2;

		if (max_payload_size > quic_overhead + stream_hdr) {
			max_payload_size -= quic_overhead + stream_hdr;
		} else {
			max_payload_size = 0;
		}
	}

	/* Limit to what fits in frame buffer */
	to_send = MIN(buf_len, available_window);
	to_send = MIN(to_send, max_payload_size);

	/* Also limit by TX retransmit buffer space */
	tx = &stream->tx_buf;
	tx_free = sizeof(tx->data) - tx->len;

	if (tx_free == 0) {
		/* Buffer full, peer is not ACKing fast enough.
		 * Caller should retry; the PTO timer will eventually
		 * unblock things by retransmitting stalled data.
		 */
		NET_DBG("[ST:%p/%d] TX retransmit buffer full on stream %" PRIu64,
			stream, quic_get_by_stream(stream), stream->id);
		return -EAGAIN;
	}

	if (to_send > tx_free) {
		to_send = tx_free;
	}

	/* Record the stream offset before updating bytes_sent */
	this_offset = stream->bytes_sent;

	/* Copy into retransmit buffer BEFORE sending.
	 * If the send fails the data is still here; it will be
	 * retransmitted by the PTO or loss detection.
	 */
	memcpy(&tx->data[tx->len], buf, to_send);
	tx->len += to_send;

	if (to_send != buf_len) {
		NET_DBG("[ST:%p/%d] Limiting send from %zd to %zd bytes due to flow control/buffer",
			stream, quic_get_by_stream(stream), buf_len, to_send);
	} else {
		NET_DBG("[ST:%p/%d] Sending %zd bytes on stream %" PRIu64, stream,
			quic_get_by_stream(stream), to_send, stream->id);
	}

	/* Build STREAM frame:
	 * Type: 0x08 base + 0x04 (offset present) + 0x02 (length present)
	 * We always include offset and length for simplicity.
	 */
	frame_type = QUIC_FRAME_TYPE_STREAM_BASE | 0x04 | 0x02;
	frame[frame_len++] = frame_type;

	/* Stream ID (variable-length integer) */
	ret = quic_put_len(&frame[frame_len], sizeof(frame) - frame_len, stream->id);
	if (ret != 0) {
		return -EINVAL;
	}

	frame_len += quic_get_varint_size(stream->id);

	/* Offset (variable-length integer) */
	ret = quic_put_len(&frame[frame_len], sizeof(frame) - frame_len, stream->bytes_sent);
	if (ret != 0) {
		return -EINVAL;
	}

	frame_len += quic_get_varint_size(stream->bytes_sent);

	/* Length (variable-length integer) */
	ret = quic_put_len(&frame[frame_len], sizeof(frame) - frame_len, to_send);
	if (ret != 0) {
		return -EINVAL;
	}

	frame_len += quic_get_varint_size(to_send);

	/* Data */
	if (frame_len + to_send > sizeof(frame)) {
		return -ENOBUFS;
	}

	memcpy(&frame[frame_len], buf, to_send);
	frame_len += to_send;

	/* Send the packet */
	ret = quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, frame_len);
	if (ret < 0) {
		/* Roll back TX buffer on send failure */
		tx->len -= to_send;

		NET_DBG("[ST:%p/%d] Failed to send STREAM frame: %d",
			stream, quic_get_by_stream(stream), ret);
		return ret;
	}

	/* Update flow control tracking */
	stream->bytes_sent += to_send;
	ep->tx_fc.bytes_sent += to_send;

	/* Annotate the sent_pkt ring-buffer entry with stream frame info
	 * so loss detection knows what to retransmit.
	 */
	quic_annotate_last_sent_stream(ep, QUIC_SECRET_LEVEL_APPLICATION,
				       stream->id, this_offset,
				       (uint16_t)to_send, false);

	NET_DBG("[ST:%p/%d] Sent %zd bytes, stream total=%" PRIu64 ", conn total=%" PRIu64,
		stream, quic_get_by_stream(stream), to_send, stream->bytes_sent,
		ep->tx_fc.bytes_sent);

	return (ssize_t)to_send;
}

static int quic_stream_recv(struct quic_stream *stream, uint8_t *buf,
			    size_t buf_len, int32_t timeout)
{
	struct quic_stream_rx_buffer *rx = &stream->rx_buf;
	struct quic_endpoint *ep;
	k_timeout_t tout;
	size_t available;
	size_t to_copy;
	int threshold;
	int ret;
	bool buffer_empty = false;

	if (timeout == SYS_FOREVER_MS) {
		tout = K_FOREVER;
	} else if (timeout == 0) {
		tout = K_NO_WAIT;
	} else {
		tout = K_MSEC(timeout);
	}

	k_mutex_lock(&stream->cond.data_available, K_FOREVER);

	/* Wait for data if buffer is empty */
	while (rx->head == rx->tail && !rx->fin_received) {
		/* Check if connection was closed */
		if (stream->ep == NULL) {
			ret = -ECONNRESET;
			goto out;
		}

		if (K_TIMEOUT_EQ(tout, K_NO_WAIT)) {
			ret = -EAGAIN;
			goto out;
		}

		NET_DBG("[ST:%p/%d] Waiting data, buf len %zd bytes",
			stream, quic_get_by_stream(stream), buf_len);

		ret = k_condvar_wait(&stream->cond.recv, &stream->cond.data_available,
				     tout);
		if (ret == -EAGAIN) {
			ret = -ETIMEDOUT;
			goto out;
		}

		/* Check again after waking up */
		if (stream->ep == NULL) {
			ret = -ECONNRESET;
			goto out;
		}
	}

	/* Check for end of data */
	if (quic_stream_is_eof(stream)) {
		ret = 0;   /* EOF */
		goto out;
	}

	/* Copy data to user buffer */
	available = rx->tail - rx->head;
	to_copy = MIN(available, buf_len);

	memcpy(buf, &rx->data[rx->head], to_copy);
	rx->head += to_copy;
	rx->read_offset += to_copy;

	/* Compact buffer if needed */
	if (rx->head == rx->tail) {
		rx->head = 0;
		rx->tail = 0;
		buffer_empty = true;

		/* Buffer is now empty, reset the poll signal so poll()
		 * doesn't immediately return POLLIN when there's no data.
		 */
		k_poll_signal_reset(&stream->recv.signal);
	} else if (rx->head > rx->size / 2) {
		/* Shift remaining data to beginning */
		memmove(rx->data, &rx->data[rx->head], rx->tail - rx->head);
		rx->tail -= rx->head;
		rx->head = 0;
	}

	/* Remember endpoint while the mutex is still held.
	 * The quic_endpoint_notify_streams_closed() can set stream->ep = NULL from
	 * another thread at any point after we release the mutex. We must not
	 * dereference stream->ep after the unlock without checking.
	 */
	ep = stream->ep;

	k_mutex_unlock(&stream->cond.data_available);

	NET_DBG("[ST:%p/%d] Received %zd bytes", stream, quic_get_by_stream(stream), to_copy);

	/* Update flow control: increase window and mark for sending update */
	stream->local_max_data += to_copy;

	/* Update at desired number of bytes consumed */
	threshold = (int)(stream->local_max_data_sent /
			  (100 / CONFIG_QUIC_STREAM_RX_WINDOW_UPDATE_THRESHOLD));

	if (stream->local_max_data > (stream->local_max_data_sent + threshold)) {
		/* Send update when we've consumed enough data */
		NET_DBG("[ST:%p/%d] Update stream window (threshold %d)", stream,
			quic_get_by_stream(stream), threshold);
		NET_DBG("[ST:%p/%d] Stream local_max_data=%" PRIu64
			", local_max_data_sent=%" PRIu64,
			stream, quic_get_by_stream(stream), stream->local_max_data,
			stream->local_max_data_sent);

		if (ep != NULL) {
			quic_send_max_stream_data(ep, stream);
		}

	} else if (buffer_empty &&
		   stream->local_max_data > stream->local_max_data_sent) {
		/* Buffer is empty and we have window credit not yet advertised.
		 * The peer may be blocked at the flow control limit. Send update
		 * to prevent deadlock, but only if the unadvertised credit is
		 * meaningful (more than half the RX buffer size).
		 */
		uint64_t unadvertised = stream->local_max_data -
					stream->local_max_data_sent;
		if (unadvertised > rx->size / 2) {
			NET_DBG("[ST:%p/%d] Buffer empty, unadvertised=%" PRIu64
				", sending window update",
				stream, quic_get_by_stream(stream), unadvertised);

			if (ep != NULL) {
				quic_send_max_stream_data(ep, stream);
			}
		}
	}

	/* Also advance connection-level flow control window */
	if (ep != NULL) {
		int conn_threshold;

		ep->rx_fc.max_data += to_copy;

		conn_threshold = (int)(ep->rx_fc.max_data_sent /
				       (100 / CONFIG_QUIC_STREAM_RX_WINDOW_UPDATE_THRESHOLD));

		if (ep->rx_fc.max_data > (ep->rx_fc.max_data_sent + conn_threshold)) {
			NET_DBG("[EP:%p/%d] Update conn window: max_data=%" PRIu64
				" sent=%" PRIu64,
				ep, quic_get_by_ep(ep), ep->rx_fc.max_data,
				ep->rx_fc.max_data_sent);
			ep->rx_fc.max_data_sent = ep->rx_fc.max_data;
			quic_send_max_data(ep);
		} else if (buffer_empty &&
			   ep->rx_fc.max_data > ep->rx_fc.max_data_sent) {
			uint64_t unadvertised = ep->rx_fc.max_data -
						ep->rx_fc.max_data_sent;
			if (unadvertised > rx->size / 2) {
				NET_DBG("[EP:%p/%d] Buffer empty, sending conn window update",
					ep, quic_get_by_ep(ep));
				ep->rx_fc.max_data_sent = ep->rx_fc.max_data;
				quic_send_max_data(ep);
			}
		}
	}

	return to_copy;

out:
	k_mutex_unlock(&stream->cond.data_available);

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return ret;
}

static ssize_t quic_stream_read_vmeth(void *obj, void *buffer, size_t count)
{
	ssize_t ret;

	ret = quic_stream_recv(obj, buffer, count, SYS_FOREVER_MS);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}

static ssize_t quic_stream_write_vmeth(void *obj, const void *buffer, size_t count)
{
	ssize_t ret;

	ret = quic_stream_send(obj, buffer, count, SYS_FOREVER_MS);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}

static void quic_stream_flush_queue(struct quic_stream *stream)
{
	/* Wake reader if it was sleeping */
	(void)k_condvar_signal(&stream->cond.recv);
	(void)k_mutex_unlock(&stream->cond.data_available);
}

static int quic_stream_accept(struct quic_context *ctx, k_timeout_t timeout,
			      struct quic_stream **new_stream)
{
	struct quic_stream *stream;
	int fd;
	int ret;

	/* Wait for an incoming stream to be queued by handle_stream_frame.
	 * Don't pre-allocate a stream, let the packet handler create it
	 * when actual traffic arrives. This avoids consuming stream slots
	 * for idle listeners.
	 */
	ret = k_sem_take(&ctx->incoming.stream_sem, timeout);
	if (ret < 0) {
		return ret;
	}

	/* Get the stream from the incoming queue */
	stream = k_fifo_get(&ctx->incoming.stream_q, K_NO_WAIT);
	if (stream == NULL) {
		/* Semaphore was signaled but no stream in queue, this should not happen */
		NET_DBG("Stream semaphore signaled but queue empty");
		return -EAGAIN;
	}

	/* Create file descriptor for the stream */
	fd = zvfs_reserve_fd();
	if (fd < 0) {
		quic_stream_unref(stream);
		return -ENOSPC;
	}

	zvfs_finalize_typed_fd(fd, stream,
			       (const struct fd_op_vtable *)&quic_stream_fd_op_vtable,
			       ZVFS_MODE_IFSOCK);

	stream->sock = fd;
	*new_stream = stream;

	NET_DBG("[CO:%p/%d] Accepted stream %" PRIu64 " on fd %d",
		ctx, quic_get_by_conn(ctx), stream->id, fd);

	return fd;
}

/**
 * Accept a fully-established connection from the listening context's
 * pending.accept_q. Returns the new connection fd, or <0 on error.
 */
static int quic_connection_accept(struct quic_context *listen_ctx,
				  k_timeout_t timeout,
				  struct quic_context **new_ctx_out)
{
	struct quic_context *child_ctx;
	int fd;
	int ret;

	ret = k_sem_take(&listen_ctx->pending.accept_sem, timeout);
	if (ret < 0) {
		return ret;   /* -EAGAIN on K_NO_WAIT, -ETIMEDOUT otherwise */
	}

	child_ctx = k_fifo_get(&listen_ctx->pending.accept_q, K_NO_WAIT);
	if (child_ctx == NULL) {
		/* Semaphore was given but queue is empty, shouldn't happen */
		NET_DBG("Connection semaphore signalled but queue empty");
		return -EAGAIN;
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		quic_context_unref(child_ctx);
		return -ENOSPC;
	}

	zvfs_finalize_typed_fd(fd, child_ctx,
			       (const struct fd_op_vtable *)&quic_ctx_fd_op_vtable,
			       ZVFS_MODE_IFSOCK);

	child_ctx->sock = fd;
	*new_ctx_out = child_ctx;

	NET_DBG("[CO:%p/%d] Accepted connection context %p/%d on fd %d",
		listen_ctx, quic_get_by_conn(listen_ctx),
		child_ctx, quic_get_by_conn(child_ctx), fd);

	return fd;
}

static int quic_accept_ctx(void *obj, struct net_sockaddr *addr,
			   net_socklen_t *addrlen)
{
	struct quic_context *ctx = obj;
	k_timeout_t timeout = K_FOREVER;
	struct quic_context *new_ctx = NULL;
	struct quic_stream *new_stream = NULL;
	int ret, fd;

	/*
	 * Prefer connection-level accept over stream-level accept.
	 * If a fully-established connection is waiting in pending.accept_q,
	 * return a connection socket (quic_ctx_fd_op_vtable).
	 * Otherwise fall through to the stream accept path.
	 */
	if (!k_fifo_is_empty(&ctx->pending.accept_q)) {
		fd = quic_connection_accept(ctx, timeout, &new_ctx);
		if (fd < 0) {
			errno = -fd;
			return -1;
		}

		if (addr != NULL && addrlen != NULL && new_ctx != NULL) {
			struct quic_endpoint *ep =
				SYS_SLIST_PEEK_HEAD_CONTAINER(&new_ctx->endpoints,
							      ep, node);

			if (ep != NULL) {
				int len = MIN(*addrlen, sizeof(ep->remote_addr));

				memcpy(addr, &ep->remote_addr, len);

				if (ep->remote_addr.ss_family == NET_AF_INET) {
					*addrlen = sizeof(struct net_sockaddr_in);
				} else if (ep->remote_addr.ss_family == NET_AF_INET6) {
					*addrlen = sizeof(struct net_sockaddr_in6);
				}
			}
		}

		return fd;
	}

	/* No pending connection, fall back to stream accept */
	fd = quic_stream_accept(ctx, timeout, &new_stream);
	if (fd < 0) {
		errno = -fd;
		return -1;
	}

	if (new_stream == NULL) {
		errno = EAGAIN;
		return -1;
	}

	if (addr != NULL && addrlen != NULL) {
		int len;

		if (new_stream->conn == NULL || new_stream->ep == NULL) {
			ret = -ENOTCONN;
			goto fail;
		}

		len = MIN(*addrlen, sizeof(new_stream->ep->remote_addr));

		memcpy(addr, &new_stream->ep->remote_addr, len);

		/* addrlen is a value-result argument, set to actual
		 * size of source address
		 */
		if (new_stream->ep->remote_addr.ss_family == NET_AF_INET) {
			*addrlen = sizeof(struct net_sockaddr_in);
		} else if (new_stream->ep->remote_addr.ss_family == NET_AF_INET6) {
			*addrlen = sizeof(struct net_sockaddr_in6);
		} else {
			ret = -ENOTSUP;
			goto fail;
		}
	}

	return fd;

fail:
	zvfs_free_fd(fd);
	quic_stream_flush_queue(new_stream);
	quic_stream_unref(new_stream);

	errno = -ret;

	return -1;
}

static ssize_t quic_stream_sendto_ctx(void *obj, const void *buf, size_t len,
				      int flags, const struct net_sockaddr *dest_addr,
				      net_socklen_t addrlen)
{
	struct quic_stream *stream = obj;
	int32_t timeout = SYS_FOREVER_MS;
	ssize_t ret;

	if (flags & ZSOCK_MSG_DONTWAIT) {
		timeout = 0;
	}

	ARG_UNUSED(dest_addr);
	ARG_UNUSED(addrlen);

	ret = quic_stream_send(stream, buf, len, timeout);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	return ret;
}

static ssize_t quic_stream_recvfrom_ctx(void *obj, void *buf, size_t max_len,
					int flags, struct net_sockaddr *src_addr,
					net_socklen_t *addrlen)
{
	struct quic_stream *stream = obj;
	int32_t timeout = SYS_FOREVER_MS;
	ssize_t ret;

	if (flags & ZSOCK_MSG_DONTWAIT) {
		timeout = 0;
	}

	if (src_addr != NULL && addrlen != NULL && stream->ep != NULL) {
		int len;

		len = MIN(*addrlen, sizeof(stream->ep->remote_addr));

		/* addrlen is a value-result argument, set to actual
		 * size of source address
		 */
		if (stream->ep->remote_addr.ss_family == NET_AF_INET) {
			*addrlen = sizeof(struct net_sockaddr_in);
		} else if (stream->ep->remote_addr.ss_family == NET_AF_INET6) {
			*addrlen = sizeof(struct net_sockaddr_in6);
		} else {
			errno = ENOTSUP;
			return -1;
		}

		memcpy(src_addr, &stream->ep->remote_addr, len);
	}

	ret = quic_stream_recv(stream, buf, max_len, timeout);
	/* quic_stream_recv returns -1 with errno set on error,
	 * or number of bytes on success. Don't overwrite errno.
	 */

	return ret;
}

static int quic_stream_getsockopt_ctx(void *obj, int level, int optname,
				      void *optval, net_socklen_t *optlen)
{
	struct quic_stream *stream = obj;

	switch (level) {
	case ZSOCK_SOL_SOCKET:
		switch (optname) {
		case ZSOCK_SO_ERROR: {
			if (*optlen != sizeof(int)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = 0; /* TODO: FIXME */
			return 0;
		}

		case ZSOCK_SO_TYPE: {
			int type = NET_SOCK_STREAM;

			if (*optlen != sizeof(type)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = type;
			return 0;
		}

		case ZSOCK_SO_PROTOCOL: {
			int proto = NET_IPPROTO_QUIC;

			if (*optlen != sizeof(proto)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = proto;
			return 0;
		}

		case ZSOCK_SO_DOMAIN: {
			struct quic_endpoint *ep;

			if (*optlen != sizeof(int)) {
				errno = EINVAL;
				return -1;
			}

			if (stream->ep != NULL) {
				*(int *)optval = stream->ep->local_addr.ss_family;
				return 0;
			}

			if (stream->conn != NULL) {
				if (stream->conn->listen != NULL) {
					*(int *)optval =
						stream->conn->listen->local_addr.ss_family;
					return 0;
				}

				ep = SYS_SLIST_PEEK_HEAD_CONTAINER(
					&stream->conn->endpoints, ep, node);
				if (ep != NULL) {
					*(int *)optval = ep->local_addr.ss_family;
					return 0;
				}
			}

			errno = ENOTCONN;
			return -1;
		}

		default:
			errno = ENOTSUP;
			return -1;
		}

	case ZSOCK_SOL_TLS:
		switch (optname) {
		case ZSOCK_TLS_ALPN_LIST: {
			const char *alpn;
			size_t alpn_len;

			/* Return the negotiated ALPN protocol from the stream's endpoint */
			if (stream->ep == NULL || !stream->ep->crypto.tls.is_initialized) {
				errno = ENOTCONN;
				return -1;
			}

			alpn = stream->ep->crypto.tls.negotiated_alpn;
			if (alpn == NULL) {
				/* No ALPN negotiated */
				if (*optlen > 0) {
					((char *)optval)[0] = '\0';
				}
				*optlen = 0;
				return 0;
			}

			alpn_len = strlen(alpn);
			if (*optlen < alpn_len + 1) {
				errno = ENOBUFS;
				return -1;
			}

			memcpy(optval, alpn, alpn_len + 1);
			*optlen = alpn_len + 1;
			return 0;
		}
		}
		break;

	case ZSOCK_SOL_QUIC:
		switch (optname) {
		case ZSOCK_QUIC_SO_STREAM_TYPE: {
			if (*optlen != sizeof(int)) {
				errno = EINVAL;
				return -1;
			}

			/* Return stream type (combination of direction | initiator) */
			*(int *)optval = stream->type;
			return 0;
		}
		}
		break;

	default:
		errno = ENOTSUP;
		return -1;
	}

	errno = ENOPROTOOPT;
	return -1;
}

static int quic_stream_setsockopt_ctx(void *obj, int level, int optname,
				      const void *optval, net_socklen_t optlen)
{
	struct quic_stream *stream = obj;

	switch (level) {
	case ZSOCK_SOL_SOCKET:
		switch (optname) {
		default:
			return -ENOTSUP;
		}

	case ZSOCK_SOL_QUIC:
		switch (optname) {
		case ZSOCK_QUIC_SO_STOP_SENDING_CODE: {
			uint64_t code;

			if (optlen != sizeof(code)) {
				errno = EINVAL;
				return -1;
			}

			memcpy(&code, optval, sizeof(code));
			stream->stop_sending_error_code = code;
			return 0;
		}
		default:
			return -ENOTSUP;
		}

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int quic_stream_close_vmeth(void *obj)
{
	struct quic_stream *stream = obj;
	int sock = stream->sock;
	int state;

	NET_DBG("[ST:%p/%d] Closing stream id %" PRIu64 ", sock %d",
		stream, quic_get_by_stream(stream), stream->id, sock);

	/*
	 * Send FIN to the peer to cleanly terminate the send side of the stream.
	 * Skip if:
	 *   - No endpoint (connection already gone)
	 *   - Stream was reset (RESET_STREAM already sent/received)
	 *   - FIN was already sent (DATA_SENT or DATA_RECVD)
	 */
	if (stream->ep != NULL) {
		state = quic_stream_get_state(stream);

		if (state != STATE_RESET_SENT &&
		    state != STATE_RESET_RECVD &&
		    state != STATE_RESET_READ &&
		    state != STATE_DATA_SENT &&
		    state != STATE_DATA_RECVD &&
		    state != STATE_DATA_READ) {
			(void)quic_send_stream_fin(stream);
		}
	}

	/* Clear the socket fd first to prevent recursive close in unref */
	stream->sock = -1;

	/* Dealloc the socket object core tracking */
	(void)sock_obj_core_dealloc(sock);

	/* Release the stream back to the pool */
	(void)quic_stream_unref(stream);

	quic_stats_update_stream_closed();

	return 0;
}

static int quic_stream_shutdown_vmeth(void *obj, int how)
{
	struct quic_stream *stream = obj;
	int ret = 0;

	if (!PART_OF_ARRAY(streams, stream)) {
		return -EINVAL;
	}

	/*
	 * SHUT_WR (or SHUT_RDWR): send a STREAM+FIN to close our sending half.
	 * RFC 9000 ch. 2.4.
	 */
	if ((how == ZSOCK_SHUT_WR || how == ZSOCK_SHUT_RDWR) &&
	    stream->ep != NULL && stream->ep->crypto.application.initialized) {
		int state;

		state = quic_stream_get_state(stream);

		/* Only send FIN if we haven't already (state machine guard) */
		if (state != STATE_DATA_SENT &&
		    state != STATE_DATA_RECVD &&
		    state != STATE_DATA_READ &&
		    state != STATE_RESET_SENT &&
		    state != STATE_RESET_RECVD &&
		    state != STATE_RESET_READ) {
			ret = quic_send_stream_fin(stream);
			if (ret < 0) {
				NET_DBG("[ST:%p/%d] shutdown(SHUT_WR): FIN send failed (%d)",
					stream, quic_get_by_stream(stream), ret);
				return ret;
			}

			/* Advance state so quic_stream_close_vmeth won't send a second FIN */
			smf_set_state(SMF_CTX(&stream->state.ctx),
				      &quic_stream_bidirectional_states[STATE_DATA_SENT]);

			NET_DBG("[ST:%p/%d] shutdown(SHUT_WR): FIN sent on stream %" PRIu64,
				stream, quic_get_by_stream(stream), stream->id);
		} else {
			NET_DBG("[ST:%p/%d] shutdown(SHUT_WR): FIN already sent (state=%d)",
				stream, quic_get_by_stream(stream), state);
		}
	}

	/*
	 * SHUT_RD (or SHUT_RDWR): tell the peer to stop sending via STOP_SENDING.
	 * RFC 9000 ch. 2.5.
	 *
	 * Skip if the peer already sent FIN (stream is in SIZE_KNOWN or later),
	 * all data has already arrived so there is nothing left to stop.
	 */
	if ((how == ZSOCK_SHUT_RD || how == ZSOCK_SHUT_RDWR) &&
	    stream->ep != NULL && stream->ep->crypto.application.initialized) {
		int state;

		state = quic_stream_get_state(stream);
		if (state != STATE_SIZE_KNOWN &&
		    state != STATE_DATA_RECVD &&
		    state != STATE_DATA_READ  &&
		    state != STATE_RESET_RECVD &&
		    state != STATE_RESET_READ) {
			stream->read_closed = true;

			/*
			 * Callers that want a specific error code should call can set the
			 * desired error code using setsockopt().
			 */
			ret = quic_send_stop_sending(stream->ep, stream->id,
						     stream->stop_sending_error_code);
			if (ret < 0) {
				NET_DBG("[ST:%p/%d] shutdown with error %" PRIu64
					" : STOP_SENDING failed (%d)",
					stream, quic_get_by_stream(stream),
					stream->stop_sending_error_code, ret);
				return ret;
			}

			NET_DBG("[ST:%p/%d] shutdown(SHUT_RD): STOP_SENDING sent on stream "
				"%" PRIu64, stream, quic_get_by_stream(stream), stream->id);
		} else {
			NET_DBG("[ST:%p/%d] shutdown(SHUT_RD): peer already done (state=%d), "
				"skipping STOP_SENDING", stream, quic_get_by_stream(stream), state);
		}
	}

	return ret;
}

static const struct socket_op_vtable quic_ctx_fd_op_vtable = {
	.fd_vtable = {
		.close = quic_ctx_close_vmeth,
		.ioctl = quic_ctx_ioctl_vmeth,
	},
	.accept = quic_accept_ctx,
	.getsockopt = quic_getsockopt_ctx,
	.setsockopt = quic_setsockopt_ctx,
};

static const struct socket_op_vtable quic_stream_fd_op_vtable = {
	.fd_vtable = {
		.read = quic_stream_read_vmeth,
		.write = quic_stream_write_vmeth,
		.close = quic_stream_close_vmeth,
		.ioctl = quic_stream_ioctl_vmeth,
	},
	.sendto = quic_stream_sendto_ctx,
	.recvfrom = quic_stream_recvfrom_ctx,
	.getsockopt = quic_stream_getsockopt_ctx,
	.setsockopt = quic_stream_setsockopt_ctx,
	.shutdown = quic_stream_shutdown_vmeth,
};

int quic_connection_open(const struct net_sockaddr *remote_addr,
			 const struct net_sockaddr *local_addr)
{
	struct quic_context *ctx;
	int ret;
	int fd;

	ctx = quic_get();
	if (ctx == NULL) {
		quic_stats_update_connection_open_failed();
		return -ENOMEM;
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		ret = -ENOSPC;
		goto out;
	}

	zvfs_finalize_typed_fd(fd, ctx,
			       (const struct fd_op_vtable *)&quic_ctx_fd_op_vtable,
			       ZVFS_MODE_IFSOCK);

	ret = quic_connection_init(ctx, remote_addr, local_addr);
	if (ret < 0) {
		NET_DBG("Failed to initialize connection (%d)", ret);
		goto out;
	}

	ctx->sock = fd;

	if (ctx->listen != NULL) {
		(void)sock_obj_core_alloc_find(ctx->listen->sock, fd, NET_SOCK_STREAM);
	}

	/* For client connections, just return the connection socket.
	 * The handshake will be initiated when the first stream is opened,
	 * allowing the user to set certificates and ALPN first.
	 */

	ret = fd;

out:
	if (ret < 0) {
		quic_context_unref(ctx);

		if (fd >= 0) {
			zvfs_free_fd(fd);
		}

		quic_stats_update_connection_open_failed();
	} else {
		quic_stats_update_connection_opened();
	}

	return ret;
}

int quic_connection_close(int sock)
{
	struct quic_context *ctx;

	if (sock < 0) {
		return -EBADF;
	}

	ctx = zvfs_get_fd_obj(sock, (const struct fd_op_vtable *)&quic_ctx_fd_op_vtable,
			      ENOENT);
	if (ctx == NULL) {
		/* It might be that the connection is already closed, in which case
		 * ignore the error.
		 */
		if (errno == EBADF) {
			/* EBADF is returned if the socket fd refcount is 0,
			 * which means the connection is already closed and
			 * cleaned up. It can also be returned if the fd is invalid,
			 * but we have no way to distinguish that case.
			 * In either case, just return success since the connection
			 * is effectively closed.
			 */
			return 0;
		}

		quic_stats_update_connection_close_failed();
		return -errno;
	}

	if (!PART_OF_ARRAY(contexts, ctx)) {
		quic_stats_update_connection_close_failed();
		return -EINVAL;
	}

	(void)sock_obj_core_dealloc(ctx->sock);
	quic_context_unref(ctx);

	quic_stats_update_connection_closed();
	return 0;
}

static uint64_t quic_stream_id_get(struct quic_stream *stream)
{
	return ((stream->conn->stream_id_counter++) << 2) | (uint64_t)stream->type;
}

int quic_stream_open(int connection_sock,
		     enum quic_stream_initiator initiator,
		     enum quic_stream_direction direction,
		     uint8_t priority)
{
	struct quic_context *ctx;
	struct quic_stream *stream;
	struct quic_stream *existing_stream;
	struct quic_endpoint *ep;
	int ret;
	int fd;

	ctx = zvfs_get_fd_obj(connection_sock,
			      (const struct fd_op_vtable *)&quic_ctx_fd_op_vtable,
			      ENOENT);
	if (ctx == NULL) {
		/* Try to get endpoint from a stream socket instead */
		existing_stream = zvfs_get_fd_obj(connection_sock,
				(const struct fd_op_vtable *)&quic_stream_fd_op_vtable,
				ENOENT);
		if (existing_stream != NULL && PART_OF_ARRAY(streams, existing_stream)) {
			ep = existing_stream->ep;
			ctx = existing_stream->conn;
			goto have_endpoint;
		}

		quic_stats_update_stream_open_failed();
		return -errno;
	}

	if (!PART_OF_ARRAY(contexts, ctx)) {
		quic_stats_update_stream_open_failed();
		return -EINVAL;
	}

	/* Get the endpoint for this context */
	ep = SYS_SLIST_PEEK_HEAD_CONTAINER(&ctx->endpoints, ep, node);
	if (ep == NULL) {
		NET_DBG("No endpoint for connection context");
		quic_stats_update_stream_open_failed();
		return -ENOTCONN;
	}

have_endpoint:

	/* We do the TLS init here so that user has a chance to
	 * set TLS options on the endpoint before the handshake starts.
	 */
	if (!ep->crypto.tls.is_initialized && !ep->is_server) {
		ret = quic_tls_init(&ep->crypto.tls, ep->is_server);
		if (ret < 0) {
			NET_DBG("[EP:%p/%d] Cannot %s TLS context for endpoint (%d)",
				ep, quic_get_by_ep(ep), "initialize", ret);
		} else {
			NET_DBG("[EP:%p/%d] TLS context %p initialized for endpoint",
				ep, quic_get_by_ep(ep), &ep->crypto.tls);
		}
	}

	/* For client connections, initiate handshake if not yet started */
	if (!ep->is_server && ep->crypto.tls.is_initialized) {
		if (ep->crypto.tls.state == QUIC_TLS_STATE_INITIAL) {
			NET_DBG("[EP:%p/%d] Initiating client handshake on stream open",
				ep, quic_get_by_ep(ep));

			quic_endpoint_init_handshake_timeout(ep,
							     QUIC_DEFAULT_HANDSHAKE_TIMEOUT_MS);

			/* Derive Initial secrets from the server's connection ID */
			if (!quic_conn_init_setup(ep, ep->peer_cid, ep->peer_cid_len)) {
				NET_DBG("[EP:%p/%d] Failed to setup initial crypto",
					ep, quic_get_by_ep(ep));
				quic_stats_update_stream_open_failed();
				return -EIO;
			}

			/* Start the TLS handshake (send ClientHello) */
			ret = quic_tls_client_start(&ep->crypto.tls);
			if (ret != 0) {
				NET_DBG("[EP:%p/%d] Failed to start client TLS: %d",
					ep, quic_get_by_ep(ep), ret);
				quic_stats_update_stream_open_failed();
				return ret;
			}
		}

		/* Wait for handshake to complete if not already done */
		if (!ep->handshake.completed) {
			ret = k_sem_take(&ep->handshake.sem,
					 K_MSEC(ep->handshake.timeout_ms));
			if (ret != 0) {
				NET_ERR("[EP:%p/%d] Handshake timeout", ep, quic_get_by_ep(ep));
				quic_stats_update_stream_open_failed();
				return -ETIMEDOUT;
			}

			if (!ep->handshake.completed) {
				NET_ERR("[EP:%p/%d] Handshake failed", ep, quic_get_by_ep(ep));
				quic_stats_update_stream_open_failed();
				return -ECONNREFUSED;
			}
		}
	}

	stream = quic_get_stream(ctx);
	if (stream == NULL) {
		quic_stats_update_stream_open_failed();
		return -ENOMEM;
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		ret = -ENOSPC;
		goto out;
	}

	zvfs_finalize_typed_fd(fd, stream,
			       (const struct fd_op_vtable *)&quic_stream_fd_op_vtable,
			       ZVFS_MODE_IFSOCK);

	stream->sock = fd;
	stream->ep = ep;  /* Associate stream with endpoint */

	(void)sock_obj_core_alloc_find(stream->conn->sock, fd, NET_SOCK_STREAM);

	stream->type = initiator | direction;
	stream->priority = priority;
	stream->id = quic_stream_id_get(stream);

	/* Initialize TX flow control from peer transport parameters */
	if (ep->peer_params.parsed) {
		if (direction == QUIC_STREAM_BIDIRECTIONAL) {
			/* For locally-initiated bidi stream, peer's bidi_remote is our TX limit */
			stream->remote_max_data =
				ep->peer_params.initial_max_stream_data_bidi_remote;
		} else {
			/* For locally-initiated uni stream, peer's uni is our TX limit */
			stream->remote_max_data =
				ep->peer_params.initial_max_stream_data_uni;
		}
		NET_DBG("[%p] Stream TX flow control: remote_max_data=%" PRIu64,
			stream, stream->remote_max_data);
	}

	if (initiator == QUIC_STREAM_CLIENT) {
		NET_DBG("[ST:%p/%d] Opened %s %s stream id %" PRIu64 " prio %u on conn %p",
			stream, quic_get_by_stream(stream), "client",
			direction == QUIC_STREAM_BIDIRECTIONAL ?
			"bidirectional" : "unidirectional",
			stream->id, priority, ctx);

		/* Client initiated streams start in READY state. */
		smf_set_initial(SMF_CTX(&stream->state.ctx),
				&quic_stream_bidirectional_states[STATE_READY]);
	} else {
		NET_DBG("[ST:%p/%d] Opened %s %s stream id %" PRIu64 " prio %u on conn %p",
			stream, quic_get_by_stream(stream), "server",
			direction == QUIC_STREAM_BIDIRECTIONAL ?
			"bidirectional" : "unidirectional",
			stream->id, priority, ctx);

		/* Server initiated streams start in RECV state. */
		smf_set_initial(SMF_CTX(&stream->state.ctx),
				&quic_stream_bidirectional_states[STATE_RECV]);
	}

	ret = smf_run_state(SMF_CTX(&stream->state.ctx));
	if (ret != 0) {
		NET_DBG("[ST:%p/%d] Failed to run initial stream state (%d)",
			stream, quic_get_by_stream(stream), ret);
		goto out;
	}

	ret = fd;
out:
	if (ret < 0) {
		(void)quic_stream_unref(stream);

		if (fd >= 0) {
			zvfs_free_fd(fd);
		}

		quic_stats_update_stream_open_failed();
	} else {
		quic_stats_update_stream_opened();
	}

	return ret;
}

int quic_stream_close(int sock)
{
	struct quic_stream *stream;

	stream = zvfs_get_fd_obj(sock,
				 (const struct fd_op_vtable *)&quic_stream_fd_op_vtable,
				 ENOENT);
	if (stream == NULL) {
		quic_stats_update_stream_close_failed();
		return -errno;
	}

	if (!PART_OF_ARRAY(contexts, stream->conn)) {
		quic_stats_update_stream_close_failed();
		return -EINVAL;
	}

	/* Do NOT send FIN here. The caller must call shutdown(SHUT_WR) first
	 * if it wants a clean half-close. zsock_close() without prior shutdown
	 * is an abortive reset, which is the correct behaviour on error paths.
	 */

	(void)sock_obj_core_dealloc(stream->sock);
	(void)quic_stream_unref(stream);

	quic_stats_update_stream_closed();
	return 0;
}

bool quic_is_stream_socket(int sock)
{
	struct quic_stream *stream;

	stream = zvfs_get_fd_obj(sock,
				 (const struct fd_op_vtable *)&quic_stream_fd_op_vtable,
				 ENOENT);
	if (stream == NULL) {
		return false;
	}

	if (!PART_OF_ARRAY(streams, stream)) {
		return false;
	}

	return true;
}

bool quic_is_connection_socket(int sock)
{
	return quic_get_context(sock) != NULL;
}

int quic_stream_get_id(int sock, uint64_t *stream_id)
{
	struct quic_stream *stream;

	stream = zvfs_get_fd_obj(sock,
				 (const struct fd_op_vtable *)&quic_stream_fd_op_vtable,
				 ENOENT);
	if (stream == NULL) {
		return -ENOENT;
	}

	if (!PART_OF_ARRAY(streams, stream)) {
		return -EINVAL;
	}

	*stream_id = stream->id;

	return 0;
}
