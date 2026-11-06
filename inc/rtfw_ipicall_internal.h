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

#ifndef RT_FRAMEWORK_INC_OS_IPICALL_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_IPICALL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_ipicall_types.h"
#include "Compiler.h"

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "rtfw_schedtable_types.h"
#endif

#include "ipicall_cfg.h"

/* Global Macro Definition */
#define SYNC_WAIT_TIMEOUT 0xFFFFU

/* Global Enumeration Definition */

#define OS_IPICALL_ACK ((os_ipicall_buf_state_t)0)
#define OS_IPICALL_SYNC_NACK ((os_ipicall_buf_state_t)1)
#define OS_IPICALL_ASYNC_NACK ((os_ipicall_buf_state_t)2)
#define OS_IPICALL_SYNC_PENDING ((os_ipicall_buf_state_t)3)

typedef enum { OS_IPICALL_UNICAST, OS_IPICALL_BROADCAST } os_ipicall_cast_mode;

/* Global Type & Structure Definition */
struct os_ipicall_activate_task_param {
	TaskType task_id;
};

struct os_ipicall_get_task_state_param {
	TaskType task_id;
	TaskStateType task_state;
};

struct os_ipicall_get_app_state_param {
	ApplicationType app_id;
	ApplicationStateType app_state;
};

struct os_ipicall_get_event_param {
	TaskType task_id;
	os_event_mask_t mask;
};

struct os_ipicall_set_event_param {
	TaskType task_id;
	os_event_mask_t mask;
};

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
struct os_ipicall_hook_call_void_hook_param {
	HookType hook_id;
};
#endif

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
struct os_ipicall_start_schtbl_rel_param {
	ScheduleTableType schedtable_id;
	TickType offset;
};

struct os_ipicall_start_schtbl_abs_param {
	ScheduleTableType schedtable_id;
	TickType start;
};

struct os_ipicall_stop_schtbl_param {
	ScheduleTableType schedtable_id;
};

struct os_ipicall_get_schtbl_status_param {
	ScheduleTableType schedtable_id;
	ScheduleTableStatusType schedtable_status;
};

struct os_ipicall_next_schtbl_param {
	ScheduleTableType curr_schedtable_id;
	ScheduleTableType next_schedtable_id;
};

struct os_ipicall_next_schtbl_fromstop_param {
	ScheduleTableType curr_schedtable_id;
	ScheduleTableType next_schedtable_id;
};

struct os_ipicall_start_schtbl_synchron_param {
	ScheduleTableType schedtable_id;
};

struct os_ipicall_sync_schtbl_param {
	ScheduleTableType schedtable_id;
	TickType tick_value;
};

struct os_ipicall_set_schtbl_async_param {
	ScheduleTableType schedtable_id;
};

struct os_ipicall_deviate_schtbl_param {
	ScheduleTableType schedtable_id;
	DeviateType deviate_value;
};

struct os_ipicall_set_schtbl_appid_param {
	ScheduleTableType schedtable_id;
	ApplicationIDType app_id;
};

struct os_ipicall_clear_schtbl_appid_param {
	ScheduleTableType schedtable_id;
	ApplicationIDType app_id;
};
#endif
#if defined(CONFIG_RT_FRAMEWORK_IPICALL_TEST) && (CONFIG_RT_FRAMEWORK_IPICALL_TEST == 1)
struct os_ipicall_test_param {
	uint32 test_value;
};
#endif

struct os_ipicall_shutdown_all_cores_param {
	StatusType error_status;
};

union os_ipicall_param_e {
	os_ipicall_activate_task_param_t avtivate_task_param;
	os_ipicall_get_task_state_param_t get_task_state_param;
	os_ipicall_get_event_param_t get_event_param;
	os_ipicall_set_event_param_t set_event_param;
	os_ipicall_get_app_state_param_t get_app_state_param;
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
	os_ipicall_hook_call_void_hook_param_t hook_call_void_hook_param;
#endif
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	os_ipicall_start_schtbl_rel_param_t start_schtbl_rel_param;
	os_ipicall_start_schtbl_abs_param_t start_schtbl_abs_param;
	os_ipicall_stop_schtbl_abs_param_t stop_schtbl_param;
	os_ipicall_get_schtbl_status_param_t get_schtbl_status_param;
	os_ipicall_next_schtbl_param_t next_schtbl_param;
	os_ipicall_next_schtbl_from_stop_param_t next_schtbl_fromstop_param;
	os_ipicall_start_schtbl_synchron_param_t start_schtbl_synchron_param;
	os_ipicall_sync_schtbl_param_t sync_schtbl_param;
	os_ipicall_set_schtbl_aysnc_param_t set_schtbl_async_param;
	os_ipicall_deviate_schtbl_param_t deviate_schtbl_param;
	os_ipicall_set_schtbl_appid_param_t set_schtbl_appid_param;
	os_ipicall_clear_schtbl_appid_param_t clear_schtbl_appid_param;
#endif /* #if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1) */
#if defined(CONFIG_RT_FRAMEWORK_IPICALL_TEST) && (CONFIG_RT_FRAMEWORK_IPICALL_TEST == 1)
	os_ipicall_test_param_t ipc_test_param;
#endif
	os_ipicall_shutdown_all_cores_param_t shutdown_all_cores_param;
};

struct os_ipicall_send_s {
	os_ipicall_param_t in_param;
	os_ipicall_service_id_t service;
	ApplicationIDType send_app_id;
	os_ipicall_buf_state_t ack;
};

