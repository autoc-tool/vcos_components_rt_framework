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
#include <nuttx/event.h>
#include <nuttx/signal.h>
#include <nuttx/sched_note.h>
#include <sched/sched.h>
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
#include "rtfw_ipicall.h"
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
#include "rtfw_task.h"
#include "rtfw_core.h"
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
#include "rtfw_syscall_internal.h"
#endif
#include "rtfw_thread_internal.h"
#if (defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)) ||                                  \
	(defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1))
#include "rtfw_lock_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE || CONFIG_RT_FRAMEWORK_SPINLOCK */

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) os_task_preinit(const os_task_cfg_t *const task_cfg)
{
	os_task_inst_t *task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);
	sem_t *sem = &(task_inst->thread.act_sem);

	if (task_cfg->autostart_mode) {
		sem->semcount += 2;
	} else {
		sem->semcount += 1;
	}
	if (sem->semcount > sem->sem_max_value + 1) {
		sem->semcount = sem->sem_max_value + 1;
	}
}

FUNC(void, OS_CODE) os_task_reset(const os_task_cfg_t *const task_cfg)
{
#if defined(CONFIG_SCHED_INSTRUMENTATION) || (CONFIG_RT_FRAMEWORK_TASK_MULTIPLE_ACTIVATION != 1)
	os_task_inst_t *task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);
#endif

	if (LIKELY(NULL_PTR != task_cfg->event_hdl)) {
		(void)nxevent_clear(&(task_cfg->event_hdl->event), NXEVENT_ALL_MASK);
	}
#if (CONFIG_RT_FRAMEWORK_TASK_MULTIPLE_ACTIVATION != 1)
	nxsem_reset(&(task_inst->thread.act_sem), 0);
#endif

#ifdef CONFIG_SCHED_INSTRUMENTATION
	if ((task_inst->thread.act_sem).semcount >= 1) {
		sched_note_reactivate(&(task_inst->thread.tcb));
	}
#endif
}

FUNC(void, OS_CODE) os_task_init(const os_task_cfg_t *const os_task_cfg_table[], OsObjIndexType size)
{
	const os_task_cfg_t *task_cfg;
	os_task_inst_t *task_inst;
	OsObjIndexType index;
	const os_ctx_cfg_t *context_cfg;
	CoreIdType core_id = os_get_core_id();
	int ret;

	if ((os_task_cfg_table == NULL) || (size == (OsObjIndexType)0)) {
		return;
	}

	for (index = 0U; index < size; index++) {
		task_cfg = os_task_cfg_table[index];
		if ((task_cfg == NULL)) {
			continue;
		}
		context_cfg = OS_TASK_GET_CONTEXT_REF(task_cfg);
		task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(context_cfg);
		if ((task_inst == NULL) || (OS_CONTEXT_GET_OWNER_COREID(context_cfg) != core_id)) {
			continue;
		}

		os_context_init_context_instance(context_cfg);

		if (LIKELY(NULL_PTR != task_cfg->event_hdl)) {
			(void)nxevent_clear(&(task_cfg->event_hdl->event), NXEVENT_ALL_MASK);
		}

		task_inst->thread.tcb.flags = TCB_FLAG_TTYPE_KERNEL | TCB_FLAG_TASK_THREAD |
					      TCB_FLAG_THREAD_ENABLE_IRQ | TCB_FLAG_THREAD_IN_KERNEL_MPU_SET |
					      TCB_FLAG_THREAD_IS_SUSPENDED;

		task_inst->thread.tcb.contexthdl = (void *)context_cfg;

#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
		up_mpu_enable_stack_region((uint32_t)(task_cfg->context.arch_ctx_cfg.stack_addr),
					   (uint32_t)(task_cfg->context.arch_ctx_cfg.stack_addr) +
						   task_cfg->context.arch_ctx_cfg.stack_size);
#endif

#if defined(CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK == 1)
		task_inst->thread.tcb.share_stack_info = OS_TASK_GET_TASK_SHARE_STACK_REF(task_cfg);
#endif
		ret = nxtask_init((struct task_tcb_s *)&(task_inst->thread.tcb), task_cfg->context.instance_name,
				  (int)task_cfg->base_priority, (void *)task_cfg->context.arch_ctx_cfg.stack_addr,
				  task_cfg->context.arch_ctx_cfg.stack_size, NULL, NULL, NULL, NULL);
		if (ret < OK) {
			continue;
		}

		if (NULL_PTR != task_cfg->event_hdl) {
			nxevent_init(&(task_cfg->event_hdl->event), 0);
			nxsem_init(&(task_cfg->event_hdl->wait.sem), 0, 0);
		}

		if (task_cfg->is_preemptive == FALSE) {
			task_inst->thread.tcb.lockcount = 1;
		}

		nxsem_init(&(task_inst->thread.act_sem), 0, 0);

		task_inst->thread.act_sem.semcount = -1;

		task_inst->thread.act_sem.sem_max_value = task_cfg->activation_limit - 1;

		nxtask_activate(&(task_inst->thread.tcb));
	}
}

