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
#include <nuttx/config.h>
#include <nuttx/arch.h>
#include "rtfw_interrupt.h"
#include "rtfw_core_internal.h"
#include "rtfw_context_internal.h"
#include "rtfw_isr.h"
#include "isr_cfg.h"
#include "rtfw_error.h"

/* Local Function Implementation */

__percpu_bss os_int_state_t core_int_state;

#define core_int_state this_cpu_var(core_int_state)

static inline_function void __os_int_set_disable_all_state(uint8 * const state)
{
	*state = INT_DISABLE_ALL_STATE_DISABLED;
}

static inline_function void __os_int_set_enable_all_state(uint8 *const state)
{
	*state = INT_DISABLE_ALL_STATE_ENABLED;
}

static inline_function void __os_int_suspend_cat1(irqlevel_t *const level)
{
#if defined(CONFIG_RT_FRAMEWORK_CAT1_INT_LEVEL_CONTROL) && (CONFIG_RT_FRAMEWORK_CAT1_INT_LEVEL_CONTROL == 1)
	*level = up_irq_level_save(CONFIG_RT_FRAMEWORK_MAX_CAT1_INT_LEVEL);
#else
	*level = up_irq_save();
#endif
}

static inline_function void __os_int_resume_cat1(const irqlevel_t *const level)
{
#if defined(CONFIG_RT_FRAMEWORK_CAT1_INT_LEVEL_CONTROL) && (CONFIG_RT_FRAMEWORK_CAT1_INT_LEVEL_CONTROL == 1)
	up_irq_level_restore(*level);
#else
	up_irq_restore(*level);
#endif
}

static inline_function void __os_int_suspend_cat2(irqlevel_t *const level)
{
#if defined(CONFIG_RT_FRAMEWORK_CAT2_INT_LEVEL_CONTROL) && (CONFIG_RT_FRAMEWORK_CAT2_INT_LEVEL_CONTROL == 1)
	*level = up_irq_level_save(CONFIG_RT_FRAMEWORK_MAX_CAT2_INT_LEVEL);
#else
	*level = up_irq_save();
#endif
}

static inline_function void __os_int_resume_cat2(const irqlevel_t *const level)
{
#if defined(CONFIG_RT_FRAMEWORK_CAT2_INT_LEVEL_CONTROL) && (CONFIG_RT_FRAMEWORK_CAT2_INT_LEVEL_CONTROL == 1)
	up_irq_level_restore(*level);
#else
	up_irq_restore(*level);
#endif
}

static inline_function boolean __os_interrupt_disabled(os_int_state_t *const this_int_state)
{
	return ((boolean)((this_int_state->int_disable_all_state |
					   this_int_state->int_suspend_cat1_cnt |
					   this_int_state->int_suspend_cat2_cnt) != 0u));
}

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) os_disable_all_interrupts(void)
{
	os_int_state_t *const this_int_state = &core_int_state;

	if (UNLIKELY(__os_interrupt_disabled(this_int_state) == TRUE)) {
		REPORT_SERVICE_ERROR(OSServiceId_DisableAllInterrupts, OS_STATUS_INT_API_ORDER, 0);
		return;
	}

	__os_int_suspend_cat1(&(this_int_state->int_level_state_cat1));
	__os_int_set_disable_all_state(&(this_int_state->int_disable_all_state));
}

FUNC(void, OS_CODE) os_enable_all_interrupts(void)
{
	os_int_state_t *const this_int_state = &core_int_state;

	if (UNLIKELY(this_int_state->int_disable_all_state == INT_DISABLE_ALL_STATE_ENABLED)) {
		REPORT_SERVICE_ERROR(OSServiceId_EnableAllInterrupts, OS_STATUS_INT_API_ORDER, 0);
		return;
	}

	__os_int_set_enable_all_state(&(this_int_state->int_disable_all_state));
	__os_int_resume_cat1(&(this_int_state->int_level_state_cat1));
}

