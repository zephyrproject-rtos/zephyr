/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Overall HW models scheduler for the native simulator
 *
 * Models events are registered with NSI_HW_EVENT().
 */

#include <stdint.h>
#include <signal.h>
#include <stddef.h>
#include <inttypes.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <string.h>
#endif
#include "nsi_tracing.h"
#include "nsi_main.h"
#include "nsi_safe_call.h"
#include "nsi_hw_scheduler.h"
#include "nsi_hws_models_if.h"

uint64_t nsi_simu_time; /* The actual time as known by the HW models */
static uint64_t end_of_time = NSI_NEVER; /* When will this device stop */
static unsigned int number_of_events;

#ifdef __APPLE__
extern const struct mach_header_64 _mh_execute_header;

struct macho_nsi_hw_event_section {
	const struct nsi_hw_event_st *start;
	size_t count;
	unsigned int prio;
};

static const struct nsi_hw_event_st *macho_nsi_hw_events[64];

static unsigned int macho_parse_nsi_hw_event_prio(const char *str)
{
	unsigned int value = 0U;

	while ((*str >= '0') && (*str <= '9')) {
		value = (value * 10U) + (unsigned int)(*str - '0');
		str++;
	}

	return value;
}

static size_t macho_collect_nsi_hw_event_sections(struct macho_nsi_hw_event_section *sections,
						  size_t max_sections)
{
	static const char prefix[] = "nsihwe_";
	const struct mach_header_64 *hdr = &_mh_execute_header;
	const struct load_command *lc =
		(const struct load_command *)((const char *)hdr + sizeof(*hdr));
	const intptr_t slide = _dyld_get_image_vmaddr_slide(0);
	const size_t prefix_len = strlen(prefix);
	size_t count = 0U;

	for (uint32_t i = 0U; i < hdr->ncmds; i++) {
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *seg =
				(const struct segment_command_64 *)lc;
			const struct section_64 *sec =
				(const struct section_64 *)(seg + 1);

			if (strncmp(seg->segname, "__DATA", sizeof(seg->segname)) == 0) {
				for (uint32_t j = 0U; (j < seg->nsects) && (count < max_sections);
				     j++, sec++) {
					char sectname[sizeof(sec->sectname) + 1];

					memcpy(sectname, sec->sectname, sizeof(sec->sectname));
					sectname[sizeof(sec->sectname)] = '\0';

					if (strncmp(sectname, prefix, prefix_len) != 0) {
						continue;
					}

					sections[count].start =
						(const struct nsi_hw_event_st *)(uintptr_t)
							(sec->addr + slide);
					sections[count].count =
						sec->size / sizeof(struct nsi_hw_event_st);
					sections[count].prio =
						macho_parse_nsi_hw_event_prio(
							sectname + prefix_len);
					count++;
				}
			}
		}

		lc = (const struct load_command *)((const char *)lc + lc->cmdsize);
	}

	return count;
}

static void macho_sort_nsi_hw_event_sections(struct macho_nsi_hw_event_section *sections,
					     size_t count)
{
	for (size_t i = 1U; i < count; i++) {
		struct macho_nsi_hw_event_section key = sections[i];
		size_t j = i;

		while ((j > 0U) && (sections[j - 1U].prio > key.prio)) {
			sections[j] = sections[j - 1U];
			j--;
		}

		sections[j] = key;
	}
}

static void macho_init_nsi_hw_events(void)
{
	struct macho_nsi_hw_event_section sections[64];
	size_t section_count = macho_collect_nsi_hw_event_sections(
		sections, NSI_ARRAY_SIZE(sections));
	size_t event_count = 0U;

	macho_sort_nsi_hw_event_sections(sections, section_count);

	for (size_t i = 0U; i < section_count; i++) {
		if (sections[i].count >
		    (NSI_ARRAY_SIZE(macho_nsi_hw_events) - event_count)) {
			nsi_print_error_and_exit("Too many NSI HW events in Mach-O image\n");
		}

		for (size_t j = 0U; j < sections[i].count; j++) {
			macho_nsi_hw_events[event_count++] = &sections[i].start[j];
		}
	}

	number_of_events = (unsigned int)event_count;
}

static const struct nsi_hw_event_st *nsi_hws_get_event(unsigned int index)
{
	return macho_nsi_hw_events[index];
}
#else
extern struct nsi_hw_event_st __nsi_hw_events_start[];
extern struct nsi_hw_event_st __nsi_hw_events_end[];

