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
#include <nuttx/config.h>
#include <nuttx/irq.h>
#include "Os.h"
#include "rtfw_core.h"
#include "rtfw_task.h"
#include "rtfw_event.h"
#include "rtfw_interrupt.h"
#include "rtfw_error.h"
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "rtfw_schedtable.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
#include "rtfw_spinlock.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
#include "rtfw_resource.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
#include "rtfw_syscall_internal.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
#include "rtfw_counter.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
#include "rtfw_ioc.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
#include "rtfw_application.h"
#include "application_cfg.h"
#endif

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
static inline_function bool syscall_is_required(void)
{
	if (up_is_privilege_mode() == FALSE) {
		return TRUE;
	}

#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	if (up_get_mpu_set() != 0U) {
		return TRUE;
	}
#endif

	return FALSE;
}
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(CoreIdType, OS_CODE) GetCoreID(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		return os_get_core_id();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETCOREID;

		up_syscall(&syscall);

		return syscall.syscall_return_value.return_coreid_type;
	}
#endif
}

FUNC(StatusType, OS_CODE) ActivateTask(TaskType task_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_activate_task(task_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_ACTIVATETASK;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_ActivateTask, status, 1, (uintptr_t)task_id);
	return (StatusType)status;
}

FUNC(void, OS_CODE) ActivateTaskAsyn(TaskType task_id)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		(void)os_activate_task_asyn(task_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_ACTIVATETASKASYNC;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;

		up_syscall(&syscall);
	}
#endif
}

FUNC(StatusType, OS_CODE) TerminateTask(void)
{
	os_status_t status;

	status = os_terminate_task();

	REPORT_SERVICE_ERROR(OSServiceId_TerminateTask, status, 0);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ChainTask(TaskType task_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_chain_task(task_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_CHAINTASK;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	if (LIKELY(status == OS_STATUS_OK)) {
		status = os_terminate_task();
	}

	REPORT_SERVICE_ERROR(OSServiceId_ChainTask, status, 1, (uintptr_t)task_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetTaskID(TaskRefType task_id_ref)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_get_task_id(task_id_ref);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETTASKID;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_ref = task_id_ref;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_GetTaskID, status, 1, (uintptr_t)task_id_ref);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetApplicationState(ApplicationType app_id, ApplicationStateRefType app_state_ref)
{
	os_status_t status;
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_get_app_state(app_id, app_state_ref);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETAPPLICATIONSTATE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_app_t = app_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_app_state_ref = app_state_ref;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_GetApplicationState, status, 2, (uintptr_t)app_id, (uintptr_t)app_state_ref);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetTaskState(TaskType task_id, TaskStateRefType task_state_ref)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_get_task_state(task_id, task_state_ref);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETTASKSTATE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_task_state_ref = task_state_ref;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_GetTaskState, status, 2, (uintptr_t)task_id, (uintptr_t)task_state_ref);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) AllowAccess(void)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_allow_access();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_ALLOWACCESS;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_AllowAccess, status, 0);

	return (StatusType)status;
}

FUNC(ApplicationType, OS_CODE) GetApplicationID(void)
{
	ApplicationType app_id = OS_APPLICATION_CNT;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		app_id = os_get_app_id();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETAPPLICATIONID;

		up_syscall(&syscall);

		app_id = syscall.syscall_return_value.return_application_type;
	}
#endif

	return app_id;
}

FUNC(ApplicationType, OS_CODE)  GetCurrentApplicationID(void)
{
	ApplicationType app_id = OS_APPLICATION_CNT;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		app_id = os_get_current_app_id();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETCURRENTAPPLICATIONID;

		up_syscall(&syscall);

		app_id = syscall.syscall_return_value.return_application_type;
	}
#endif

	return app_id;
}

FUNC(StatusType, OS_CODE) Schedule(void)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_schedule();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SCHEDULE;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_Schedule, status, 0);

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) task_tickdelay(TickType ticks_to_delay)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = rtfw_task_tickdelay(ticks_to_delay);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_TASK_TICKDELAY;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_tick_t = ticks_to_delay;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_task_tickdelay, status, 1, (uintptr_t)ticks_to_delay);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) WaitEvent(EventMaskType mask)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_wait_event(mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_WAITEVENT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask = mask;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_WaitEvent, status, 1, (uintptr_t)mask);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) event_tickwait(EventMaskType mask, TickType ticks_to_wait)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = rtfw_event_tickwait(mask, ticks_to_wait);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_EVENT_TICKWAIT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask = mask;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_t = ticks_to_wait;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_event_tickwait, status, 2, (uintptr_t)mask, (uintptr_t)ticks_to_wait);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) SetEvent(TaskType task_id, EventMaskType mask)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_set_event(task_id, mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SETEVENT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask = mask;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_SetEvent, status, 2, (uintptr_t)task_id, (uintptr_t)mask);
	return (StatusType)status;
}

