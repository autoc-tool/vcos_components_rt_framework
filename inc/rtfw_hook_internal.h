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

#ifndef RT_FRAMEWORK_INC_OS_HOOK_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_HOOK_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <nuttx/config.h>
#include <nuttx/sched.h>
#include "rtfw_types.h"
#include "rtfw_hook_types.h"
#include "rtfw_context_internal.h"
#include "rtfw_thread_internal.h"
#include "rtfw_core_types.h"
#include <setjmp.h>

/* Global Macro Definition */

/* 通过 os_hook_cfg_t(hook_cfg) 获取各种 object ID */
#define OS_HOOK_GET_OWNER_APPID(hook_cfg) (OS_APPLICATION_GET_APPID(OS_HOOK_GET_APPONRWECFG_REF(hook_cfg)))
#define OS_HOOK_GET_OWNER_COREID(hook_cfg) (OS_CORE_GET_COREID(OS_HOOK_GET_CORE_REF(hook_cfg)))

/* 通过 os_hook_cfg_t(hook_cfg) 获取 (hook_cfg) 的内部信息 */
#define OS_HOOK_GET_CONTEXT_REF(hook_cfg) (&((hook_cfg)->context))
#define OS_HOOK_GET_HOOKID(hook_cfg) ((hook_cfg)->hook_id)
#define OS_HOOK_GET_NESTINGMASK(hook_cfg) ((hook_cfg)->nesting_mask)

/* 通过 os_hook_cfg_t(hook_cfg) 获取 os_hook_inst_t(hook_inst_ref) 的内部信息 */
#define OS_HOOK_GET_PARAMETER(hook_cfg) ((OS_ISR_GET_INSTANCE_REF(hook_cfg))->parameter)
#define OS_HOOK_GET_RETURNVALUE(hook_cfg) ((OS_ISR_GET_INSTANCE_REF(hook_cfg))->return_value)
#define OS_HOOK_GET_ACCUMULATEDHOOKCONTEXT(hook_cfg) ((OS_ISR_GET_INSTANCE_REF(hook_cfg))->accumulated_hook_ctx)
#define OS_HOOK_GET_CURRENTNESTINGDEPTH(hook_cfg) ((OS_ISR_GET_INSTANCE_REF(hook_cfg))->cur_nesting_depth)

/* 通过 os_hook_inst_t(hook_inst_ref) 获取 (hook_inst_ref) 的内部信息 */
#define OS_HOOKINST_GET_PARAMETER(hook_inst_ref) ((hook_inst_ref)->parameter)
#define OS_HOOKINST_GET_RETURNVALUE(hook_inst_ref) ((hook_inst_ref)->return_value)
#define OS_HOOKINST_GET_ACCUMULATEDHOOKCONTEXT(hook_inst_ref) ((hook_inst_ref)->accumulated_hook_ctx)
#define OS_HOOKINST_GET_CURRENTNESTINGDEPTH(hook_inst_ref) ((hook_inst_ref)->cur_nesting_depth)

/* 通过 os_hook_cfg_t(hook_cfg) 获取 (hook_cfg)->Context 的内部信息 */
#define OS_HOOK_GET_INSTANCE_REF(hook_cfg) ((os_hook_inst_t *)((hook_cfg)->context.instance))
#define OS_HOOK_GET_OBJNAME(hook_cfg) ((hook_cfg)->context.instance_name)
#define OS_HOOK_GET_CONTEXTINST_REF(hook_cfg) (&(OS_HOOK_GET_INSTANCE_REF(hook_cfg)->ctx_instance))
#define OS_HOOK_GET_CORE_REF(hook_cfg) ((hook_cfg)->context.core)
#define OS_HOOK_GET_ARCHCONTEXTCFG_REF(hook_cfg) (&((hook_cfg)->context.arch_ctx_cfg))
#define OS_HOOK_GET_APPONRWECFG_REF(hook_cfg) ((hook_cfg)->context.app_owner_cfg)
#define OS_HOOK_GET_ACCESSINGAPPLMASK(hook_cfg) ((hook_cfg)->context.access_app_mask)
#define OS_HOOK_GET_INITCONTEXTID(hook_cfg) ((hook_cfg)->context.initial_ctx_id)
#define OS_HOOK_GET_USEFPU(hook_cfg) ((hook_cfg)->context.use_fpu)

/* 从 hook_cfg 获取 ContextInst 的内部信息 */
#define OS_HOOK_GET_CURRENT_CONTEXTID(hook_cfg) ((OS_HOOK_GET_CONTEXTINST_REF(hook_cfg))->current_ctx_id)
#define OS_HOOK_GET_LOCKLIST_REF(hook_cfg) (&((OS_HOOK_GET_CONTEXTINST_REF(hook_cfg))->lock_list))