static const struct nsi_hw_event_st *nsi_hws_get_event(unsigned int index)
{
	return &__nsi_hw_events_start[index];
}
#endif

static unsigned int next_timer_index;
static uint64_t next_timer_time;

/* Have we received a SIGTERM or SIGINT */
static volatile sig_atomic_t signaled_end;

/**
 * Handler for SIGTERM and SIGINT
 */
static void nsi_hws_signal_end_handler(int sig)
{
	signaled_end = 1;
}

/**
 * Set the handler for SIGTERM and SIGINT which will cause the
 * program to exit gracefully when they are received the 1st time
 *
 * Note that our handler only sets a variable indicating the signal was
 * received, and in each iteration of the hw main loop this variable is
 * evaluated.
 * If for some reason (the program is stuck) we never evaluate it, the program
 * would never exit.
 * Therefore we set SA_RESETHAND: This way, the 2nd time the signal is received
 * the default handler would be called to terminate the program no matter what.
 *
 * Note that SA_RESETHAND requires either _POSIX_C_SOURCE>=200809L or
 * _XOPEN_SOURCE>=500
 */
static void nsi_hws_set_sig_handler(void)
{
	struct sigaction act;

	act.sa_handler = nsi_hws_signal_end_handler;
	NSI_SAFE_CALL(sigemptyset(&act.sa_mask));

	act.sa_flags = SA_RESETHAND;

	NSI_SAFE_CALL(sigaction(SIGTERM, &act, NULL));
	NSI_SAFE_CALL(sigaction(SIGINT, &act, NULL));
}


static void nsi_hws_sleep_until_next_event(void)
{
	if (next_timer_time >= nsi_simu_time) { /* LCOV_EXCL_BR_LINE */
		nsi_simu_time = next_timer_time;
	} else {
		/* LCOV_EXCL_START */
		nsi_print_warning("next_timer_time corrupted (%"PRIu64"<= %"
				PRIu64", timer idx=%i)\n",
				(uint64_t)next_timer_time,
				(uint64_t)nsi_simu_time,
				next_timer_index);
		/* LCOV_EXCL_STOP */
	}

	if (signaled_end || (nsi_simu_time >= end_of_time)) {
		nsi_print_trace("\nStopped at %.3Lfs\n",
				((long double)nsi_simu_time)/1.0e6L);
		nsi_exit(0);
	}
}


/**
 * Find in between all events timers which is the next one.
 * (and update the internal next_timer_* accordingly)
 */
void nsi_hws_find_next_event(void)
{
	const struct nsi_hw_event_st *event;

	if (number_of_events == 0U) {
		return;
	}

	next_timer_index = 0;
	event = nsi_hws_get_event(0U);
	next_timer_time = *event->timer;

	for (unsigned int i = 1U; i < number_of_events; i++) {
		event = nsi_hws_get_event(i);
		if (next_timer_time > *event->timer) {
			next_timer_index = i;
			next_timer_time = *event->timer;
		}
	}
}

uint64_t nsi_hws_get_next_event_time(void)
{
	return next_timer_time;
}

/**
 * Execute the next scheduled HW event
 * (advancing time until that event would trigger)
 */
void nsi_hws_one_event(void)
{
	nsi_hws_sleep_until_next_event();

	if (next_timer_index < number_of_events) { /* LCOV_EXCL_BR_LINE */
		nsi_hws_get_event(next_timer_index)->callback();
	} else {
		nsi_print_error_and_exit("next_timer_index corrupted\n"); /* LCOV_EXCL_LINE */
	}

	nsi_hws_find_next_event();
}

/**
 * Set the simulated time when the process will stop
 */
void nsi_hws_set_end_of_time(uint64_t new_end_of_time)
{
	end_of_time = new_end_of_time;
}

/**
 * Function to initialize the HW scheduler
 *
 * Note that the HW models should register their initialization functions
 * as NSI_TASKS of HW_INIT level.
 */
void nsi_hws_init(void)
{
#ifdef __APPLE__
	macho_init_nsi_hw_events();
#else
	number_of_events = __nsi_hw_events_end - __nsi_hw_events_start;
#endif

	nsi_hws_set_sig_handler();
	nsi_hws_find_next_event();
}

/**
 * Function to free any resources allocated by the HW scheduler
 *
 * Note that the HW models should register their initialization functions
 * as NSI_TASKS of ON_EXIT_PRE/POST levels.
 */
void nsi_hws_cleanup(void)
{
	/* Nothing to be done so far */
}