FUNC(void, OS_CODE) SetEventAsyn(TaskType task_id, EventMaskType mask)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		(void)os_set_event_asyn(task_id, mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SETEVENTASYN;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask = mask;

		up_syscall(&syscall);
	}
#endif
}

FUNC(StatusType, OS_CODE) GetEvent(TaskType task_id, EventMaskRefType mask)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_get_event(task_id, mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETEVENT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t = task_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask_ref = mask;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_GetEvent, status, 2, (uintptr_t)task_id, (uintptr_t)mask);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ClearEvent(EventMaskType mask)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_clear_event(mask);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_CLEAREVENT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask = mask;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_ClearEvent, status, 1, (uintptr_t)mask);
	return (StatusType)status;
}

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
FUNC(StatusType, OS_CODE) StartScheduleTableRel(ScheduleTableType schedtable_id, TickType Offset)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_start_schedule_table_rel(schedtable_id, Offset);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_STARTSCHEDULETABLEREL;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_t = Offset;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_StartScheduleTableRel, status, 2, (uintptr_t)schedtable_id, (uintptr_t)Offset);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) StartScheduleTableAbs(ScheduleTableType schedtable_id, TickType start)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_start_schedule_table_abs(schedtable_id, start);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_STARTSCHEDULETABLEABS;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_t = start;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_StartScheduleTableAbs, status, 2, (uintptr_t)schedtable_id, (uintptr_t)start);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) StartScheduleTableSynchron(ScheduleTableType schedtable_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_start_schedule_table_synchron(schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_STARTSCHEDULETABLESYNCHRON;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_StartScheduleTableSynchron, status, 1, (uintptr_t)schedtable_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) StopScheduleTable(ScheduleTableType schedtable_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_stop_schedule_table(schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_STOPSCHEDULETABLE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_StopScheduleTable, status, 1, (uintptr_t)schedtable_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) SetScheduleTableAppId(ScheduleTableType schedtable_id, ApplicationIDType app_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_set_schedule_table_app_id(schedtable_id, app_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SETSCHEDULETABLEAPPID;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_applicationid = app_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_SetScheduleTableAppId, status, 2, (uintptr_t)schedtable_id, (uintptr_t)app_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ClearScheduleTableAppId(ScheduleTableType schedtable_id, ApplicationIDType app_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_clear_schedule_table_app_id(schedtable_id, app_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_CLEARSCHEDULETABLEAPPID;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_applicationid = app_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_ClearScheduleTableAppId, status, 2, (uintptr_t)schedtable_id,
			     (uintptr_t)app_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) NextScheduleTable(ScheduleTableType curr_schedtable_id, ScheduleTableType next_schedtable_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_next_schedule_table(curr_schedtable_id, next_schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_NEXTSCHEDULETABLE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = curr_schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable = next_schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_NextScheduleTable, status, 2, (uintptr_t)curr_schedtable_id,
			     (uintptr_t)next_schedtable_id);

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE)
NextScheduleTableFromStop(ScheduleTableType curr_schedtable_id, ScheduleTableType next_schedtable_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_next_schedule_table_from_stop(curr_schedtable_id, next_schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_NEXTSCHEDULETABLEFROMSTOP;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = curr_schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable = next_schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_NextScheduleTableFromStop, status, 2, (uintptr_t)curr_schedtable_id,
			     (uintptr_t)next_schedtable_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) SyncScheduleTable(ScheduleTableType schedtable_id, TickType tick_value)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_sync_schedule_table(schedtable_id, tick_value);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SYNCSCHEDULETABLE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_t = tick_value;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_SyncScheduleTable, status, 2, (uintptr_t)schedtable_id, (uintptr_t)tick_value);

	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) DeviateScheduleTable(ScheduleTableType schedtable_id, DeviateType deviate_value)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_deviate_schedule_table(schedtable_id, deviate_value);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_DEVIATESCHEDULETABLE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_deviate = deviate_value;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_DeviateScheduleTable, status, 2, (uintptr_t)schedtable_id,
			     (uintptr_t)deviate_value);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetScheduleTableStatus(ScheduleTableType schedtable_id, ScheduleTableStatusRefType StatusRef)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_get_schedule_table_status(schedtable_id, StatusRef);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETSCHEDULETABLESTATUS;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable_status_ref = StatusRef;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_GetScheduleTableStatus, status, 2, (uintptr_t)schedtable_id,
			     (uintptr_t)StatusRef);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) SetScheduleTableAsync(ScheduleTableType schedtable_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_set_schedule_table_async(schedtable_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SETSCHEDULETABLEASYNC;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable = schedtable_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif

	REPORT_SERVICE_ERROR(OSServiceId_SetScheduleTableAsync, status, 1, (uintptr_t)schedtable_id);
	return (StatusType)status;
}

#endif /* CONFIG_RT_FRAMEWORK_SCHTBL */

#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
FUNC(StatusType, OS_CODE) GetSpinlock(SpinlockIdType spinlock_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_get_spinlock(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETSPINLOCK;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_GetSpinlock, status, 1, (uintptr_t)spinlock_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ReleaseSpinlock(SpinlockIdType spinlock_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_release_spinlock(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_RELEASESPINLOCK;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_ReleaseSpinlock, status, 1, (uintptr_t)spinlock_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) TryToGetSpinlock(SpinlockIdType spinlock_id, TryToGetSpinlockType *success)
{
	os_status_t status;
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_try_to_get_spinlock(spinlock_id, success);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_TRYTOGETSPINLOCK;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref = success;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_TryToGetSpinlock, status, 2, (uintptr_t)spinlock_id, (uintptr_t)success);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetSpinlockRead(SpinlockIdType spinlock_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_get_spinlock_read(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETSPINLOCKREAD;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_GetSpinlock, status, 1, (uintptr_t)spinlock_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetSpinlockWrite(SpinlockIdType spinlock_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_get_spinlock_write(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETSPINLOCKWRITE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_GetSpinlock, status, 1, (uintptr_t)spinlock_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ReleaseSpinlockRead(SpinlockIdType spinlock_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_release_spinlock_read(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_RELEASESPINLOCKREAD;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_ReleaseSpinlock, status, 1, (uintptr_t)spinlock_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ReleaseSpinlockWrite(SpinlockIdType spinlock_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_release_spinlock_write(spinlock_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_RELEASESPINLOCKWRITE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_ReleaseSpinlock, status, 1, (uintptr_t)spinlock_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) TryToGetSpinlockRead(SpinlockIdType spinlock_id, TryToGetSpinlockType *success)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_try_to_get_spinlock_read(spinlock_id, success);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_TRYTOGETSPINLOCKREAD;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref = success;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_TryToGetSpinlock, status, 2, (uintptr_t)spinlock_id, (uintptr_t)success);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) TryToGetSpinlockWrite(SpinlockIdType spinlock_id, TryToGetSpinlockType *success)
{
	os_status_t status;
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		status = os_try_to_get_spinlock_write(spinlock_id, success);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_TRYTOGETSPINLOCKWRITE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock = spinlock_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref = success;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_TryToGetSpinlock, status, 2, (uintptr_t)spinlock_id, (uintptr_t)success);
	return (StatusType)status;
}

#endif /* CONFIG_RT_FRAMEWORK_SPINLOCK */

FUNC(void, OS_CODE) DisableAllInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		os_disable_all_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_DISABLEALLINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) EnableAllInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		os_enable_all_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_ENABLEALLINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) SuspendAllInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		os_suspend_all_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SUSPENDALLINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) ResumeAllInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		os_resume_all_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_RESUMEALLINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) SuspendOSInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		os_suspend_os_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SUSPENDOSINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) ResumeOSInterrupts(void)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		os_resume_os_interrupts();
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_RESUMEOSINTERRUPTS;
		up_syscall(&syscall);
	}
#endif
}

#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
FUNC(StatusType, OS_CODE) GetResource(ResourceType resource_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_get_resource(resource_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETRESOURCE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_resource = resource_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_GetResource, status, 1, (uintptr_t)resource_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) ReleaseResource(ResourceType resource_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_release_resource(resource_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_RELEASERESOURCE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_resource = resource_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_ReleaseResource, status, 1, (uintptr_t)resource_id);
	return (StatusType)status;
}
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE */

#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
FUNC(StatusType, OS_CODE) GetCounterValue(CounterType counter_id, TickRefType tick_value)
{
	os_status_t status;
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_counter_get_value(counter_id, tick_value);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETCOUNTERVALUE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_counter_type = counter_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_ref_type = tick_value;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_GetCounterValue, status, 2, (uintptr_t)counter_id, (uintptr_t)tick_value);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) GetElapsedValue(CounterType counter_id, TickRefType pre_value, TickRefType elapsed_value)
{
	os_status_t status;
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_counter_get_elapsed_value(counter_id, pre_value, elapsed_value);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_GETELAPSEDVALUE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_counter_type = counter_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_ref_type = pre_value;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_3].param_tick_ref_type = elapsed_value;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_GetElapsedValue, status, 3, (uintptr_t)counter_id, (uintptr_t)pre_value,
			     (uintptr_t)elapsed_value);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) IncrementCounter(CounterType counter_id)
{
	os_status_t status;
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_counter_increment(counter_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_INCREMENTCOUNTER;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_counter_type = counter_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_IncrementCounter, status, 1, (uintptr_t)counter_id);
	return (StatusType)status;
}

