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
#include <nuttx/irq.h>
#include "rtfw_syscall_internal.h"
#include "rtfw_task.h"
#include "rtfw_event.h"
#include "rtfw_interrupt.h"
#include "rtfw_core.h"
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "rtfw_schedtable.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
#include "rtfw_spinlock.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
#include "rtfw_resource.h"
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
/* global Function Implementation */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

typedef void (*syscall_entry_t)(syscall_data_t *syscall_data, uint32_t *regs);

static void sys_activatetask(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_activate_task(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t);
}

static void sys_activatetaskasyn(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	(void)os_activate_task_asyn(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t);
}

static void sys_chaintask(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_chain_task(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t);
}

static void sys_gettaskid(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_get_task_id(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_ref);
}

static void sys_gettaskstate(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_get_task_state(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t,
				  syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_task_state_ref);
}

static void sys_getapplicationstate(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_get_app_state(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_app_t,
				 syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_app_state_ref);
}

static void sys_task_tickdelay(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		rtfw_task_tickdelay(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_tick_t);
}

static void sys_set_user_to_privilege(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)syscall_data;
	(void)regs;

	up_syscall_set_prev_privilege(regs, true);
}

static void sys_set_event(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_set_event(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t,
			     syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask);
}

static void sys_set_eventasync(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	(void)os_set_event_asyn(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t,
				syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask);
}

static void sys_clear_event(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_clear_event(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask);
}

static void sys_wait_event(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_wait_event(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask);
}

static void sys_event_tickwait(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		rtfw_event_tickwait(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_event_mask,
				    syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_t);
}

static void sys_get_event(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_get_event(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_task_t,
			     syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_event_mask_ref);
}

static void sys_schedule(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status = os_schedule();
}

static void sys_allowaccess(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status = os_allow_access();
}

static void sys_getapplicationid(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_application_type = os_get_app_id();
}

static void sys_getcurrentapplicationid(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_application_type = os_get_current_app_id();
}

static void sys_disableallinterrupts(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)syscall_data;
	(void)regs;

	os_disable_all_interrupts();
	up_syscall_set_prev_int_state(regs);
}

static void sys_enableallinterrupts(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)syscall_data;
	(void)regs;

	os_enable_all_interrupts();
	up_syscall_set_prev_int_state(regs);
}

static void sys_suspendallinterrupts(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)syscall_data;
	(void)regs;

	os_suspend_all_interrupts();
	up_syscall_set_prev_int_state(regs);
}

static void sys_resumeallinterrupts(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)syscall_data;
	(void)regs;

	os_resume_all_interrupts();
	up_syscall_set_prev_int_state(regs);
}

static void sys_suspendosinterrupts(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)syscall_data;
	(void)regs;

	os_suspend_os_interrupts();
	up_syscall_set_prev_int_state(regs);
}

static void sys_resumeosinterrupts(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)syscall_data;
	(void)regs;

	os_resume_os_interrupts();
	up_syscall_set_prev_int_state(regs);
}

static void sys_getcoreid(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_coreid_type = os_get_core_id();
}

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
static void sys_start_schedule_table_rel(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_start_schedule_table_rel(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
					    syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_t);
}

static void sys_start_schedule_table_abs(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_start_schedule_table_abs(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
					    syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_uint32_t);
}

static void sys_stop_schedule_table(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_stop_schedule_table(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable);
}

static void sys_set_schedule_table_app_id(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_set_schedule_table_app_id(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
					     syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_applicationid);
}

static void sys_clear_schedule_table_app_id(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_clear_schedule_table_app_id(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
					       syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_applicationid);
}

static void sys_get_schedule_table_status(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_get_schedule_table_status(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
		syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable_status_ref);
}

static void sys_next_schedule_table(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_next_schedule_table(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				       syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable);
}

static void sys_next_schedule_table_from_stop(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
	os_next_schedule_table_from_stop(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
	syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_scheduletable);
}

static void sys_start_schedule_table_synchron(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
	os_start_schedule_table_synchron(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable);
}

static void sys_sync_schedule_table(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_sync_schedule_table(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
				       syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_t);
}

static void sys_deviate_schedule_table(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_deviate_schedule_table(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable,
					  syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_deviate);
}

static void sys_set_schedule_table_async(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_set_schedule_table_async(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_scheduletable);
}

#endif /* CONFIG_RT_FRAMEWORK_SCHTBL */

