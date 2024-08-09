/*
 * Copyright (c) 2023 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 * From: https://gist.github.com/alan-mushi/19546a0e2c6bd4e059fd
 */

#include <json.h>
#include <stdio.h>

int main(void)
{
	const char *question = "Mum, clouds hide alien spaceships don't they ?";
	const char *answer = "Of course not! (sigh)";
	const char *json_str;
	struct json_object *jobj;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "question", json_object_new_string(question));
	json_object_object_add(jobj, "answer", json_object_new_string(answer));

	json_str = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);
	printf("%s\n", json_str);

	json_object_put(jobj);

	return 0;
}