FUNC(os_status_t, OS_CODE) os_activate_task(TaskType task_id)
{
#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS) || defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();
#endif
	const os_task_cfg_t *task_cfg;
	const os_ctx_cfg_t *task_context;
	os_task_inst_t *task_inst;
	CoreIdType task_core_id;
	int ret;

#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS)
	if (UNLIKELY(FALSE == os_context_check_curcontext_id(current_context, OS_CONTEXT_ACTIVATETASK))) {
		return OS_STATUS_CALLEVEL;
	}
	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#endif

	task_cfg = os_task_cfg_ref_table[task_id];
	task_context = OS_TASK_GET_CONTEXT_REF(task_cfg);
	task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);
	task_core_id = OS_CONTEXT_GET_OWNER_COREID(task_context);

#if defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	ApplicationIDType app_id = OS_CONTEXT_GET_CURRENT_APPID(current_context);
	AccessApplMaskType target_app_mask = OS_CONTEXT_GET_ACCESSINGAPPLMASK(task_context);

	if (UNLIKELY(os_app_check_access(app_id, target_app_mask) != OS_STATUS_OK)) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif

	if (task_core_id == os_get_core_id()) {
#if defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
		if (UNLIKELY(os_app_check_is_accessible(OS_CONTEXT_GET_APPONRWECFG_REF(task_context)) !=
			     OS_STATUS_OK)) {
			return OS_STATUS_NOT_ACCESSIBLE;
		}
#endif
		ret = nxsem_post(&(task_inst->thread.act_sem));
		if (ret == -EOVERFLOW) {
			return OS_STATUS_LIMIT;
		} else {
			return (ret == OK ? OS_STATUS_OK : OS_STATUS_NOT_OK);
		}
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_activate_task((uint32)task_core_id, task_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_terminate_task(void)
{
	const os_ctx_cfg_t *current_context;
	os_task_inst_t *task_inst;
	os_status_t status = OS_STATUS_OK;

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	os_set_thread_mode_to_privilege();
#endif
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	up_mpu_enable_set(0U);
#endif
	current_context = os_context_get_contexthdl();

#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS)
	if (UNLIKELY(FALSE == os_context_check_curcontext_id(current_context, OS_CONTEXT_TERMINATETASK))) {
		status = OS_STATUS_CALLEVEL;
	} else if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		status = OS_STATUS_DISABLEDINT;
	} else {
		status = os_context_check_locklist(current_context);
	}
#endif

	if (UNLIKELY(status != OS_STATUS_OK)) {
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
		up_mpu_enable_set(((os_ctx_inst_t *)(current_context->instance))->current_mpu_access_right->set_num);
#endif
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
		os_set_thread_mode_from_privilege(current_context->app_owner_cfg->is_privileged);
#endif
		return status;
	}
	up_irq_disable();
	task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(current_context);
	task_inst->thread.tcb.flags |= (TCB_FLAG_THREAD_IN_KERNEL_MPU_SET | TCB_FLAG_THREAD_IS_SUSPENDED);

	longjmp(task_inst->thread.jmpbuf, 1);
	/* not be executed */
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_activate_task_asyn(TaskType task_id)
{
	const os_task_cfg_t *task_cfg;
	const os_ctx_cfg_t *task_context;
	CoreIdType task_core_id;
	os_task_inst_t *task_inst;
	int ret;

	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	task_cfg = os_task_cfg_ref_table[task_id];

	task_context = OS_TASK_GET_CONTEXT_REF(task_cfg);
	task_core_id = OS_CONTEXT_GET_OWNER_COREID(task_context);
	task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);

#if defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	const os_ctx_cfg_t *const current_ctx = os_context_get_contexthdl();

	ApplicationIDType app_id = OS_CONTEXT_GET_CURRENT_APPID(current_ctx);
	AccessApplMaskType target_app_mask = OS_CONTEXT_GET_ACCESSINGAPPLMASK(task_context);

	if (UNLIKELY(os_app_check_access(app_id, target_app_mask) != OS_STATUS_OK)) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif

	if (task_core_id == os_get_core_id()) {
#if defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
		if (UNLIKELY(os_app_check_is_accessible(OS_CONTEXT_GET_APPONRWECFG_REF(task_context)) !=
			     OS_STATUS_OK)) {
			return OS_STATUS_NOT_ACCESSIBLE;
		}
#endif
		ret = nxsem_post(&(task_inst->thread.act_sem));
		if (ret == -EOVERFLOW) {
			return OS_STATUS_LIMIT;
		} else {
			return (ret == OK ? OS_STATUS_OK : OS_STATUS_NOT_OK);
		}
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_activate_task_asyn((uint32)task_core_id, task_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_chain_task(TaskType task_id)
{
	int ret = 0;

#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS) || defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();
#endif

#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS)
	if (UNLIKELY(os_context_check_curcontext_id(current_context, OS_CONTEXT_CHAINTASK) == (boolean)FALSE)) {
		return OS_STATUS_CALLEVEL;
	}

	if (UNLIKELY((task_id >= os_task_cfg_ref_table_size) || (NULL == os_task_cfg_ref_table[task_id]))) {
		return OS_STATUS_ID_1;
	}

	if (UNLIKELY(os_interrupt_is_disabled() == (boolean)TRUE)) {
		return OS_STATUS_DISABLEDINT;
	}

#if (defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)) ||                                  \
	(defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1))
	if (UNLIKELY(OS_LIST_IS_EMPTY(OS_CONTEXT_GET_LOCKLIST_REF(current_context)) == FALSE)) {
		const os_list_node_t *last_node = OS_LIST_LAST(OS_CONTEXT_GET_LOCKLIST_REF(current_context));

		if (((OS_LOCK_CFG_PTR_GET(last_node))->type) == OS_LOCK_TYPE_SPINLOCK) {
			return OS_STATUS_SPINLOCK;
		} else {
			return OS_STATUS_RESOURCE;
		}
	}
#endif
#endif /* CONFIG_RT_FRAMEWORK_EXTENDED_STATUS */

#if defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	if (UNLIKELY(os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(current_context),
					 OS_TASK_GET_ACCESSINGAPPLMASK(os_task_cfg_ref_table[task_id])) !=
		     OS_STATUS_OK)) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK*/

	const os_task_cfg_t *task_cfg = os_task_cfg_ref_table[task_id];
	const os_ctx_cfg_t *task_context = OS_TASK_GET_CONTEXT_REF(task_cfg);
	os_task_inst_t *task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);
	CoreIdType task_core_id = OS_CONTEXT_GET_OWNER_COREID(task_context);

	if (task_core_id == os_get_core_id()) {
#if defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
		if (UNLIKELY(os_app_check_is_accessible(OS_CONTEXT_GET_APPONRWECFG_REF(task_context)) !=
			     OS_STATUS_OK)) {
			return OS_STATUS_NOT_ACCESSIBLE;
		}
#endif
		ret = nxsem_post(&(task_inst->thread.act_sem));
		if (ret == -EOVERFLOW) {
			return OS_STATUS_LIMIT;
		} else {
			return (ret == OK ? OS_STATUS_OK : OS_STATUS_NOT_OK);
		}
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_activate_task((uint32)task_core_id, task_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}
}

