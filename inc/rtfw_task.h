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

#ifndef RT_FRAMEWORK_INC_OS_TASK_H
#define RT_FRAMEWORK_INC_OS_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_task_internal.h"
#include "rtfw_thread_types.h"

/*! Task stack declare */

/*! Task declare */
#define DeclareBasicTask(section_const, section_var, task_id)                           \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_task_inst_t, task_id);                \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_task, const os_task_cfg_t, task_id)

#define DeclareExtendedTask(section_const, section_var, task_id)                        \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_eventhdl, os_task_event_t, task_id);      \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_task_inst_t, task_id);                \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_task, const os_task_cfg_t, task_id)

#if defined(CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK == 1)
#define DeclareTaskShareStackOwner(section_var, comb_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_task_share_stack_owner, struct tcb_s*, comb_id)

#define DeclareTaskShareStackCFG(section_const, section_var, task_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_task_temp_stack, task_temp_buffer_t, task_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_task_share_stack_cfg, const task_share_stack_info_t, task_id)

#define TASKTEMPSTACKVAR(task_id) RTFW_LINK_ENTRY_NAME(os_task_temp_stack, task_id)
#define TASKSHARESTACKOWNERVAR(comb_id) RTFW_LINK_ENTRY_NAME(os_task_share_stack_owner, comb_id)
#define TASKSHARESTACKCFGVAR(task_id) RTFW_LINK_ENTRY_NAME(os_task_share_stack_cfg, task_id)
#endif

#define TASKCFGVAR(task_id) RTFW_LINK_ENTRY_NAME(os_task, task_id)
#define TASKOBJVAR(task_id) RTFW_LINK_ENTRY_NAME(os_obj, task_id)
#define TASKEVENTOBJVAR(task_id) RTFW_LINK_ENTRY_NAME(os_obj_eventhdl, task_id)

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
extern FUNC(os_status_t, OS_CODE) get_task_app_id(ApplicationIDType *appid_ref);
#endif
extern FUNC(os_status_t, OS_CODE) rtfw_task_tickdelay(TickType ticks_to_delay);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /*  RT_FRAMEWORK_INC_OS_TASK_H */
