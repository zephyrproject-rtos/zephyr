/* Copyright (c) 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SAMPLES_APPLICATION_DEVELOPMENT_CHRE_INCLUDE_APPS_H_
#define SAMPLES_APPLICATION_DEVELOPMENT_CHRE_INCLUDE_APPS_H_

#include "chre/core/nanoapp.h"
#include "chre/util/unique_ptr.h"

namespace chre {

UniquePtr<Nanoapp> initializeStaticNanoappEchoApp();

}  /* namespace chre */

#endif /* SAMPLES_APPLICATION_DEVELOPMENT_CHRE_INCLUDE_APPS_H_ */
