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

#ifndef RT_FRAMEWORK_INC_OS_CORE_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_CORE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_core_types.h"
#include "rtfw_context.h"
#include "Compiler.h"

#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
#include "rtfw_ipicall_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */

#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
#include "rtfw_resource_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE */

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
#include "rtfw_application_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_MULTIAPP */

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
#include "rtfw_hook_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_HOOK */

#if defined(CONFIG_RT_FRAMEWORK_ERROR) && (CONFIG_RT_FRAMEWORK_ERROR == 1)
#include "rtfw_error_internal.h"
#endif

#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
#include "rtfw_ioc_internal.h"
#endif

#if defined (CONFIG_RT_FRAMEWORK_HAVE_TRAP_HANDLER) && (CONFIG_RT_FRAMEWORK_HAVE_TRAP_HANDLER == 1)
#include "rtfw_trap_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_HAVE_TRAP_HANDLER */

#include "rtfw_interrupt_internal.h"

/* 获取 os_core_cfg_t 中的信息 */
#define OS_CORE_GET_CORE_INSTANCE_REF(core_ref) ((core_ref)->instance)
#define OS_CORE_GET_COREID(core_ref) (((os_core_cfg_t *)core_ref)->core_id)
#define OS_CORE_GET_PHYSICAL_COREID(core_ref) ((core_ref)->physical_id)

#define OS_CORE_GET_STARTUP_HOOK(core_ref) ((core_ref)->startup_hook)
#define OS_CORE_GET_SHUTDOWN_HOOK(core_ref) ((core_ref)->shutdown_hook)
#define OS_CORE_GET_ERROR_HOOK(core_ref) ((core_ref)->error_hook)
#define OS_CORE_GET_PROTECTION_HOOK(core_ref) ((core_ref)->protection_hook)

#define OS_CORE_GET_KERNEL_APP(core_ref) ((core_ref)->kernel_app)

/* 通过 os_core_cfg_t 获取 OsCoreInstanceType 中的信息 */
#define OS_CORE_GET_INTAPISTATE_REF(core_ref) (&((OS_CORE_GET_CORE_INSTANCE_REF(core_ref))->int_api_state))
#define OS_CORE_GET_CONTEXTNEST_REF(core_ref) (&((OS_CORE_GET_CORE_INSTANCE_REF(core_ref))->ctx_next))

/* 获取 OsCoreInstanceType 中的信息 */
#define OS_CORE_INSTANCE_GET_INTAPISTATE_REF(core_inst_ref) (&((core_inst_ref)->int_api_state))
#define OS_CORE_INSTANCE_GET_CONTEXTNEST_REF(core_inst_ref) (&((core_inst_ref)->ctx_next))

/* Global Enumeration Definition */
/* coverity[misra_c_2012_rule_2_3_violation:SUPPRESS] */
typedef enum {
	/*! before StartCore(). */
	OS_CORESTARTSTATE_NO_START = 0,
	/*! After StartCore() before StartOS(). */
	OS_CORESTARTSTATE_START_CORE,
	/*! After StartOS(). */
	OS_CORESTARTSTATE_START_OS,
} Os_CoreStartStateType;

/* Global Type & Structure Definition */
struct os_core_inst {
	uint32 status; /*! Core status */
#if defined(CONFIG_RT_FRAMEWORK_ERROR) && (CONFIG_RT_FRAMEWORK_ERROR == 1)
	struct os_error_info_s last_error_info;
#endif
	os_ctx_nest_t ctx_next; /*! 上下文嵌套信息 */
#if defined(CONFIG_RT_FRAMEWORK_HAVE_TRAP_HANDLER) && (CONFIG_RT_FRAMEWORK_HAVE_TRAP_HANDLER == 1)
	os_expt_full_context_t expt_full_context; /*! Exception full context */
#endif /* CONFIG_RT_FRAMEWORK_HAVE_TRAP_HANDLER */
};

/*! Constant core configuration information. */
struct os_core_cfg_s {
	os_core_inst_t *instance; /*! Core Object Description */
	CoreIdType core_id; /*! Logical core ID */
	CoreIdType physical_id; /*! Physical core ID */
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
	const os_app_cfg_t *kernel_app;
	const os_app_cfg_t *const *app_refs; /*! Application reference. */
	const OsObjIndexType *app_count; /*! Number of tasks. */
#endif
#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
	const os_ioc_cfg_t *const *ioc_refs;
	const OsObjIndexType *ioc_count;
#endif /* CONFIG_RT_FRAMEWORK_IOC */
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
	const os_ipicall_cfg_t *const *ipicall_refs; /*! Ipicall reference. */
	const OsObjIndexType *ipicall_count; /*! Number of IpiCalls. */
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
	const os_resource_cfg_t *const *resource_refs; /*! Resource reference. */
	const OsObjIndexType *resource_count; /*! Number of Resource. */
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE */
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
	const os_hook_cfg_t *startup_hook;
	const os_hook_cfg_t *shutdown_hook;
	const os_hook_cfg_t *error_hook;
	const os_hook_cfg_t *protection_hook;
	const os_hook_cfg_t *const *hook_refs;
	const OsObjIndexType *hook_count;
#endif
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	const struct mpu_access_right_s *mpu_access_right;
#endif
};

DeclareRtfwRefTable_H(core) /*! Reference to Cores configuration table  */

/* Global Function Declaration */

static inline CoreIdType os_get_core_id(void)
{
#ifdef CONFIG_ARCH_HAVE_MULTICPU
	return (CoreIdType)up_cpu_index();
#else
	return 0;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_CORE_INTERNAL_H */
