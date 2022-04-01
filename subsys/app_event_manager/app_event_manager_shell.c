/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <shell/shell.h>
#include <app_event_manager/app_event_manager.h>

static int show_events(const struct shell *event_manager_shell, size_t argc,
		char **argv)
{
	shell_fprintf(event_manager_shell, SHELL_NORMAL,
		      "Registered Events:\n");

	for (const struct event_type *et = _event_type_list_start;
	     (et != NULL) && (et != _event_type_list_end); et++) {

		size_t ev_id = et - _event_type_list_start;

		shell_fprintf(event_manager_shell,
			      SHELL_NORMAL,
			      "%c %d:\t%s\n",
			      (atomic_test_bit(_app_event_manager_event_display_bm.flags, ev_id)) ?
				'E' : 'D',
			      ev_id,
			      et->name);
	}

	return 0;
}

static int show_listeners(const struct shell *event_manager_shell, size_t argc,
		char **argv)
{
	shell_fprintf(event_manager_shell, SHELL_NORMAL, "Registered Listeners:\n");

	STRUCT_SECTION_FOREACH(event_listener, el) {
		__ASSERT_NO_MSG(el != NULL);
		shell_fprintf(event_manager_shell, SHELL_NORMAL, "|\t[L:%s]\n", el->name);
	}

	return 0;
}

static int show_subscribers(const struct shell *event_manager_shell, size_t argc,
		char **argv)
{
	shell_fprintf(event_manager_shell, SHELL_NORMAL, "Registered Subscribers:\n");

	STRUCT_SECTION_FOREACH(event_type, et) {
		bool is_subscribed = false;

		for (const struct event_subscriber *es =
				et->subs_start;
			es != et->subs_stop;
			es++) {

			__ASSERT_NO_MSG(es != NULL);
			const struct event_listener *el = es->listener;

			__ASSERT_NO_MSG(el != NULL);
			shell_fprintf(event_manager_shell, SHELL_NORMAL,
					"|\t[E:%s] -> [L:%s]\n",
				et->name, el->name);

			is_subscribed = true;
		}


		if (!is_subscribed) {
			shell_fprintf(event_manager_shell, SHELL_NORMAL,
				      "|\t[E:%s] has no subscribers\n",
				      et->name);
		}
		shell_fprintf(event_manager_shell, SHELL_NORMAL, "\n");
	}

	return 0;
}

static void set_event_displaying(const struct shell *event_manager_shell, size_t argc,
				 char **argv, bool enable)
{
	/* If no IDs specified, all registered events are affected */
	if (argc == 1) {
		for (const struct event_type *et = _event_type_list_start;
		     (et != NULL) && (et != _event_type_list_end);
		     et++) {

			size_t ev_id = et - _event_type_list_start;

			atomic_set_bit_to(_app_event_manager_event_display_bm.flags, ev_id, enable);
		}

		shell_fprintf(event_manager_shell,
			      SHELL_NORMAL,
			      "Displaying all events %sabled\n",
			      enable ? "en":"dis");
	} else {
		int event_indexes[argc - 1];

		for (size_t i = 0; i < ARRAY_SIZE(event_indexes); i++) {
			char *end;

			event_indexes[i] = strtol(argv[i + 1], &end, 10);

			if ((event_indexes[i] < 0)
			    || (event_indexes[i] >= _event_type_list_end - _event_type_list_start)
			    || (*end != '\0')) {

				shell_error(event_manager_shell, "Invalid event ID: %s",
					    argv[i + 1]);
				return;
			}
		}

		for (size_t i = 0; i < ARRAY_SIZE(event_indexes); i++) {
			atomic_set_bit_to(_app_event_manager_event_display_bm.flags,
					  event_indexes[i],
					  enable);
			const struct event_type *et =
				_event_type_list_start + event_indexes[i];
			const char *event_name = et->name;

			shell_fprintf(event_manager_shell,
				      SHELL_NORMAL,
				      "Displaying event %s %sabled\n",
				      event_name,
				      enable ? "en":"dis");
		}
	}
}

static int enable_event_displaying(const struct shell *event_manager_shell, size_t argc,
				   char **argv)
{
	set_event_displaying(event_manager_shell, argc, argv, true);
	return 0;
}

static int disable_event_displaying(const struct shell *event_manager_shell,
					size_t argc, char **argv)
{
	set_event_displaying(event_manager_shell, argc, argv, false);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_app_event_manager,
	SHELL_CMD_ARG(show_listeners, NULL, "Show listeners",
		      show_listeners, 0, 0),
	SHELL_CMD_ARG(show_subscribers, NULL, "Show subscribers",
		      show_subscribers, 0, 0),
	SHELL_CMD_ARG(show_events, NULL, "Show events", show_events, 0, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable displaying event with given ID",
		      disable_event_displaying, 0,
		      sizeof(_app_event_manager_event_display_bm) * 8 - 1),
	SHELL_CMD_ARG(enable, NULL, "Enable displaying event with given ID",
		      enable_event_displaying, 0,
		      sizeof(_app_event_manager_event_display_bm) * 8 - 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(app_event_manager, &sub_app_event_manager,
		   "Application Event Manager commands", NULL);
