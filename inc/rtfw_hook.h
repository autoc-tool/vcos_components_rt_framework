/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 * Licensed under the Apache License, Version 2.0(the "License");
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

#ifndef RT_FRAMEWORK_INC_OS_HOOK_H
#define RT_FRAMEWORK_INC_OS_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif
#include <nuttx/config.h>
#include "rtfw_hook_internal.h"
#include "rtfw_thread_types.h"

#define DeclareHook(section_const, section_var, hook_id)                                                               \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_hook_inst_t, hook_id);                                           \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_hook, const os_hook_cfg_t, hook_id)

#define HOOKCFGVAR(hook_id) RTFW_LINK_ENTRY_NAME(os_hook, hook_id)
#define HOOKOBJVAR(hook_id) RTFW_LINK_ENTRY_NAME(os_obj, hook_id)

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_HOOK_H */