#endif

#if defined(CONFIG_RT_FRAMEWORK_ERROR) && (CONFIG_RT_FRAMEWORK_ERROR == 1)
FUNC(void, OS_CODE) service_error_report(OSServiceIdType service_id, os_status_t error_status, uint32 param_cnt, ...)
{
	va_list args;

	va_start(args, param_cnt);

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		vservice_error_report(service_id, error_status, param_cnt, args);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SERVICEERRORREPORT;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_service_id = service_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_os_status = error_status;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_3].param_uint32_t = param_cnt;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_4].param_va_list = args;
		up_syscall(&syscall);
	}
#endif
	va_end(args);
}
#endif

#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
FUNC(StatusType, OS_CODE) IocSend(IocIdType ioc_id, void const *data)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_ioc_send(ioc_id, data);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_IOCSEND;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type = ioc_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_const_void_ref_type = data;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_IocSend, status, 2, (uintptr_t)ioc_id, (uintptr_t)data);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) IocReceive(IocIdType ioc_id, void *data)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_ioc_recv(ioc_id, data);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_IOCRECEIVE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type = ioc_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_void_ref_type = data;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_IocReceive, status, 2, (uintptr_t)ioc_id, (uintptr_t)data);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) IocEmptyQueue(IocIdType ioc_id)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_ioc_empty_queue(ioc_id);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_IOCEMPTYQUEUE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type = ioc_id;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_IocEmptyQueue, status, 1, (uintptr_t)ioc_id);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) IocWrite(IocIdType ioc_id, void const *data)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_ioc_write(ioc_id, data);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_IOCWRITE;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type = ioc_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_const_void_ref_type = data;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_IocWrite, status, 2, (uintptr_t)ioc_id, (uintptr_t)data);
	return (StatusType)status;
}

