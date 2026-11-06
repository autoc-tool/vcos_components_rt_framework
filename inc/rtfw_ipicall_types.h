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

#ifndef RT_FRAMEWORK_INC_OS_IPICALL_TYPES_H
#define RT_FRAMEWORK_INC_OS_IPICALL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */

/* Global Type & Structure Definition */
struct os_ipicall_activate_task_param;
typedef struct os_ipicall_activate_task_param os_ipicall_activate_task_param_t;

struct os_ipicall_get_task_state_param;
typedef struct os_ipicall_get_task_state_param os_ipicall_get_task_state_param_t;

struct os_ipicall_get_app_state_param;
typedef struct os_ipicall_get_app_state_param os_ipicall_get_app_state_param_t;

struct os_ipicall_get_event_param;
typedef struct os_ipicall_get_event_param os_ipicall_get_event_param_t;

struct os_ipicall_set_event_param;
typedef struct os_ipicall_set_event_param os_ipicall_set_event_param_t;

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
struct os_ipicall_hook_call_void_hook_param;
typedef struct os_ipicall_hook_call_void_hook_param os_ipicall_hook_call_void_hook_param_t;
#endif

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)

struct os_ipicall_start_schtbl_rel_param;
typedef struct os_ipicall_start_schtbl_rel_param os_ipicall_start_schtbl_rel_param_t;

struct os_ipicall_start_schtbl_abs_param;
typedef struct os_ipicall_start_schtbl_abs_param os_ipicall_start_schtbl_abs_param_t;

struct os_ipicall_stop_schtbl_param;
typedef struct os_ipicall_stop_schtbl_param os_ipicall_stop_schtbl_abs_param_t;

struct os_ipicall_get_schtbl_status_param;
typedef struct os_ipicall_get_schtbl_status_param os_ipicall_get_schtbl_status_param_t;

struct os_ipicall_next_schtbl_param;
typedef struct os_ipicall_next_schtbl_param os_ipicall_next_schtbl_param_t;

struct os_ipicall_next_schtbl_fromstop_param;
typedef struct os_ipicall_next_schtbl_fromstop_param os_ipicall_next_schtbl_from_stop_param_t;

struct os_ipicall_start_schtbl_synchron_param;
typedef struct os_ipicall_start_schtbl_synchron_param os_ipicall_start_schtbl_synchron_param_t;

struct os_ipicall_sync_schtbl_param;
typedef struct os_ipicall_sync_schtbl_param os_ipicall_sync_schtbl_param_t;

struct os_ipicall_set_schtbl_async_param;
typedef struct os_ipicall_set_schtbl_async_param os_ipicall_set_schtbl_aysnc_param_t;

struct os_ipicall_deviate_schtbl_param;
typedef struct os_ipicall_deviate_schtbl_param os_ipicall_deviate_schtbl_param_t;

struct os_ipicall_set_schtbl_appid_param;
typedef struct os_ipicall_set_schtbl_appid_param os_ipicall_set_schtbl_appid_param_t;

struct os_ipicall_clear_schtbl_appid_param;
typedef struct os_ipicall_clear_schtbl_appid_param os_ipicall_clear_schtbl_appid_param_t;
#endif
#if defined(CONFIG_RT_FRAMEWORK_IPICALL_TEST) && (CONFIG_RT_FRAMEWORK_IPICALL_TEST == 1)
struct os_ipicall_test_param;
typedef struct os_ipicall_test_param os_ipicall_test_param_t;
#endif

struct os_ipicall_shutdown_all_cores_param;
typedef struct os_ipicall_shutdown_all_cores_param os_ipicall_shutdown_all_cores_param_t;

struct Os_IpiCall_Test_Parameter;
typedef struct Os_IpiCall_Test_Parameter Os_IpiCall_Test_ParameterType;

union os_ipicall_param_e;
typedef union os_ipicall_param_e os_ipicall_param_t;

struct os_ipicall_send_s;
typedef struct os_ipicall_send_s os_ipicall_send_t;

struct os_ipicall_return_s;
typedef struct os_ipicall_return_s os_ipicall_return_t;

struct os_ipicall_data_s;
typedef struct os_ipicall_data_s os_ipicall_data_t;

struct os_ipicall_inst_s;
typedef struct os_ipicall_inst_s os_ipicall_inst_t;

struct os_ipicall_cfg_s;
typedef struct os_ipicall_cfg_s os_ipicall_cfg_t;

typedef uint8 os_ipicall_index_t;
typedef uint8 os_ipicall_buf_state_t;
typedef uint8 os_ipicall_service_id_t;

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_IPICALL_TYPES_H */
