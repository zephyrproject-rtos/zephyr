/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <pw_hdlc/rpc_channel.h>
#include <pw_span/span.h>
#include <pw_stream/stream.h>

#include <atomic>
#include <thread>

#include "rpc_demo/service/demo_service.h"

namespace rpc_demo {

class ServiceReader {
 public:
  ServiceReader(pw::stream::Reader* reader,
                pw::stream::Writer* writer,
                pw::hdlc::RpcChannelOutput* output,
                pw::span<pw::rpc::Channel> channels);

  bool ParsePacket();

 private:
  pw::stream::Reader* reader_;
  pw::stream::Writer* writer_;
  pw::hdlc::RpcChannelOutput* output_;
  pw::span<pw::rpc::Channel> channels_;
};

}  // namespace rpc_demo