FUNC(void, OS_CODE) os_suspend_all_interrupts(void)
{
	os_int_state_t *const this_int_state = &core_int_state;
	os_int_api_cnt_t int_suspend_cnt;

	if (UNLIKELY(this_int_state->int_disable_all_state == INT_DISABLE_ALL_STATE_DISABLED)) {
		REPORT_SERVICE_ERROR(OSServiceId_SuspendAllInterrupts, OS_STATUS_INT_API_ORDER, 0);
		return;
	}

	int_suspend_cnt = this_int_state->int_suspend_cat1_cnt;

	if (LIKELY(int_suspend_cnt == 0u)) {
		__os_int_suspend_cat1(&(this_int_state->int_level_state_cat1));
		this_int_state->int_suspend_cat2_record = this_int_state->int_suspend_cat2_cnt;
		this_int_state->int_suspend_cat1_cnt = 1u;
	} else {
		++int_suspend_cnt;
		if (UNLIKELY(int_suspend_cnt == 0u)) {
			REPORT_SERVICE_ERROR(OSServiceId_SuspendAllInterrupts, OS_STATUS_INT_API_OVERFLOW, 0);
			return;
		}
		this_int_state->int_suspend_cat1_cnt = int_suspend_cnt;
	}
}

FUNC(void, OS_CODE) os_resume_all_interrupts(void)
{
	os_int_state_t *const this_int_state = &core_int_state;
	os_int_api_cnt_t int_suspend_cnt = this_int_state->int_suspend_cat1_cnt;

	if (UNLIKELY(int_suspend_cnt == 0u)) {
		REPORT_SERVICE_ERROR(OSServiceId_ResumeAllInterrupts, OS_STATUS_INT_API_ORDER, 0);
		return;
	}

	if (LIKELY(int_suspend_cnt == 1u)) {
		this_int_state->int_suspend_cat1_cnt = 0u;
		if (LIKELY(this_int_state->int_suspend_cat2_record == this_int_state->int_suspend_cat2_cnt)) {
			__os_int_resume_cat1(&(this_int_state->int_level_state_cat1));
		} else {
			REPORT_SERVICE_ERROR(OSServiceId_ResumeAllInterrupts, OS_STATUS_INT_API_ORDER, 0);
		}
	} else {
		this_int_state->int_suspend_cat1_cnt = int_suspend_cnt - 1;
	}
}

FUNC(void, OS_CODE) os_suspend_os_interrupts(void)
{
	os_int_state_t *const this_int_state = &core_int_state;
	os_int_api_cnt_t int_suspend_cnt;

	if (UNLIKELY(this_int_state->int_disable_all_state == INT_DISABLE_ALL_STATE_DISABLED)) {
		REPORT_SERVICE_ERROR(OSServiceId_SuspendOSInterrupts, OS_STATUS_INT_API_ORDER, 0);
		return;
	}

	int_suspend_cnt = this_int_state->int_suspend_cat2_cnt;
	if (LIKELY(int_suspend_cnt == 0u)) {
		__os_int_suspend_cat2(&(this_int_state->int_level_state_cat2));
		this_int_state->int_suspend_cat1_record = this_int_state->int_suspend_cat1_cnt;
		this_int_state->int_suspend_cat2_cnt = 1u;
	} else {
		++int_suspend_cnt;
		if (UNLIKELY(int_suspend_cnt == 0u)) {
			REPORT_SERVICE_ERROR(OSServiceId_SuspendOSInterrupts, OS_STATUS_INT_API_OVERFLOW, 0);
			return;
		}
		this_int_state->int_suspend_cat2_cnt = int_suspend_cnt;
	}
}

FUNC(void, OS_CODE) os_resume_os_interrupts(void)
{
	os_int_state_t *const this_int_state = &core_int_state;
	os_int_api_cnt_t int_suspend_cnt = this_int_state->int_suspend_cat2_cnt;

	if (UNLIKELY(int_suspend_cnt == 0U)) {
		REPORT_SERVICE_ERROR(OSServiceId_ResumeOSInterrupts, OS_STATUS_INT_API_ORDER, 0);
		return;
	}

	if (LIKELY(int_suspend_cnt == 1u)) {
		this_int_state->int_suspend_cat2_cnt = 0u;
		if (LIKELY(this_int_state->int_suspend_cat1_record == this_int_state->int_suspend_cat1_cnt)) {
			__os_int_resume_cat2(&(this_int_state->int_level_state_cat2));
		} else {
			REPORT_SERVICE_ERROR(OSServiceId_ResumeOSInterrupts, OS_STATUS_INT_API_ORDER, 0);
		}
	} else {
		this_int_state->int_suspend_cat2_cnt = int_suspend_cnt - 1;
	}
}

FUNC(boolean, OS_CODE) os_interrupt_is_disabled(void)
{
	os_int_state_t *const this_int_state = &core_int_state;

	return __os_interrupt_disabled(this_int_state);
}

FUNC(void, OS_CODE) os_int_suspend_kernel(irqlevel_t *const irq_level)
{
	*irq_level = up_kernel_int_save();
}