typedef enum {
	PRO_IGNORE = 0,
	PRO_IGNORE_EXCEPTION,
	PRO_TERMINATETASKISR,
	PRO_TERMINATEAPPL,
	PRO_TERMINATEAPPL_RESTART,
	PRO_SHUTDOWN,
	OS_PROTECTION_REACTION_COUNT
} protection_return_t;

typedef void (*status_hook_callback_t)(StatusType);
typedef void (*void_hook_callback_t)(void);
typedef protection_return_t (*protection_hook_callback_t)(StatusType);

typedef union {
	StatusType status;
	AppModeType app_mode;
} os_hook_parameter_t;

typedef union {
	protection_return_t protection_reaction;
} os_hook_return_t;

struct os_hook_inst_s {
	os_ctx_inst_t ctx_instance; /*! Context instance. Must be the first member !!! */
	os_context_id_t accumulated_hook_ctx; /*! Accumulated Hook Context. */
	os_hook_parameter_t parameter; /*! Hook parameter. */
	os_hook_return_t return_value; /*! Hook return value. */
	void *saved_regs;
#if defined(CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK == 1)
	os_hook_thread_inst_t *thread;
#else
	os_hook_thread_inst_t thread;
#endif
	uint8 cur_nesting_depth;
}; /* os_hook_inst_s */

struct os_hook_cfg_s {
	os_ctx_cfg_t context; /*! Context configuration. Must be the first member !!! */
	HookType hook_id;
	os_context_id_t nesting_mask; /*! Nesting mask. */
	void_hook_callback_t callback; /*! Callback function. */
}; /* os_hook_cfg_s */

DeclareRtfwRefTable_H(hook) /*! Reference to OS task configuration table  */

#if defined(CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK == 1)
	extern const os_hook_thread_inst_t *rtfw_shared_hook_thead_ref_table[];
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
extern FUNC(void, OS_CODE) os_hook_init(const os_hook_cfg_t *const os_hook_cfg_table[], OsObjIndexType size);
extern FUNC(os_status_t, OS_CODE) os_hook_call_status_hook(HookType hook_id, StatusType error);
extern FUNC(os_status_t, OS_CODE) os_hook_call_void_hook(HookType hook_id);
extern FUNC(protection_return_t, OS_CODE) os_hook_call_protection_hook(HookType hook_id, StatusType error);
extern FUNC(protection_return_t, OS_CODE) os_hook_call_callback(const os_hook_cfg_t *hook_cfg);
extern FUNC(void, OS_CODE) os_hook_return(void);

#if defined(CONFIG_RT_FRAMEWORK_ERROR_HOOK) && (CONFIG_RT_FRAMEWORK_ERROR_HOOK == 1)
FUNC(void, OS_CODE) os_call_error_hook(const os_core_cfg_t *core_cfg, StatusType error_status);
#endif

#if defined(CONFIG_RT_FRAMEWORK_STARTUP_HOOK) && (CONFIG_RT_FRAMEWORK_STARTUP_HOOK == 1)
FUNC(void, OS_CODE) os_call_startup_hook(const os_core_cfg_t *core_cfg);
#endif

#if defined(CONFIG_RT_FRAMEWORK_PROTECTION_HOOK) && (CONFIG_RT_FRAMEWORK_PROTECTION_HOOK == 1)
FUNC(protection_return_t, OS_CODE) os_call_protection_hook(StatusType error_status);
#endif

#if defined(CONFIG_RT_FRAMEWORK_SHUTDOWN_HOOK) && (CONFIG_RT_FRAMEWORK_SHUTDOWN_HOOK == 1)
FUNC(void, OS_CODE) os_call_shutdown_hook(StatusType error_status);
#endif

extern FUNC(os_status_t, OS_CODE) rtfw_call_void_callback_hook(const os_hook_cfg_t *hook_cfg);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#if defined(CONFIG_RT_FRAMEWORK_PRETASK_HOOK) && (CONFIG_RT_FRAMEWORK_PRETASK_HOOK == 1)
#define OS_START_SEC_CALLOUT_CODE
#include "rtfw_memmap.h"
extern FUNC(void, OS_CALLOUT_CODE) os_pretaskhook(void);

#define OS_STOP_SEC_CALLOUT_CODE
#include "rtfw_memmap.h"
#endif

#if defined(CONFIG_RT_FRAMEWORK_POSTTASK_HOOK) && (CONFIG_RT_FRAMEWORK_POSTTASK_HOOK == 1)
#define OS_START_SEC_CALLOUT_CODE
#include "rtfw_memmap.h"
extern FUNC(void, OS_CALLOUT_CODE) os_posttaskhook(void);

#define OS_STOP_SEC_CALLOUT_CODE
#include "rtfw_memmap.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_HOOK_INTERNAL_H */