/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <pw_hdlc/rpc_channel.h>
#include <pw_hdlc/rpc_packets.h>
#include <pw_rpc/client.h>
#include <pw_stream/stream.h>

#include <array>
#include <atomic>

namespace rpc_demo {

class ClientReader {
 public:
  ClientReader(pw::stream::Reader& rx, pw::rpc::Client* rpc_client);
  bool ParsePacket();

 private:
  pw::stream::Reader& rx_;
  pw::rpc::Client* rpc_client_;
  std::array<std::byte, CONFIG_RPC_BUFFER_SIZE> decode_buffer_;
  pw::hdlc::Decoder decoder_;
};

}  // namespace rpc_demo
