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

#ifndef RT_FRAMEWORK_INC_OS_TASK_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_TASK_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_task_types.h"
#include "rtfw_list.h"
#include "rtfw_context_internal.h"
#include "rtfw_core_types.h"
#include "rtfw_thread_internal.h"

/* 全局宏定义 */

#define OS_TASK_NO_ACTIVATION_CNT ((uint8)0) /* 任务没有激活 */
#define OS_TASK_MIN_ACTIVATION_CNT ((uint8)1) /* 任务最小激活次数 */
#if (CONFIG_RT_FRAMEWORK_TASK_MULTIPLE_ACTIVATION == 1)
#define OS_TASK_MAX_ACTIVATION_CNT ((uint8)255) /* 任务最大激活次数 */
#else
#define OS_TASK_MAX_ACTIVATION_CNT ((uint8)1) /* 任务最大激活次数 */
#endif /* CONFIG_RT_FRAMEWORK_TASK_MULTIPLE_ACTIVATION */

/* 通过 os_task_cfg_t(task_cfg) 获取各种对象 ID */
#define OS_TASK_GET_OWNER_APPID(task_cfg) (OS_APPLICATION_GET_APPID(OS_TASK_GET_APPONRWECFG_REF(task_cfg)))
#define OS_TASK_GET_OWNER_COREID(task_cfg) (OS_CORE_GET_COREID(OS_TASK_GET_CORE_REF(task_cfg)))

/* 通过 os_task_cfg_t(task_cfg) 获取 (task_cfg) 的内部信息 */
#define OS_TASK_GET_CONTEXT_REF(task_cfg) (&((task_cfg)->context))
#define OS_TASK_GET_BASEPRIORITY(task_cfg) ((task_cfg)->base_priority)
#define OS_TASK_GET_TASKID(task_cfg) ((task_cfg)->task_id)
#define OS_TASK_GET_AUTOSTARTMODE(task_cfg) ((task_cfg)->autostart_mode)
#define OS_TASK_GET_ACTIVATIONLIMIT(task_cfg) ((task_cfg)->activation_limit)
#if defined(CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK == 1)
#define OS_TASK_GET_TASK_SHARE_STACK_REF(task_cfg) ((task_cfg)->task_share_stack)
#endif
/* 通过 os_task_cfg_t(task_cfg) 获取 os_ctx_cfg_t(ContextRef) 的内部信息 */
#define OS_TASK_GET_INSTANCE_REF(task_cfg) ((os_task_inst_t *)((task_cfg)->context.instance))
#define OS_TASK_GET_OBJNAME(task_cfg) ((task_cfg)->context.instance_name)
#define OS_TASK_GET_CONTEXTINST_REF(task_cfg) (&(OS_TASK_GET_INSTANCE_REF(task_cfg)->ctx_instance))
#define OS_TASK_GET_CORE_REF(task_cfg) ((task_cfg)->context.core)
#define OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg) (&((task_cfg)->context.arch_ctx_cfg))
#define OS_TASK_GET_APPONRWECFG_REF(task_cfg) ((task_cfg)->context.app_owner_cfg)
#define GET_APP_CFG_BY_TASK_ID(task_id) ((os_task_cfg_ref_table[task_id])->context.app_owner_cfg)
#define GET_APP_ID_BY_TASK_ID(task_id) ((os_task_cfg_ref_table[task_id])->context.app_owner_cfg->app_id)
#define OS_TASK_GET_ACCESSINGAPPLMASK(task_cfg) ((task_cfg)->context.access_app_mask)
#define OS_TASK_GET_INITCONTEXTID(task_cfg) ((task_cfg)->context.initial_ctx_id)
#define OS_TASK_GET_USEFPU(task_cfg) ((task_cfg)->context.use_fpu)
#define OS_TASK_GET_ACTIVATION_LIMIT(task_cfg) ((task_cfg)->activation_limit)

/* 通过 os_task_cfg_t(task_cfg) 获取 ContextInstType(ctx_instance) 的内部信息 */
#define OS_TASK_GET_CURRENT_CONTEXTID(task_cfg) ((OS_TASK_GET_CONTEXTINST_REF(task_cfg))->current_ctx_id)

