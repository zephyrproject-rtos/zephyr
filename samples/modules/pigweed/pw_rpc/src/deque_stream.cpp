/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mutex>
#include "rpc_demo/deque_stream.h"

pw::StatusWithSize DequeReadWriter::DoRead(pw::ByteSpan destination) {
  size_t count = 0;
  std::lock_guard lock(mutex_);

  while (!buff_.empty() && count < destination.size_bytes()) {
    destination[count++] = buff_.front();
    buff_.pop_front();
  }

  auto status = (count == 0) ? pw::Status::OutOfRange() : pw::OkStatus();
  return pw::StatusWithSize(status, count);
}

pw::Status DequeReadWriter::DoWrite(pw::ConstByteSpan data) {
  std::lock_guard lock(mutex_);
  for (size_t i = 0; i < data.size_bytes(); ++i) {
    buff_.push_back(data[i]);
  }

  return pw::OkStatus();
}
