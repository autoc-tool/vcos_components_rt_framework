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
#include <nuttx/config.h>
#include <nuttx/sched.h>
#include <nuttx/event.h>
#include "rtfw_interrupt.h"
#include "rtfw_task.h"
#include "rtfw_core.h"
#include "rtfw_lock_internal.h"
#include "rtfw_event_internal.h"

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(os_status_t, OS_CODE) os_wait_event(os_event_mask_t mask)
{
	nxevent_mask_t ret;
	boolean check_ret;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	const os_task_cfg_t *task_cfg = OS_CONTEXT_GET_TASK_CFG_REF(cur_context);

	check_ret = os_context_check_curcontext_id(cur_context, OS_CONTEXT_WAITEVENT);
	if (UNLIKELY(FALSE == check_ret)) {
		return OS_STATUS_CALLEVEL;
	}
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	check_ret = os_interrupt_is_disabled();
	if (UNLIKELY(TRUE == check_ret)) {
		return OS_STATUS_DISABLEDINT;
	}
	if (UNLIKELY(NULL_PTR == task_cfg->event_hdl)) {
		return OS_STATUS_NOT_EXTENDED_TASK;
	}

	os_status_t lock_status = os_context_check_locklist(cur_context);

	if (UNLIKELY(lock_status != OS_STATUS_OK)) {
		return lock_status;
	}
#endif

	os_task_event_t *task_event = task_cfg->event_hdl;

	ret = nxevent_tickwait(&(task_event->wait), &(task_event->event), mask, NXEVENT_WAIT_NOCLEAR, UINT32_MAX);
	/* returns 0 when events were not received */
	if (UNLIKELY(ret == (nxevent_mask_t)0)) {
		return OS_STATUS_NOT_OK;
	}

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) rtfw_event_tickwait(os_event_mask_t mask, uint32_t ticks_to_wait)
{
	nxevent_mask_t ret;
	boolean check_ret;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	const os_task_cfg_t *task_cfg = OS_CONTEXT_GET_TASK_CFG_REF(cur_context);

	check_ret = os_context_check_curcontext_id(cur_context, OS_CONTEXT_EVENT_TICKWAIT);
	if (UNLIKELY(FALSE == check_ret)) {
		return OS_STATUS_CALLEVEL;
	}

#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	check_ret = os_interrupt_is_disabled();
	if (UNLIKELY(TRUE == check_ret)) {
		return OS_STATUS_DISABLEDINT;
	}
	if (UNLIKELY(NULL_PTR == task_cfg->event_hdl)) {
		return OS_STATUS_NOT_EXTENDED_TASK;
	}

	os_status_t lock_status = os_context_check_locklist(cur_context);

	if (UNLIKELY(lock_status != OS_STATUS_OK)) {
		return lock_status;
	}
#endif

	os_task_event_t *task_event = task_cfg->event_hdl;

	ret = nxevent_tickwait(&(task_event->wait), &(task_event->event), mask, NXEVENT_WAIT_NOCLEAR, ticks_to_wait);
	/* returns 0 when events were not received or timeout occurred */
	if (UNLIKELY(ret == (nxevent_mask_t)0)) {
		return OS_STATUS_TIMEOUT;
	}

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_set_event(TaskType task_id, os_event_mask_t mask)
{
	boolean check_ret;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();

	check_ret = os_context_check_curcontext_id(cur_context, OS_CONTEXT_SETEVENT);
	if (UNLIKELY(FALSE == check_ret)) {
		return OS_STATUS_CALLEVEL;
	}
	check_ret = os_interrupt_is_disabled();
	if (UNLIKELY(TRUE == check_ret)) {
		return OS_STATUS_DISABLEDINT;
	}
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY((task_id >= os_task_cfg_ref_table_size) || (NULL == os_task_cfg_ref_table[task_id]))) {
		return OS_STATUS_ID_1;
	}
#endif
	const os_task_cfg_t *task_cfg = os_task_cfg_ref_table[task_id];

#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(NULL_PTR == task_cfg->event_hdl)) {
		return OS_STATUS_NOT_EXTENDED_TASK;
	}
