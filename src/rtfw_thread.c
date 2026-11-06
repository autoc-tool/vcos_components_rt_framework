#include <nuttx/config.h>
#include <nuttx/signal.h>
#include <nuttx/irq.h>
#include <nuttx/sched_note.h>
#include <task/task.h>
#include <sched/sched.h>
#include "setjmp.h"
#include "rtfw_thread_internal.h"
#include "rtfw_task.h"
#include "rtfw_hook.h"
#include "rtfw_core.h"
#include "rtfw_context.h"

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

static noinline_function FUNC(void, OS_CODE) os_thread_enter_task(const os_task_cfg_t *task_cfg, boolean is_privilege)
{
	os_task_inst_t *task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);

	task_inst->thread.tcb.flags &= ~(TCB_FLAG_THREAD_IN_KERNEL_MPU_SET | TCB_FLAG_THREAD_IS_SUSPENDED);

#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	up_mpu_enable_set(OS_TASK_GET_CONTEXTINST_REF(task_cfg)->current_mpu_access_right->set_num);
#endif
#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
	os_set_thread_mode_from_privilege(is_privilege);
#else
	(void)is_privilege;
#endif
	task_cfg->context.arch_ctx_cfg.entry_addr(task_cfg->context.arch_ctx_cfg.entry_param);
}

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
static noinline_function FUNC(void, OS_CODE) os_thread_enter_hook(void)
{
	protection_return_t protection_return;
#if defined(CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK == 1)
	os_hook_thread_inst_t *hook_thread =
		(os_hook_thread_inst_t *)rtfw_shared_hook_thead_ref_table[os_get_core_id()];
#endif
	const os_ctx_cfg_t *current_context = os_context_get_contexthdl();
	const os_hook_cfg_t *hook_cfg = OS_CONTEXT_GET_HOOK_CFG_REF(current_context);
	os_hook_inst_t *hook_inst = OS_HOOK_GET_INSTANCE_REF(hook_cfg);
	struct tcb_s *hook_tcb = NULL_PTR;
#if defined(CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK == 1)
	hook_tcb = &(hook_inst->thread->tcb);
#else
	hook_tcb = &(hook_inst->thread.tcb);
#endif

	if ((hook_tcb->flags & TCB_FLAG_THREAD_IS_SUSPENDED) == 0U) {
		hook_tcb->flags &= ~TCB_FLAG_THREAD_IN_KERNEL_MPU_SET;
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
		up_mpu_enable_set(OS_HOOK_GET_CONTEXTINST_REF(hook_cfg)->current_mpu_access_right->set_num);
#endif
		protection_return = os_hook_call_callback(hook_cfg);
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
		up_mpu_enable_set(0U);
#endif
		hook_inst->return_value.protection_reaction = protection_return;
	}

#if defined(CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK == 1)
	longjmp(hook_thread->jmpbuf, 1);
#else
	longjmp(hook_inst->thread.jmpbuf, 1);
#endif
}
#endif

static noinline_function void os_task_thread_start(int argc, FAR char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	up_irq_disable();
	const os_ctx_cfg_t *current_context = os_context_get_contexthdl();
	const os_task_cfg_t *task_cfg = OS_CONTEXT_GET_TASK_CFG_REF(current_context);
	os_task_inst_t *task_inst = OS_CONTEXT_GET_TASK_INSTANCE_REF(current_context);

	os_task_preinit(task_cfg);
	if (setjmp(task_inst->thread.jmpbuf) == 1) {
		os_task_reset(task_cfg);
	}

	up_irq_enable();
	nxsem_wait_uninterruptible(&(task_inst->thread.act_sem));

	os_thread_enter_task(task_cfg, current_context->app_owner_cfg->is_privileged);

	while (1) {
		/* It is illegal to get here !!!*/
		nxsig_usleep(MAX_DELAY);
	}
}

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
static noinline_function void os_hook_thread_start(int argc, FAR char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

#if defined(CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK == 1)
	os_hook_thread_inst_t *hook_thread =
		(os_hook_thread_inst_t *)rtfw_shared_hook_thead_ref_table[os_get_core_id()];

	if (setjmp(hook_thread->jmpbuf) == 1) {
#else
	const os_ctx_cfg_t *current_context = os_context_get_contexthdl();
	const os_hook_cfg_t *hook_cfg = OS_CONTEXT_GET_HOOK_CFG_REF(current_context);
	os_hook_inst_t *hook_inst = OS_HOOK_GET_INSTANCE_REF(hook_cfg);

	if (setjmp(hook_inst->thread.jmpbuf) == 1) {
#endif
		os_hook_return();
	}

	os_thread_enter_hook();
}
#endif

void rtfw_thread_start(void)
{
	char **argv;
	int argc;
	struct tcb_s *tcb = NULL_PTR;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	os_context_id_t cur_context_id = OS_CONTEXT_GET_INITCONTEXTID(cur_context);
	boolean this_is_task = ((cur_context_id & OS_CONTEXT_ANYTASK) ? TRUE : FALSE);

	DEBUGASSERT((cur_context_id & OS_CONTEXT_ANYTASK) || (cur_context_id & OS_CONTEXT_ANYHOOK));

	if (this_is_task) {
		tcb = &(OS_CONTEXT_GET_TASK_INSTANCE_REF(cur_context)->thread.tcb);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
#if defined(CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_HOOK_STACK == 1)
		tcb = &(OS_CONTEXT_GET_HOOK_INSTANCE_REF(cur_context)->thread->tcb);
#else
		tcb = &(OS_CONTEXT_GET_HOOK_INSTANCE_REF(cur_context)->thread.tcb);
#endif
#endif
	}

	argv = nxsched_get_stackargs(tcb);
	for (argc = 0; argv && argv[argc]; argc++) {
		/* do nothing */
	}

	if (this_is_task) {
		os_task_thread_start(argc, argv);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
		os_hook_thread_start(argc, argv);
#endif
	}
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"