struct os_ipicall_return_s {
	os_ipicall_param_t out_param;
	os_status_t return_status;
	os_ipicall_service_id_t service;
};

struct os_ipicall_data_s {
	os_ipicall_index_t index;
	os_ipicall_send_t send_element;
	os_ipicall_return_t ret_element;
};

typedef os_status_t (*os_ipicall_service_t)(os_ipicall_data_t *param);

/*! IPICALL Instance Description */
struct os_ipicall_inst_s {
	os_ipicall_index_t send_idx;
	os_ipicall_index_t receive_idx;
	os_ipicall_send_t send_buffer[QUEUE_DEPTH];
	os_ipicall_return_t result_buffer[QUEUE_DEPTH];
};

/*! IpiCall configuration by user */
struct os_ipicall_cfg_s {
	os_ipicall_inst_t *instance;
	uint32 ipicall_id;
	/*! 发起核ID */
	CoreIdType src_core_id;
	/*! 目标核ID */
	CoreIdType target_core_id;
	/*! 支持的服务列表 */
	const os_ipicall_service_t *service_ref;
};

DeclareRtfwRefTable_H(ipicall) /*! Reference to OS task configuration table  */

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
/* Global Function Implementation */
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_activate_task(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_activate_task_asyn(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_get_task_state(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_get_application_state(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_get_event(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_set_event(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_set_event_asyn(os_ipicall_data_t *param);
FUNC(os_status_t, OS_CODE)
os_ipicall_get_app_state(uint32 target_core, ApplicationType app_id, ApplicationStateRefType state);
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_start_scheduletable_rel(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_start_scheduletable_abs(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_stop_scheduletable(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_get_scheduletable_status(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_next_scheduletable(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_next_scheduletable_from_stop(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_start_scheduletable_synchron(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_sync_scheduletable(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_set_scheduletable_async(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_start_schedule_table_rel(uint32 target_core, ScheduleTableType schedtable_id, TickType offset);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_start_schedule_table_abs(uint32 target_core, ScheduleTableType schedtable_id, TickType start);
extern FUNC(os_status_t, OS_CODE) os_ipicall_stop_schedule_table(uint32 target_core, ScheduleTableType schedtable_id);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_get_schedule_table_status(uint32 target_core, ScheduleTableType schedtable_id,
					     ScheduleTableStatusType *schedtable_status);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_next_schedule_table(uint32 target_core, ScheduleTableType curr_schedtable_id,
				       ScheduleTableType next_schedtable_id);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_next_schedule_table_from_stop(uint32 target_core, ScheduleTableType curr_schedtable_id,
						 ScheduleTableType next_schedtable_id);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_start_schedule_table_synchron(uint32 target_core, ScheduleTableType schedtable_id);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_sync_schedule_table(uint32 target_core, ScheduleTableType schedtable_id, TickType tick_value);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_set_schedule_table_async(uint32 target_core, ScheduleTableType schedtable_id);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_deviate_schedule_table(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_set_schedule_table_app_id(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_clear_schedule_table_app_id(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_deviate_schedule_table(uint32 target_core, ScheduleTableType schedtable_id, TickType deviate_value);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_set_schedule_table_app_id(uint32 target_core, ScheduleTableType schedtable_id,
					     ApplicationIDType app_id);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_clear_schedule_table_app_id(uint32 target_core, ScheduleTableType schedtable_id,
					       ApplicationIDType app_id);
#endif /* #if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1) */
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
extern FUNC(os_status_t, OS_CODE) os_ipicall_hook_call_void_hook(uint32 target_core, HookType hook_id);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_hook_call_void_hook(os_ipicall_data_t *param);
#endif
extern FUNC(void, OS_CODE) os_ipicall_init(const os_ipicall_cfg_t *const os_ipicall_cfg_table[], OsObjIndexType count);
extern FUNC(os_status_t, OS_CODE) os_ipicall_activate_task(uint32 target_core, TaskType task_id);
extern FUNC(os_status_t, OS_CODE) os_ipicall_activate_task_asyn(uint32 target_core, TaskType task_id);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_get_task_state(uint32 target_core, TaskType task_id, TaskStateRefType state_ref);
extern FUNC(os_status_t, OS_CODE) os_ipicall_get_event(uint32 target_core, TaskType task_id, os_event_mask_t *mask);
extern FUNC(os_status_t, OS_CODE) os_ipicall_set_event(uint32 target_core, TaskType task_id, os_event_mask_t mask);
extern FUNC(os_status_t, OS_CODE)
	os_ipicall_set_event_asyn(uint32 target_core, TaskType task_id, os_event_mask_t mask);
#if defined(CONFIG_RT_FRAMEWORK_IPICALL_TEST) && (CONFIG_RT_FRAMEWORK_IPICALL_TEST == 1)
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_ipc_test_function(os_ipicall_data_t *param);
extern FUNC(os_status_t, OS_CODE) os_ipicall_ipc_test_function(uint32 target_core, uint32 test_value);
#endif
extern FUNC(os_status_t, OS_CODE) os_ipicall_shutdown_all_cores(uint32 target_core_mask, StatusType error_status);
extern FUNC(os_status_t, OS_CODE) os_ipicall_recv_shutdown_all_cores(os_ipicall_data_t *param);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_IPICALL_INTERNAL_H */