#define OS_TASK_GET_LOCKLIST_REF(task_cfg) (&((OS_TASK_GET_CONTEXTINST_REF(task_cfg))->lock_list))

/* 通过 os_task_cfg_t(task_cfg)  获取 os_arch_ctx_cfg_t(arch_ctx_cfg) 的内部信息 */
#define OS_TASK_GET_STACKADDR(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->stack_addr)
#define OS_TASK_GET_STACKSIZE(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->stack_size)
#define OS_TASK_GET_ENTRYADDR(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->entry_addr)
#define OS_TASK_GET_EXITADDR(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->exit_addr)
#define OS_TASK_GET_ENTRYPARAM(task_cfg) ((OS_TASK_GET_ARCHCONTEXTCFG_REF(task_cfg))->entry_param)

#define OS_TASK_GET_TASKREF_BY_ID(task_id) (os_task_cfg_ref_table[task_id])

/* 全局枚举定义 */

/* 全局类型与结构体定义 */
struct os_task_event_s {
	nxevent_wait_t wait;
	nxevent_t event;
};

struct os_task_inst_s {
	os_ctx_inst_t ctx_instance; /*! 上下文实例。必须是第一个成员！！！ */
	os_task_thread_inst_t thread;
}; /* os_task_inst_t */

/*! Task configuration by user */
struct os_task_cfg_s {
	os_ctx_cfg_t context; /*! 上下文配置。必须是第一个成员！！！ */
	OsTaskPrioType base_priority; /*! 任务的配置优先级 */
	os_task_event_t *event_hdl;
#if defined(CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK == 1)
	const task_share_stack_info_t *const task_share_stack;
#endif
	TaskType task_id; /*! 任务标识符 */
	AppModeType autostart_mode; /*! 在 OS 初始化时应被激活的应用模式 */
	boolean is_preemptive;
	uint8 activation_limit;  /*! 任务激活限制，必须大于等于 1 */
}; /* os_task_cfg_t */

DeclareRtfwRefTable_H(task) /*! 指向 OS 任务配置表的引用 */

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

extern FUNC(void, OS_CODE) os_task_init(const os_task_cfg_t *const os_task_cfg_table[], OsObjIndexType size);
extern FUNC(void, OS_CODE) os_disable_level_km(void);
extern FUNC(void, OS_CODE) os_enable_level_km(void);
extern FUNC(os_status_t, OS_CODE) os_activate_task(TaskType task_id);
extern FUNC(os_status_t, OS_CODE) os_activate_task_asyn(TaskType task_id);
extern FUNC(os_status_t, OS_CODE) os_get_task_state(TaskType task_id, TaskStateRefType task_state_ref);
extern FUNC(os_status_t, OS_CODE) os_schedule(void);
extern FUNC(os_status_t, OS_CODE) os_terminate_task(void);
extern FUNC(os_status_t, OS_CODE) os_chain_task(TaskType task_id);
extern FUNC(os_status_t, OS_CODE) os_get_task_id(TaskRefType task_id_ref);
extern FUNC(void, OS_CODE) os_task_reset(const os_task_cfg_t *const task_cfg);
extern FUNC(void, OS_CODE) os_task_preinit(const os_task_cfg_t *const task_cfg);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

static inline_function TaskStateType os_get_task_state_internal(const os_task_inst_t *task_inst)
{
	if (task_inst->thread.tcb.flags & TCB_FLAG_THREAD_IS_SUSPENDED) {
		return SUSPENDED;
	} else {
		if (task_inst->thread.tcb.task_state == TSTATE_WAIT_SEM) {
			return WAITING;
		} else if (task_inst->thread.tcb.task_state == TSTATE_TASK_READYTORUN) {
			return READY;
		} else if (task_inst->thread.tcb.task_state == TSTATE_TASK_RUNNING) {
			return RUNNING;
		} else {
			return INVALID_STATE;
		}
	}
}

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_TASK_INTERNAL_H */
