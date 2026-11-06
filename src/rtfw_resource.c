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

/* Including File */
#include "Os.h"
#include "rtfw_resource.h"
#include "rtfw_context_internal.h"
#include "rtfw_task.h"
#include "rtfw_lock_internal.h"
#include "sched.h"

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
FUNC(void, OS_CODE) os_resource_init(const os_resource_cfg_t * const os_resource_cfg_table[], OsObjIndexType size)
{
	const os_resource_cfg_t *resourcetp;
	os_resource_inst_t *resource_inst;
	OsObjIndexType index;

	if ((os_resource_cfg_table == NULL_PTR) || (size == (OsObjIndexType)0U)) {
		return;
	}

	for (index = 0U; index < size; index++) {
		resourcetp = os_resource_cfg_table[index];
		resource_inst = os_resource_cfg_table[index]->obj_desc;
		resource_inst->resource_id = resourcetp->resource_id;
		resource_inst->lock.type = OS_LOCK_TYPE_RESOURCE;
		resource_inst->is_occupied = FALSE;
	}
}

FUNC(os_status_t, OS_CODE) os_get_resource(ResourceType resource_id)
{
	const os_resource_cfg_t *resource_cfg;
	os_resource_inst_t *resource_inst;
	uint32 current_priority;
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();
	const os_task_cfg_t *task_cfg = OS_CONTEXT_GET_TASK_CFG_REF(current_context);
	struct sched_param param;
	irqstate_t flags;

	if (os_context_check_curcontext_id(current_context, OS_CONTEXT_GETRESOURCE) == FALSE) {
		return OS_STATUS_CALLEVEL;
	}
	if (os_interrupt_is_disabled() == TRUE) {
		return OS_STATUS_DISABLEDINT;
	}

	if (resource_id >= os_resource_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	resource_cfg = os_resource_cfg_ref_table[resource_id];

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if (UNLIKELY(os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(current_context),
	resource_cfg->access_app_mask) != OS_STATUS_OK)) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	resource_inst = resource_cfg->obj_desc;
	if (resource_inst->is_occupied == TRUE) {
		return OS_STATUS_LOCKED;
	}

	/* Check if the original task priority is less than the resource priority */
	if (task_cfg->base_priority >= resource_cfg->ceiling_priority) {
		return OS_STATUS_PRIORITY;
	}

	/* Scheduling point, depends on this interface for scheduling, do not modify */
	sched_lock();
	flags = enter_critical_section();

	/* Insert the current lock node into the task locklist */
	os_list_append(OS_CONTEXT_GET_LOCKLIST_REF(current_context), &(resource_inst->lock.lock_node));
	/* Mark resource as occupied */
	resource_inst->is_occupied = TRUE;

	/* Get current task priority */
	_SCHED_GETPARAM(0, &param);
	current_priority = param.sched_priority;

	/* Save the current task priority */
	resource_inst->previous_priority = current_priority;
	if (resource_cfg->ceiling_priority > current_priority) {
		/* Only raise priority when resource priority is higher than current task priority */
		param.sched_priority = resource_cfg->ceiling_priority;
		_SCHED_SETPARAM(0, &param);
	}

	/* Scheduling point, depends on this interface for scheduling, do not modify */
	leave_critical_section(flags);
	sched_unlock();

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_release_resource(ResourceType resource_id)
{
	const os_resource_cfg_t *resource_cfg;
	os_resource_inst_t *resource_inst;
	uint32 current_priority;
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();
	struct sched_param param;
	irqstate_t flags;

	if (os_context_check_curcontext_id(current_context, OS_CONTEXT_RELEASERESOURCE) == FALSE) {
		return OS_STATUS_CALLEVEL;
	}
	if (os_interrupt_is_disabled() == TRUE) {
		return OS_STATUS_DISABLEDINT;
	}

	if (resource_id >= os_resource_cfg_ref_table_size) {
		return OS_STATUS_ID_1;
	}

	resource_cfg = os_resource_cfg_ref_table[resource_id];

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	if (UNLIKELY(os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(current_context),
	resource_cfg->access_app_mask) != OS_STATUS_OK)) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	resource_inst = resource_cfg->obj_desc;
	if (resource_inst->is_occupied == FALSE) {
		return OS_STATUS_UNLOCKED;
	}

	/* Check if the task locklist's last node equals this lock */
	if (OS_LIST_LAST(OS_CONTEXT_GET_LOCKLIST_REF(current_context)) != &(resource_inst->lock.lock_node)) {
		/* lock release does not follow LIFO */
		return OS_STATUS_ORDER;
	}

	/* Scheduling point, depends on this interface for scheduling, do not modify */
	sched_lock();
	flags = enter_critical_section();

	/* Remove the current node from the list */
	os_list_remove(OS_CONTEXT_GET_LOCKLIST_REF(current_context), &(resource_inst->lock.lock_node));
	resource_inst->is_occupied = FALSE;

	/* Get current priority */
	_SCHED_GETPARAM(0, &param);
	current_priority = param.sched_priority;

	if (resource_inst->previous_priority < current_priority) {
		/* Only restore priority when original priority is less than current task priority */
		param.sched_priority = resource_inst->previous_priority;
		_SCHED_SETPARAM(0, &param);
	}

	/* Scheduling point, depends on this interface for scheduling, do not modify */
	leave_critical_section(flags);
	sched_unlock();

	return OS_STATUS_OK;
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
