/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

void main(void)
{
	/* Quark: this is an x86 stub to just run ARC; it does nothing
	 * but start the x86 kernel so it will pass messages from the
	 * ARC processor via the IPM module.
	 *
	 * There is a solution planned for the future that will allow
	 * this workaround to be removed (without need to support
	 * backwards compatibility). */
}
