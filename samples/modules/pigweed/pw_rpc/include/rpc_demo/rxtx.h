/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <pw_stream/stream.h>

/**
 * @return The ReaderWriter stream used for client-to-service communication.
 */
pw::stream::ReaderWriter& ClientToServiceStream();

/**
 * @return The ReaderWriter stream used for service-to-client communication.
 */
pw::stream::ReaderWriter& ServiceToClientStream();