FUNC(void, OS_CODE) os_int_resume_kernel(const irqlevel_t *const irq_level)
{
	up_kernel_int_restore(*irq_level);
}

FUNC(void, OS_CODE) os_int_disable_global(void)
{
	up_irq_disable();
}

FUNC(void, OS_CODE) os_int_enable_global(void)
{
	up_irq_enable();
}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
static FUNC(os_status_t, OS_CODE) os_interrupt_access_check(const os_isr_cfg_t *IsrCfgRef)
{
	const os_ctx_cfg_t *const current_ctx = os_context_get_contexthdl();

	if (OS_STATUS_OK !=
	    os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(current_ctx), IsrCfgRef->context.access_app_mask)) {
		return OS_STATUS_ACCESS_RIGHT;
	} else {
		return OS_STATUS_OK;
	}
}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

FUNC(os_status_t, OS_CODE) os_enable_interrupt_source(isr_id_t isr_id, boolean clear_pending)
{
	os_status_t status = OS_STATUS_NOT_OK;

	/* Only if OsStatus is EXTENDED */
	if (isr_id >= OS_ISR_CNT) {
		status = OS_STATUS_ID_1;
	}
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	else if (os_app_check_is_accessible(GET_APP_CFG_BY_ISR_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_ACCESS_RIGHT;
	}
#endif
	else {
		uint32 IrqNumber = OS_ISR_GET_ISRREF_BY_ID(isr_id)->arch_isr_cfg.vector_num;

		if (up_irq_is_enabled((int)IrqNumber)) {
			status = OS_STATUS_ORDER;
		}
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
		else if ((os_get_curcontext_id() & OS_CONTEXT_ISRENABLESOURCE) == 0U) {
			status = OS_STATUS_CALLEVEL;
		}
#endif
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		else if (os_interrupt_access_check(OS_ISR_GET_ISRREF_BY_ID(isr_id)) != OS_STATUS_OK) {
			status = OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
		else {
			if (clear_pending) {
				up_clear_irq((int)IrqNumber);
			}

			up_enable_irq((int)IrqNumber);
			status = OS_STATUS_OK;
		}
	}

	return status;
}

FUNC(os_status_t, OS_CODE) os_disable_interrupt_source(isr_id_t isr_id)
{
	os_status_t status = OS_STATUS_NOT_OK;

	if (isr_id >= OS_ISR_CNT) {
		status = OS_STATUS_ID_1;
	}
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	else if (os_app_check_is_accessible(GET_APP_CFG_BY_ISR_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_ACCESS_RIGHT;
	}
#endif
	else {
		uint32 IrqNumber = OS_ISR_GET_ISRREF_BY_ID(isr_id)->arch_isr_cfg.vector_num;

		if (!up_irq_is_enabled((int)IrqNumber)) {
			status = OS_STATUS_ORDER;
		}
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
		else if ((os_get_curcontext_id() & OS_CONTEXT_ISRDISABLESOURCE) == 0U) {
			status = OS_STATUS_CALLEVEL;
		}
#endif
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		else if (os_interrupt_access_check(OS_ISR_GET_ISRREF_BY_ID(isr_id)) != OS_STATUS_OK) {
			status = OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
		else {
			up_disable_irq((int)IrqNumber);
			status = OS_STATUS_OK;
		}
	}

	/* Call ErrorHook if error happens */
	return status;
}

FUNC(os_status_t, OS_CODE) os_clear_pending_interrupt(isr_id_t isr_id)
{
	os_status_t status = OS_STATUS_NOT_OK;

	if (isr_id >= OS_ISR_CNT) {
		status = OS_STATUS_ID_1;
	}
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
	else if ((os_get_curcontext_id() & OS_CONTEXT_ISRDISABLESOURCE) == 0U) {
		status = OS_STATUS_CALLEVEL;
	}
#endif
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	else if (os_app_check_is_accessible(GET_APP_CFG_BY_ISR_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_ACCESS_RIGHT;
	} else if (os_interrupt_access_check(OS_ISR_GET_ISRREF_BY_ID(isr_id)) != OS_STATUS_OK) {
		status = OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */
	else {
		uint32 IrqNumber = OS_ISR_GET_ISRREF_BY_ID(isr_id)->arch_isr_cfg.vector_num;

		up_clear_irq((int)IrqNumber);
		status = OS_STATUS_OK;
	}

	return status;
}

FUNC(void, OS_CODE) os_disable_level_km(void)
{
	up_irq_disable();
}

FUNC(void, OS_CODE) os_enable_level_km(void)
{
	up_irq_enable();
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
