#include <zephyr/kernel.h>

#include "pw_unit_test/framework.h"
#include "pw_unit_test/logging_event_handler.h"

int main() {
  testing::InitGoogleTest(nullptr, nullptr);
  pw::unit_test::LoggingEventHandler handler;
  pw::unit_test::RegisterEventHandler(&handler);
  return RUN_ALL_TESTS();
}
