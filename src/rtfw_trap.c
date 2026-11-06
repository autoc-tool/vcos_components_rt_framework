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

#include <inttypes.h>
#include <nuttx/config.h>
#include "rtfw_core_internal.h"
#include "rtfw_isr_internal.h"
#include "rtfw_task_internal.h"
#include "rtfw_hook_internal.h"
#include "rtfw_trap.h"
#include "rtfw_error.h"
#include "rtfw_trap_internal.h"

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

void rtfw_expt_protection_hook(rtfw_protection_status_t rtfw_protection_status)
{
	if(rtfw_protection_status == RTFW_STATUS_PROTECTION_MEMORY) {
		REPORT_PROTECTION_ERROR(OS_STATUS_PROTECTION_MEMORY);
	} else {
		REPORT_PROTECTION_ERROR(OS_STATUS_PROTECTION_EXCEPTION);
	}
}

void rtfw_expt_detail_info_print(void)
{
	os_core_inst_t *core = OS_CORE_GET_CORE_INSTANCE_REF(os_core_cfg_ref_table[os_get_core_id()]);
	os_expt_detail_t expt_detail = core->expt_full_context.expt_detail;

	EXP_INFO("Core %u: Application ID: %lu\r\n", up_cpu_index(), expt_detail.application_id);
	EXP_INFO("Core %u: Thread Type: %u\r\n", up_cpu_index(), expt_detail.thread_type);
	EXP_INFO("Core %u: Thread ID: %lu\r\n", up_cpu_index(), expt_detail.thread_id);
	EXP_INFO("Backtrace Depth: %lu\r\n", expt_detail.backtrace_depth);
	for (uint8_t i = 0U; i < expt_detail.backtrace_depth; i++) {
		EXP_INFO("Backtrace Address[%u]: 0x%lX\r\n", i, expt_detail.backtrace_addr[i]);
	}
}

void rtfw_expt_backtrace_addr_save(uint32_t backtrace_addr, uint8_t backtrace_depth)
{
	os_core_inst_t *core = OS_CORE_GET_CORE_INSTANCE_REF(os_core_cfg_ref_table[os_get_core_id()]);

	core->expt_full_context.expt_detail.backtrace_addr[backtrace_depth] = backtrace_addr;
	core->expt_full_context.expt_detail.backtrace_depth = backtrace_depth + 1;
}

void rtfw_expt_detail_save(void)
{
	os_core_inst_t *core = OS_CORE_GET_CORE_INSTANCE_REF(os_core_cfg_ref_table[os_get_core_id()]);
	os_expt_detail_t *expt_detail = &core->expt_full_context.expt_detail;
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();

	expt_detail->core_id = up_cpu_index();
	expt_detail->application_id = os_get_current_app_id();
	if (os_context_is_any_isr(current_context)) {
		expt_detail->thread_type = os_expt_thread_type_isr;
		expt_detail->thread_id = OS_CONTEXT_GET_CURRENT_ISRID(current_context);
	} else if (os_context_is_any_task(current_context)) {
		expt_detail->thread_type = os_expt_thread_type_task;
		expt_detail->thread_id = OS_CONTEXT_GET_CURRENT_TASKID(current_context);
	} else {
		expt_detail->thread_type = os_expt_thread_type_hook;
		expt_detail->thread_id = OS_CONTEXT_GET_CURRENT_HOOKID(current_context);
	}
}

void rtfw_expt_frame_save(const up_expt_frame_t *expt_frame_args)
{
	os_core_inst_t *core = OS_CORE_GET_CORE_INSTANCE_REF(os_core_cfg_ref_table[os_get_core_id()]);
	os_expt_full_context_t *expt_full_context = &core->expt_full_context;

	(void)memcpy(&(expt_full_context->expt_frame), expt_frame_args, sizeof(up_expt_frame_t));
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