#endif
	const os_ctx_cfg_t *task_context = &(task_cfg->context);
	os_task_event_t *task_event = task_cfg->event_hdl;
	CoreIdType task_core_id = OS_CONTEXT_GET_OWNER_COREID(task_context);

	if (task_core_id == os_get_core_id()) {
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
		os_task_inst_t *task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(task_context);

		if (UNLIKELY(task_inst->thread.tcb.flags & TCB_FLAG_THREAD_IS_SUSPENDED)) {
			return OS_STATUS_STOPPED;
		} else {
			return (nxevent_post(&(task_event->event), mask, 0) == 0 ? OS_STATUS_OK : OS_STATUS_NOT_OK);
		}
#else
		return (nxevent_post(&(task_event->event), mask, 0) == 0 ? OS_STATUS_OK : OS_STATUS_NOT_OK);
#endif
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_set_event((uint32)task_core_id, task_id, mask);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_set_event_asyn(TaskType task_id, os_event_mask_t mask)
{
	const os_task_cfg_t *task_cfg;
	const os_ctx_cfg_t *task_context;
	os_task_event_t *task_event;
	CoreIdType task_core_id;

	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	task_cfg = os_task_cfg_ref_table[task_id];

	if (UNLIKELY(task_cfg->event_hdl == NULL_PTR)) {
		return OS_STATUS_NOT_EXTENDED_TASK;
	}

	task_context = &(task_cfg->context);
	task_event = task_cfg->event_hdl;
	task_core_id = OS_CONTEXT_GET_OWNER_COREID(task_context);

	if (task_core_id == os_get_core_id()) {
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
		os_task_inst_t *task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(task_context);

		if (UNLIKELY(task_inst->thread.tcb.flags & TCB_FLAG_THREAD_IS_SUSPENDED)) {
			return OS_STATUS_STOPPED;
		} else {
			return (nxevent_post(&(task_event->event), mask, 0) == 0 ? OS_STATUS_OK : OS_STATUS_NOT_OK);
		}
#else
	return (nxevent_post(&(task_event->event), mask, 0) == 0 ? OS_STATUS_OK : OS_STATUS_NOT_OK);
#endif
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_set_event_asyn((uint32)task_core_id, task_id, mask);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_get_event(TaskType task_id, os_event_mask_t *mask)
{
	boolean check_ret;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();

	check_ret = os_context_check_curcontext_id(cur_context, OS_CONTEXT_GETEVENT);
	if (UNLIKELY(FALSE == check_ret)) {
		return OS_STATUS_CALLEVEL;
	}
	check_ret = os_interrupt_is_disabled();
	if (UNLIKELY(TRUE == check_ret)) {
		return OS_STATUS_DISABLEDINT;
	}
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY((task_id >= os_task_cfg_ref_table_size) || (NULL == os_task_cfg_ref_table[task_id]))) {
		return OS_STATUS_ID_1;
	}
#endif
	if (UNLIKELY(NULL == mask)) {
		return OS_STATUS_PARAM_POINTER_2;
	}
	const os_task_cfg_t *task_cfg = os_task_cfg_ref_table[task_id];

#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(NULL_PTR == task_cfg->event_hdl)) {
		return OS_STATUS_NOT_EXTENDED_TASK;
	}
#endif

	const os_ctx_cfg_t *task_context = &(task_cfg->context);
	os_task_event_t *task_event = task_cfg->event_hdl;
	CoreIdType task_core_id = OS_CONTEXT_GET_OWNER_COREID(task_context);

	if (task_core_id == os_get_core_id()) {
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
		os_task_inst_t *task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(task_context);

		if (UNLIKELY(task_inst->thread.tcb.flags & TCB_FLAG_THREAD_IS_SUSPENDED)) {
			return OS_STATUS_STOPPED;
		} else {
			*mask = nxevent_getmask(&(task_event->event));
		}
#else
	*mask = nxevent_getmask(&(task_event->event));
#endif
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_get_event((uint32)task_core_id, task_id, mask);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_clear_event(os_event_mask_t mask)
{
	boolean check_ret;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	const os_task_cfg_t *task_cfg = OS_CONTEXT_GET_TASK_CFG_REF(cur_context);

	check_ret = os_context_check_curcontext_id(cur_context, OS_CONTEXT_CLEAREVENT);
	if (UNLIKELY(FALSE == check_ret)) {
		return OS_STATUS_CALLEVEL;
	}
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	check_ret = os_interrupt_is_disabled();
	if (UNLIKELY(TRUE == check_ret)) {
		return OS_STATUS_DISABLEDINT;
	}
	if (UNLIKELY(NULL_PTR == task_cfg->event_hdl)) {
		return OS_STATUS_NOT_EXTENDED_TASK;
	}
#endif
	(void)nxevent_clear(&(task_cfg->event_hdl->event), mask);

	return OS_STATUS_OK;
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
