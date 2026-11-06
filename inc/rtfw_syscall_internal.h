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

#ifndef OS_SYSCALL_INTERNAL_H
#define OS_SYSCALL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_syscall_types.h"

/* Global Enumeration Definition */
typedef enum {
	OS_SYSCALLID_ACTIVATETASK,
	OS_SYSCALLID_GETCOREID,
	OS_SYSCALLID_ACTIVATETASKASYNC,
	OS_SYSCALLID_CHAINTASK,
	OS_SYSCALLID_GETTASKID,
	OS_SYSCALLID_GETTASKSTATE,
	OS_SYSCALLID_ALLOWACCESS,
	OS_SYSCALLID_GETAPPLICATIONSTATE,
	OS_SYSCALLID_GETAPPLICATIONID,
	OS_SYSCALLID_GETCURRENTAPPLICATIONID,
	OS_SYSCALLID_SCHEDULE,
	OS_SYSCALLID_TASK_TICKDELAY,
	OS_SYSCALLID_SET_USER_TO_PRIVILEGE,
	OS_SYSCALLID_SETEVENT,
	OS_SYSCALLID_SETEVENTASYN,
	OS_SYSCALLID_CLEAREVENT,
	OS_SYSCALLID_WAITEVENT,
	OS_SYSCALLID_EVENT_TICKWAIT,
	OS_SYSCALLID_GETEVENT,
	OS_SYSCALLID_DISABLEALLINTERRUPTS,
	OS_SYSCALLID_ENABLEALLINTERRUPTS,
	OS_SYSCALLID_SUSPENDALLINTERRUPTS,
	OS_SYSCALLID_RESUMEALLINTERRUPTS,
	OS_SYSCALLID_SUSPENDOSINTERRUPTS,
	OS_SYSCALLID_RESUMEOSINTERRUPTS,
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	OS_SYSCALLID_STARTSCHEDULETABLEREL,
	OS_SYSCALLID_STARTSCHEDULETABLEABS,
	OS_SYSCALLID_STOPSCHEDULETABLE,
	OS_SYSCALLID_SETSCHEDULETABLEAPPID,
	OS_SYSCALLID_CLEARSCHEDULETABLEAPPID,
	OS_SYSCALLID_GETSCHEDULETABLESTATUS,
	OS_SYSCALLID_NEXTSCHEDULETABLE,
	OS_SYSCALLID_NEXTSCHEDULETABLEFROMSTOP,
	OS_SYSCALLID_STARTSCHEDULETABLESYNCHRON,
	OS_SYSCALLID_SYNCSCHEDULETABLE,
	OS_SYSCALLID_DEVIATESCHEDULETABLE,
	OS_SYSCALLID_SETSCHEDULETABLEASYNC,
#endif
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
	OS_SYSCALLID_GETSPINLOCK,
	OS_SYSCALLID_RELEASESPINLOCK,
	OS_SYSCALLID_TRYTOGETSPINLOCK,
	OS_SYSCALLID_GETSPINLOCKREAD,
	OS_SYSCALLID_GETSPINLOCKWRITE,
	OS_SYSCALLID_RELEASESPINLOCKREAD,
	OS_SYSCALLID_RELEASESPINLOCKWRITE,
	OS_SYSCALLID_TRYTOGETSPINLOCKREAD,
	OS_SYSCALLID_TRYTOGETSPINLOCKWRITE,
#endif
#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
	OS_SYSCALLID_GETRESOURCE,
	OS_SYSCALLID_RELEASERESOURCE,
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE */
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
	OS_SYSCALLID_INCREMENTCOUNTER,
	OS_SYSCALLID_GETCOUNTERVALUE,
	OS_SYSCALLID_GETELAPSEDVALUE,
#endif /* CONFIG_RT_FRAMEWORK_COUNTER */
#if defined(CONFIG_RT_FRAMEWORK_ERROR) && (CONFIG_RT_FRAMEWORK_ERROR == 1)
	OS_SYSCALLID_SERVICEERRORREPORT,
#endif /* CONFIG_RT_FRAMEWORK_ERROR */
#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
	OS_SYSCALLID_IOCSEND,
	OS_SYSCALLID_IOCRECEIVE,
	OS_SYSCALLID_IOCEMPTYQUEUE,
	OS_SYSCALLID_IOCWRITE,
	OS_SYSCALLID_IOCREAD,
#endif /* CONFIG_RT_FRAMEWORK_IOC */
	OS_SYSCALLID_SHUTDOWNOS,
	OS_SYSCALLID_SHUTDOWNALLCORES,
	OS_SYSCALLID_COUNT
} os_syscall_t;

typedef enum {
	OS_SYSCALLPARAMINDEX_1 = 0,
	OS_SYSCALLPARAMINDEX_2,
	OS_SYSCALLPARAMINDEX_3,
	OS_SYSCALLPARAMINDEX_4,
	OS_SYSCALLPARAMINDEX_5,
	OS_SYSCALLPARAMINDEX_COUNT
} os_syscall_param_index_t;

/* Global Type & Structure Definition */
typedef union {
	TaskType param_task_t;
	ApplicationType param_app_t;
	TaskRefType param_task_ref;
	TaskStateRefType param_task_state_ref;
	ApplicationStateRefType param_app_state_ref;
	TickType param_tick_t;
	ApplicationIDType param_applicationid;
	EventMaskType param_event_mask;
	EventMaskRefType param_event_mask_ref;
	TickRefType param_tick_ref_type;
	OSServiceIdType param_service_id;
	os_status_t param_os_status;
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	ScheduleTableType param_scheduletable;
	ScheduleTableStatusType param_scheduletable_status;
	ScheduleTableStatusRefType param_scheduletable_status_ref;
	DeviateType param_deviate;
#endif
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
	SpinlockIdType param_spinlock;
	TryToGetSpinlockType *param_try_to_get_spinlock_ref;
#endif
#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
	ResourceType param_resource;
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE */
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
	CounterType param_counter_type;
#endif /* CONFIG_RT_FRAMEWORK_COUNTER */
	IocIdType param_ioc_type;
	uint8 param_uint8_t;
	uint8 *param_uint8_ref_t;
	uint16 param_uint16_t;
	uint16 *param_uint16_ref_t;
	uint32 param_uint32_t;
	uint32 *param_uint32_ref_t;
	void *param_void_ref_type;
	void const *param_const_void_ref_type;
	va_list param_va_list;
	StatusType param_error_status;
} os_syscall_param_t;

typedef union {
	CoreIdType    return_coreid_type;
	ApplicationType return_application_type;
	os_status_t return_os_status;
} os_syscall_return_t;

struct syscall_data_s {
	os_syscall_t syscall_id;
	os_syscall_return_t syscall_return_value;
	os_syscall_param_t syscall_param[OS_SYSCALLPARAMINDEX_COUNT];
};

#ifdef __cplusplus
}
#endif

#endif /* OS_SYSCALL_INTERNAL_H */
