/* version.c - version number library */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include "version.h"

void show_version(const char *name, const struct version *ver)
{
	fprintf(stdout,
		"\n%s"
		" %4d.%d.%d.%3.3d, (c) Wind River Systems, Inc.\n"
		" Built on %s at %s\n\n",
		name,
		ver->kernel, ver->major, ver->minor, ver->maintenance,
		__DATE__, __TIME__);
}