FUNC(os_status_t, OS_CODE) os_get_task_id(TaskRefType task_id_ref)
{
	os_status_t ret = OS_STATUS_OK;
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();

	if (UNLIKELY(task_id_ref == NULL)) {
		ret = OS_STATUS_PARAM_POINTER_1;
	} else if (UNLIKELY(os_context_check_curcontext_id(current_context, OS_CONTEXT_GETTASKID) == (boolean)FALSE)) {
		ret = OS_STATUS_CALLEVEL;
	} else if (UNLIKELY(os_interrupt_is_disabled() == (boolean)TRUE)) {
		ret = OS_STATUS_DISABLEDINT;
	} else {
		const struct tcb_s *current_tcb = g_running_tasks[this_cpu()];
		const os_ctx_cfg_t *const current_task_context = current_tcb->contexthdl;
		const os_task_cfg_t *current_task_cfg = OS_CONTEXT_GET_TASK_CFG_REF(current_task_context);

		(*task_id_ref) = OS_TASK_GET_TASKID(current_task_cfg);

		ret = OS_STATUS_OK;
	}

	return ret;
}

FUNC(os_status_t, OS_CODE) os_get_task_state(TaskType task_id, TaskStateRefType task_state_ref)
{
	os_status_t ret = OS_STATUS_OK;
#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS) || defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();
#endif

#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS)
	if (UNLIKELY(FALSE == os_context_check_curcontext_id(current_context, OS_CONTEXT_GETTASKSTATE))) {
		return OS_STATUS_CALLEVEL;
	}
	if (UNLIKELY(task_state_ref == NULL_PTR)) {
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#endif

	const os_task_cfg_t *task_cfg = os_task_cfg_ref_table[task_id];
	const os_ctx_cfg_t *task_context = OS_TASK_GET_CONTEXT_REF(task_cfg);
	os_task_inst_t *task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);
	CoreIdType task_core_id = OS_CONTEXT_GET_OWNER_COREID(task_context);

	if (task_core_id == os_get_core_id()) {
#if defined(CONFIG_RT_FRAMEWORK_ACCESSCHECK)
		if (UNLIKELY(os_app_check_is_accessible(OS_CONTEXT_GET_APPONRWECFG_REF(task_context)) !=
			     OS_STATUS_OK)) {
			return OS_STATUS_NOT_ACCESSIBLE;
		}
#endif
		*task_state_ref = os_get_task_state_internal(task_inst);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL)
		return os_ipicall_get_task_state((uint32)task_core_id, task_id, task_state_ref);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return ret;
}