#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
static void sys_get_spinlock(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_get_spinlock(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
	up_syscall_set_prev_int_state(regs);
}

static void sys_release_spinlock(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_release_spinlock(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
	up_syscall_set_prev_int_state(regs);
}

static void sys_try_to_get_spinlock(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_try_to_get_spinlock(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock,
		syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref);
	up_syscall_set_prev_int_state(regs);
}

static void sys_get_spinlock_read(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_get_spinlock_read(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
	up_syscall_set_prev_int_state(regs);
}

static void sys_get_spinlock_write(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_get_spinlock_write(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
	up_syscall_set_prev_int_state(regs);
}

static void sys_release_spinlock_read(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_release_spinlock_read(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
	up_syscall_set_prev_int_state(regs);
}

static void sys_release_spinlock_write(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_release_spinlock_write(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock);
	up_syscall_set_prev_int_state(regs);
}

static void sys_try_to_get_spinlock_read(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_try_to_get_spinlock_read(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock,
		syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref);
	up_syscall_set_prev_int_state(regs);
}

static void sys_try_to_get_spinlock_write(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_try_to_get_spinlock_write(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_spinlock,
		syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_try_to_get_spinlock_ref);
	up_syscall_set_prev_int_state(regs);
}

#endif /* CONFIG_RT_FRAMEWORK_SPINLOCK */

#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
static void sys_get_resource(syscall_data_t *syscall_data, uint32_t *regs)
{
	syscall_data->syscall_return_value.return_os_status =
		os_get_resource(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_resource);
	up_syscall_set_prev_int_state(regs);
}

static void sys_release_resource(syscall_data_t *syscall_data, uint32_t *regs)
{
	syscall_data->syscall_return_value.return_os_status =
		os_release_resource(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_resource);
	up_syscall_set_prev_int_state(regs);
}

#endif /* CONFIG_RT_FRAMEWORK_RESOURCE */
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
static void sys_get_counter_value(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_counter_get_value(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_counter_type,
				     syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_ref_type);
}

static void sys_get_elapsed_value(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_counter_get_elapsed_value(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_counter_type,
					     syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_tick_ref_type,
					     syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_3].param_tick_ref_type);
}

static void sys_increase_counter(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_counter_increment(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_counter_type);
}

static void sys_vservice_error_report(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	vservice_error_report(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_service_id,
			      syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_os_status,
			      syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_3].param_uint32_t,
			      syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_4].param_va_list);
}

#endif /* CONFIG_RT_FRAMEWORK_COUNTER */

#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
static void sys_ioc_send(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_ioc_send(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
			       syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_const_void_ref_type);
}

static void sys_ioc_receive(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_ioc_recv(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
				  syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_void_ref_type);
}

static void sys_ioc_emptyqueue(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_ioc_empty_queue(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type);
}

static void sys_ioc_write(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_ioc_write(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
			     syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_const_void_ref_type);
}

static void sys_ioc_read(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	syscall_data->syscall_return_value.return_os_status =
		os_ioc_read(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_ioc_type,
			    syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_2].param_void_ref_type);
}
#endif

static void sys_shutdown_os(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	os_shutdown_os(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_error_status);
}

static void sys_shutdown_all_cores(syscall_data_t *syscall_data, uint32_t *regs)
{
	(void)regs;

	os_shutdown_all_cores(syscall_data->syscall_param[OS_SYSCALLPARAMINDEX_1].param_error_status);
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#define OS_START_SEC_CONST
#include "rtfw_memmap.h"

static VAR(syscall_entry_t, OS_CONST) os_syscall_table[] = {
	[OS_SYSCALLID_ACTIVATETASK] = sys_activatetask,
	[OS_SYSCALLID_ACTIVATETASKASYNC] = sys_activatetaskasyn,
	[OS_SYSCALLID_CHAINTASK] = sys_chaintask,
	[OS_SYSCALLID_GETTASKID] = sys_gettaskid,
	[OS_SYSCALLID_GETTASKSTATE] = sys_gettaskstate,
	[OS_SYSCALLID_GETAPPLICATIONSTATE] = sys_getapplicationstate,
	[OS_SYSCALLID_TASK_TICKDELAY] = sys_task_tickdelay,
	[OS_SYSCALLID_SET_USER_TO_PRIVILEGE] = sys_set_user_to_privilege,
	[OS_SYSCALLID_SETEVENT] = sys_set_event,
	[OS_SYSCALLID_SETEVENTASYN] = sys_set_eventasync,
	[OS_SYSCALLID_CLEAREVENT] = sys_clear_event,
	[OS_SYSCALLID_WAITEVENT] = sys_wait_event,
	[OS_SYSCALLID_EVENT_TICKWAIT] = sys_event_tickwait,
	[OS_SYSCALLID_GETEVENT] = sys_get_event,
	[OS_SYSCALLID_SCHEDULE] = sys_schedule,
	[OS_SYSCALLID_ALLOWACCESS] = sys_allowaccess,
	[OS_SYSCALLID_GETAPPLICATIONID] = sys_getapplicationid,
	[OS_SYSCALLID_GETCURRENTAPPLICATIONID] = sys_getcurrentapplicationid,
	[OS_SYSCALLID_DISABLEALLINTERRUPTS] = sys_disableallinterrupts,
	[OS_SYSCALLID_ENABLEALLINTERRUPTS] = sys_enableallinterrupts,
	[OS_SYSCALLID_SUSPENDALLINTERRUPTS] = sys_suspendallinterrupts,
	[OS_SYSCALLID_RESUMEALLINTERRUPTS] = sys_resumeallinterrupts,
	[OS_SYSCALLID_SUSPENDOSINTERRUPTS] = sys_suspendosinterrupts,
	[OS_SYSCALLID_RESUMEOSINTERRUPTS] = sys_resumeosinterrupts,
	[OS_SYSCALLID_GETCOREID] = sys_getcoreid,
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	[OS_SYSCALLID_STARTSCHEDULETABLEREL] = sys_start_schedule_table_rel,
	[OS_SYSCALLID_STARTSCHEDULETABLEABS] = sys_start_schedule_table_abs,
	[OS_SYSCALLID_STOPSCHEDULETABLE] = sys_stop_schedule_table,
	[OS_SYSCALLID_SETSCHEDULETABLEAPPID] = sys_set_schedule_table_app_id,
	[OS_SYSCALLID_CLEARSCHEDULETABLEAPPID] = sys_clear_schedule_table_app_id,
	[OS_SYSCALLID_GETSCHEDULETABLESTATUS] = sys_get_schedule_table_status,
	[OS_SYSCALLID_NEXTSCHEDULETABLE] = sys_next_schedule_table,
	[OS_SYSCALLID_NEXTSCHEDULETABLEFROMSTOP] = sys_next_schedule_table_from_stop,
	[OS_SYSCALLID_STARTSCHEDULETABLESYNCHRON] = sys_start_schedule_table_synchron,
	[OS_SYSCALLID_SYNCSCHEDULETABLE] = sys_sync_schedule_table,
	[OS_SYSCALLID_DEVIATESCHEDULETABLE] = sys_deviate_schedule_table,
	[OS_SYSCALLID_SETSCHEDULETABLEASYNC] = sys_set_schedule_table_async,
#endif /* CONFIG_RT_FRAMEWORK_SCHTBL */
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
	[OS_SYSCALLID_GETSPINLOCK] = sys_get_spinlock,
	[OS_SYSCALLID_RELEASESPINLOCK] = sys_release_spinlock,
	[OS_SYSCALLID_TRYTOGETSPINLOCK] = sys_try_to_get_spinlock,
	[OS_SYSCALLID_GETSPINLOCKREAD] = sys_get_spinlock_read,
	[OS_SYSCALLID_GETSPINLOCKWRITE] = sys_get_spinlock_write,
	[OS_SYSCALLID_RELEASESPINLOCKREAD] = sys_release_spinlock_read,
	[OS_SYSCALLID_RELEASESPINLOCKWRITE] = sys_release_spinlock_write,
	[OS_SYSCALLID_TRYTOGETSPINLOCKREAD] = sys_try_to_get_spinlock_read,
	[OS_SYSCALLID_TRYTOGETSPINLOCKWRITE] = sys_try_to_get_spinlock_write,
#endif /* CONFIG_RT_FRAMEWORK_SPINLOCK */
#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
	[OS_SYSCALLID_GETRESOURCE] = sys_get_resource,
	[OS_SYSCALLID_RELEASERESOURCE] = sys_release_resource,
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE */
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
	[OS_SYSCALLID_GETCOUNTERVALUE] = sys_get_counter_value,
	[OS_SYSCALLID_GETELAPSEDVALUE] = sys_get_elapsed_value,
	[OS_SYSCALLID_INCREMENTCOUNTER] = sys_increase_counter,
#endif /* CONFIG_RT_FRAMEWORK_COUNTER */
#ifdef CONFIG_RT_FRAMEWORK_ERROR
	[OS_SYSCALLID_SERVICEERRORREPORT] = sys_vservice_error_report,
#endif /* CONFIG_RT_FRAMEWORK_ERROR */
#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
	[OS_SYSCALLID_IOCSEND] = sys_ioc_send,
	[OS_SYSCALLID_IOCRECEIVE] = sys_ioc_receive,
	[OS_SYSCALLID_IOCEMPTYQUEUE] = sys_ioc_emptyqueue,
	[OS_SYSCALLID_IOCWRITE] = sys_ioc_write,
	[OS_SYSCALLID_IOCREAD] = sys_ioc_read,
#endif /* CONFIG_RT_FRAMEWORK_IOC */
	[OS_SYSCALLID_SHUTDOWNOS] = sys_shutdown_os,
	[OS_SYSCALLID_SHUTDOWNALLCORES] = sys_shutdown_all_cores,
};

#define OS_STOP_SEC_CONST
#include "rtfw_memmap.h"

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) rtfw_syscall_handler(void *syscall_args, uint32_t *regs)
{
	syscall_data_t *syscall_data;

	up_syscall_set_enter_int_state(regs);

	if (UNLIKELY(syscall_args == NULL)) {
		up_syscall_set_exit_int_state(regs);
		return;
	}

	syscall_data = (syscall_data_t *)syscall_args;

	if (LIKELY((syscall_data->syscall_id < (OsObjIndexType)OS_SYSCALLID_COUNT) &&
		   (os_syscall_table[syscall_data->syscall_id] != NULL_PTR))) {
		os_syscall_table[syscall_data->syscall_id](syscall_data, regs);
	}

	up_syscall_set_exit_int_state(regs);
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
