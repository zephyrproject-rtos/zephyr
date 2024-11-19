# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(thread_metric)

FILE(GLOB app_sources src/tm_porting_layer_zephyr.c)
target_sources(app PRIVATE ${app_sources})
target_sources_ifdef(CONFIG_TM_BASIC                 app PRIVATE src/tm_basic_processing_test.c)
target_sources_ifdef(CONFIG_TM_COOPERATIVE           app PRIVATE src/tm_cooperative_scheduling_test.c)
target_sources_ifdef(CONFIG_TM_INTERRUPT             app PRIVATE src/tm_interrupt_processing_test.c)
target_sources_ifdef(CONFIG_TM_INTERRUPT_PREEMPTION  app PRIVATE src/tm_interrupt_preemption_processing_test.c)
target_sources_ifdef(CONFIG_TM_MEMORY_ALLOCATION     app PRIVATE src/tm_memory_allocation_test.c)
target_sources_ifdef(CONFIG_TM_MESSAGE               app PRIVATE src/tm_message_processing_test.c)
target_sources_ifdef(CONFIG_TM_PREEMPTIVE            app PRIVATE src/tm_preemptive_scheduling_test.c)
target_sources_ifdef(CONFIG_TM_SYNCHRONIZATION       app PRIVATE src/tm_synchronization_processing_test.c)