FUNC(os_status_t, OS_CODE) os_schedule(void)
{
	os_status_t ret = OS_STATUS_OK;

#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS) || defined(CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK)
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();
#endif

#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS)
	if (UNLIKELY(FALSE == os_context_check_curcontext_id(current_context, OS_CONTEXT_SCHEDULE))) {
		return OS_STATUS_CALLEVEL;
	}
	if (UNLIKELY(os_interrupt_is_disabled() == (boolean)TRUE)) {
		return OS_STATUS_DISABLEDINT;
	}
#endif

#if defined(CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK == 1)
	const os_task_cfg_t *task_cfg = os_task_cfg_ref_table[OS_CONTEXT_GET_CURRENT_TASKID(current_context)];

	if (task_cfg->task_share_stack != NULL_PTR) {
		return OS_STATUS_CALLEVEL_SHARED_STACK;
	}
#endif
#if defined(CONFIG_RT_FRAMEWORK_EXTENDED_STATUS)
	ret = os_context_check_locklist(current_context);
	if (ret != OS_STATUS_OK) {
		return ret;
	}
#endif

	sched_yield();

	return ret;
}

FUNC(os_status_t, OS_CODE) get_task_app_id(ApplicationIDType *appid_ref)
{
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();

	if (UNLIKELY(appid_ref == NULL_PTR)) {
		return OS_STATUS_PARAM_POINTER_1;
	}

	if (UNLIKELY(os_context_check_curcontext_id(current_context, OS_CONTEXT_GETTASKID) == (boolean)FALSE)) {
		return OS_STATUS_CALLEVEL;
	}

	if (UNLIKELY(os_interrupt_is_disabled() == (boolean)TRUE)) {
		return OS_STATUS_DISABLEDINT;
	}

	*appid_ref = OS_CONTEXT_GET_CURRENT_APPID(current_context);

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) rtfw_task_tickdelay(TickType ticks_to_delay)
{
	os_status_t ret = OS_STATUS_OK;
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();

	if (UNLIKELY(os_context_check_curcontext_id(current_context, OS_CONTEXT_TASK_TICKDELAY) == (boolean)FALSE)) {
		return OS_STATUS_CALLEVEL;
	}

	if (UNLIKELY(os_interrupt_is_disabled() == (boolean)TRUE)) {
		return OS_STATUS_DISABLEDINT;
	}

#if defined(CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK == 1)
	if (OS_TASK_GET_TASK_SHARE_STACK_REF((const os_task_cfg_t *const)current_context) != NULL_PTR) {
		return OS_STATUS_CALLEVEL_SHARED_STACK;
	}
#endif

	ret = os_context_check_locklist(current_context);
	if (LIKELY(ret == OS_STATUS_OK)) {
		(void)nxsig_usleep(TICK2USEC(ticks_to_delay));
	}

	return ret;
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
