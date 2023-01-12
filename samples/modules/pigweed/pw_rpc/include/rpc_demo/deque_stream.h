/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <pw_stream/stream.h>
#include <pw_sync/mutex.h>

#include <deque>

class DequeReadWriter : public pw::stream::NonSeekableReaderWriter {
 public:
  DequeReadWriter() = default;

 protected:
  pw::StatusWithSize DoRead(pw::ByteSpan destination) override;
  pw::Status DoWrite(pw::ConstByteSpan data) override;

 private:
  std::deque<std::byte> buff_;
  pw::sync::Mutex mutex_;
};
