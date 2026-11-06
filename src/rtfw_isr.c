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
#ifdef CONFIG_IRQ_USE_ARCH_DISPATCH
#include "arch/irq_dispatch.h"
#endif
#include "rtfw_isr.h"
#include "rtfw_core.h"
#include <sched/sched.h>
#include "rtfw_context_internal.h"

#if (CONFIG_ARCH_IRQ_NESTING_LIMIT > 1) && !defined(CONFIG_ARCH_HAVE_IRQ_PRIO_MASK_CONTEXT)
static inline_function irqlevel_t os_pre_isr_prio_mask_save(uint32 irq_nesting, const os_ctx_cfg_t *pre_context)
{
	irqlevel_t irq_prio_mask = 0U;

	if (irq_nesting == 1) {
		irq_prio_mask = up_get_irq_mask_level();
	} else {
		irq_prio_mask = OS_ISR_GET_LEVEL(OS_CONTEXT_GET_ISR_CFG_REF(pre_context));
	}
	return irq_prio_mask;
}

static inline_function void os_isr_irq_prio_mask_restore(uint32 irq_prio_mask)
{
	up_set_irq_mask_level(irq_prio_mask);
}
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
#ifdef CONFIG_IRQ_USE_ARCH_DISPATCH
FUNC(uintptr_t, OS_CODE) rtfw_irq_handler(int irq, void *regs, void *arg)
#else
FUNC(int, OS_CODE) rtfw_irq_handler(int irq, void *regs, void *arg)
#endif
{
	irqlevel_t irq_state;
	uint8_t irq_nesting;
	const os_isr_cfg_t *isr_cfg = (os_isr_cfg_t *)arg;
	const os_ctx_cfg_t *this_context = NULL_PTR;
#if (CONFIG_ARCH_IRQ_NESTING_LIMIT > 1) || defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION)
	const os_ctx_cfg_t *pre_context = os_context_get_contexthdl();
#endif
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	const struct mpu_access_right_s *pre_access_right = NULL_PTR;
	const struct mpu_access_right_s *this_access_right = NULL_PTR;
#endif

#ifdef CONFIG_IRQ_USE_ARCH_DISPATCH
	up_irq_dispatch_enter(regs);
#endif

	UNUSED(irq);
	UNUSED(regs);
	UNUSED(irq_state);
	UNUSED(irq_nesting);

#if (CONFIG_ARCH_IRQ_NESTING_LIMIT > 1)
	irq_nesting = up_get_irq_nesting();
	if (irq_nesting < CONFIG_ARCH_IRQ_NESTING_LIMIT) {
#ifdef CONFIG_RT_FRAMEWORK_INT_LEVEL
#ifndef CONFIG_ARCH_HAVE_IRQ_PRIO_MASK_CONTEXT
		irq_state = os_pre_isr_prio_mask_save(irq_nesting, pre_context);
#endif
		up_set_irq_mask_level(up_get_irq_running_level());
#endif
		up_irq_enable();
	}
#else
#if defined(CONFIG_RT_FRAMEWORK_CAT0_INT_IS_IRQ) && defined(CONFIG_RT_FRAMEWORK_INT_LEVEL)
	irq_state = up_kernel_int_save();
	up_irq_enable();
#endif
#endif /* CONFIG_ARCH_IRQ_NESTING_LIMIT */
	this_context = OS_ISR_GET_CONTEXT_REF(isr_cfg);

	os_context_set_contexthdl(this_context);

#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	this_access_right = OS_CONTEXT_GET_CONTEXTINST_REF(this_context)->current_mpu_access_right;

	if (NULL == pre_context) {
		up_mpu_switch_access_right(&empty_thread_access_right, this_access_right);
	} else {
		pre_access_right = OS_CONTEXT_GET_CONTEXTINST_REF(pre_context)->current_mpu_access_right;
		up_mpu_switch_access_right(pre_access_right, this_access_right);
	}
	up_mpu_enable_set(this_access_right->set_num);
#endif

	isr_cfg->context.arch_ctx_cfg.entry_addr(NULL_PTR);

#if defined(CONFIG_STACK_OVERFLOW_CHECK) && (CONFIG_STACK_OVERFLOW_CHECK == 1)
	if (0U != up_check_intstack_overflow(os_get_core_id())) {
		// TODO 待添加TRAP后处理
	}
#endif

#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	up_mpu_enable_set(0U);
#endif

#if (CONFIG_ARCH_IRQ_NESTING_LIMIT > 1)
	if (irq_nesting < CONFIG_ARCH_IRQ_NESTING_LIMIT) {
		up_irq_disable();
#ifndef CONFIG_ARCH_HAVE_IRQ_PRIO_MASK_CONTEXT
		os_isr_irq_prio_mask_restore(irq_state);
#endif
	}
	if (irq_nesting > 1) {
		os_context_set_contexthdl(pre_context);
	}
#else
#if defined(CONFIG_RT_FRAMEWORK_CAT0_INT_IS_IRQ) && defined(CONFIG_RT_FRAMEWORK_INT_LEVEL)
	up_irq_disable();
#ifndef CONFIG_ARCH_HAVE_IRQ_PRIO_MASK_CONTEXT
	up_kernel_int_restore(irq_state);
#endif
#endif /* CONFIG_RT_FRAMEWORK_CAT0_INT_IS_IRQ */
#endif /* CONFIG_ARCH_IRQ_NESTING_LIMIT */

#ifdef CONFIG_IRQ_USE_ARCH_DISPATCH
	regs = up_irq_dispatch_exit(regs);
	return (uintptr_t)regs;
#else
	return 0;
#endif
}

FUNC(void, OS_CODE) rtfw_isr_init(const os_isr_cfg_t *const os_isr_cfg_table[], OsObjIndexType size)
{
	const os_isr_cfg_t *isr_cfg;
	os_isr_inst_t *isrInst;
	OsObjIndexType index;
	const os_ctx_cfg_t *ctx_cfg;
	const os_arch_isr_cfg_t *arch_ctx_cfg;
	CoreIdType core_id = os_get_core_id();

	if ((os_isr_cfg_table == NULL) || (size == (OsObjIndexType)0)) {
		return;
	}

	for (index = 0U; index < size; index++) {
		isr_cfg = os_isr_cfg_table[index];
		if (isr_cfg == NULL) {
			continue;
		}
		ctx_cfg = OS_ISR_GET_CONTEXT_REF(isr_cfg);
		if ((OS_CONTEXT_GET_OWNER_COREID(ctx_cfg) != core_id)) {
			continue;
		}
		arch_ctx_cfg = &(isr_cfg->arch_isr_cfg);
		isrInst = OS_ISR_GET_INSTANCE_REF(isr_cfg);
		isrInst->top_of_stack = NULL_PTR;
		os_context_init_context_instance(ctx_cfg);
#ifndef CONFIG_IRQ_USE_ARCH_DISPATCH
		irq_attach((int)(arch_ctx_cfg->irq), rtfw_irq_handler, (void *)isr_cfg);
#endif
		up_init_irq((int)(arch_ctx_cfg->vector_num), (int)(arch_ctx_cfg->level));
		if (isr_cfg->is_enabled_on_init) {
			up_enable_irq((int)(arch_ctx_cfg->vector_num));
		}
	}
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
