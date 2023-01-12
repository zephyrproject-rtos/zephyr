/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rpc_demo/rxtx.h"
#include "rpc_demo/deque_stream.h"

namespace {
DequeReadWriter client_to_service_stream;
DequeReadWriter service_to_client_stream;
} // namespace

pw::stream::ReaderWriter &ClientToServiceStream() {
  return client_to_service_stream;
}
pw::stream::ReaderWriter &ServiceToClientStream() {
  return service_to_client_stream;
}