FUNC(StatusType, OS_CODE) IocRead(IocIdType ioc_id, void *data)
{
	os_status_t status;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (syscall_is_required() == FALSE) {
#endif
		status = os_ioc_read(ioc_id, data);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_IOCREAD;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type = ioc_id;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_2].param_void_ref_type = data;

		up_syscall(&syscall);

		status = syscall.syscall_return_value.return_os_status;
	}
#endif
	REPORT_SERVICE_ERROR(OSServiceId_IocRead, status, 2, (uintptr_t)ioc_id, (uintptr_t)data);
	return (StatusType)status;
}
#endif

FUNC(void, OS_CODE) ShutdownOS(StatusType error_status)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		os_shutdown_os(error_status);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SHUTDOWNOS;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_error_status = error_status;

		up_syscall(&syscall);
	}
#endif
}

FUNC(void, OS_CODE) ShutdownAllCores(StatusType error_status)
{
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	if (FALSE == syscall_is_required()) {
#endif
		os_shutdown_all_cores(error_status);
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	} else {
		syscall_data_t syscall;

		syscall.syscall_id = OS_SYSCALLID_SHUTDOWNALLCORES;
		syscall.syscall_param[OS_SYSCALLPARAMINDEX_1].param_error_status = error_status;

		up_syscall(&syscall);
	}
#endif
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
