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
#include "rtfw_core_internal.h"
#include "rtfw_error_internal.h"
#include "rtfw_task_internal.h"
#include "rtfw_isr_internal.h"
#include "rtfw_context_internal.h"
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
#include "rtfw_hook_internal.h"
#endif

static inline_function void set_last_error_info(struct os_error_info_s *error_info, os_status_t status)
{
	const os_ctx_cfg_t *context = os_context_get_contexthdl();

	error_info->detailed_status = status;
	error_info->status = (StatusType) status;
	error_info->app_id = OS_CONTEXT_GET_CURRENT_APPID(context);
	error_info->context_id = OS_CONTEXT_GET_INITCONTEXTID(context);
	if ((OS_CONTEXT_ANYISR & error_info->context_id) != 0U) {
		error_info->call_id.isr_id = OS_CONTEXT_GET_CURRENT_ISRID(context);
	} else if ((OS_CONTEXT_ANYTASK & error_info->context_id) != 0U) {
		error_info->call_id.task_id = OS_CONTEXT_GET_CURRENT_TASKID(context);
	} else {
		/* Hook context or startup context: call_id field is not applicable */
	}
	/* Clear all saved parameters */
	(void)memset(error_info->parameter, 0, sizeof(error_info->parameter));
}

#if defined(CONFIG_RT_FRAMEWORK_PROTECTION_HOOK) && (CONFIG_RT_FRAMEWORK_PROTECTION_HOOK == 1)
static inline_function const os_ctx_cfg_t *get_terminateable_context(void)
{
	os_context_cnt_t ctx_cnt;
	const os_core_cfg_t *core;
	struct os_context_nest *ctx_nest;
	const os_ctx_cfg_t *context = os_context_get_contexthdl();

	if ((context->initial_ctx_id & OS_CONTEXT_TERMINATEABLE) == 0U) {
		core = os_core_cfg_ref_table[os_get_core_id()];
		ctx_nest = OS_CORE_GET_CONTEXTNEST_REF(core);
		ctx_cnt = ctx_nest->count;

		while (ctx_cnt-- > 0) {
			if ((ctx_nest->ctx_ref[ctx_cnt]->initial_ctx_id & OS_CONTEXT_TERMINATEABLE) != 0U) {
				return ctx_nest->ctx_ref[ctx_cnt];
			}
		}
	}

	return context;
}

static inline_function protection_return_t verify_protection_response(const os_ctx_cfg_t *context,
																	  const protection_return_t response,
																	  const StatusType response_status)
{
	const os_app_cfg_t *app_cfg;
	const os_app_cfg_t *kernel_app_cfg;

	DEBUGASSERT(response < OS_PROTECTION_REACTION_COUNT);

	if ((response == PRO_IGNORE) ^ (response_status == E_OS_PROTECTION_ARRIVAL)) {
		return PRO_SHUTDOWN;
	}

	if ((response_status == E_OS_PROTECTION_EXCEPTION)) {
		return PRO_SHUTDOWN;
	}

	if (response != PRO_IGNORE_EXCEPTION) {
		if ((response_status == E_OS_PROTECTION_MEMORY) ||
			(os_interrupt_is_disabled())) {
				return PRO_SHUTDOWN;
			}
	} else if (response_status != E_OS_PROTECTION_MEMORY) {
		return PRO_SHUTDOWN;
	}

	if (response == PRO_TERMINATETASKISR) {
		if ((!os_context_is_any_task(context)) && (!os_context_is_any_isr(context))) {
			return PRO_TERMINATEAPPL;
		}
	}

	if (response == PRO_TERMINATEAPPL_RESTART) {
		/* TODO: if restart app is supported, SWS_Os_00557 should be followed. */
		return PRO_SHUTDOWN;
	}

	if (response == PRO_TERMINATEAPPL) {
		app_cfg = OS_CONTEXT_GET_CURRENTAPPCFG_REF(context);
		kernel_app_cfg = OS_CORE_GET_KERNEL_APP(OS_CONTEXT_GET_CORE_REF(context));
		if (app_cfg == NULL || app_cfg == kernel_app_cfg) {
			return PRO_SHUTDOWN;
		}
	}

	return response;
}

static inline void handle_protection_response(protection_return_t response, StatusType response_status)
{
	const os_ctx_cfg_t *context = get_terminateable_context();

	response = verify_protection_response(context, response, response_status);

	switch (response) {
		case PRO_IGNORE:
			break;

		case PRO_IGNORE_EXCEPTION:
			break;

		case PRO_TERMINATETASKISR:
			break;

		case PRO_TERMINATEAPPL:
			break;

		case PRO_TERMINATEAPPL_RESTART:
			break;

		case PRO_SHUTDOWN:
			break;

		default:
			break;
	}
}
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) os_error_init(void)
{
	uint32 idx;
	os_core_inst_t *core = OS_CORE_GET_CORE_INSTANCE_REF(os_core_cfg_ref_table[os_get_core_id()]);
	struct os_error_info_s *error_info = &(core->last_error_info);

	error_info->app_id = (ApplicationIDType) 0xFF;
	error_info->context_id = (os_context_id_t) 0xFFFFFFFFU;
	error_info->call_id.task_id = (TaskType) 0xFFFFU;
	error_info->status = E_OK;
	error_info->detailed_status = OS_STATUS_OK;
	error_info->service_id = OSSERVICEID_COUNT;
	for (idx = 0U; idx < OS_ERROR_PARAM_INDEX_COUNT; idx++) {
		error_info->parameter[idx] = 0UL;
	}
}

FUNC(StatusType, OS_CODE) os_get_error_info(struct os_error_info_s *error_info)
{
	os_core_inst_t *core = OS_CORE_GET_CORE_INSTANCE_REF(os_core_cfg_ref_table[os_get_core_id()]);

	DEBUGASSERT(error_info != NULL_PTR);

	(void)memcpy(error_info, &core->last_error_info, sizeof(struct os_error_info_s));
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_get_error_service_id(OSServiceIdType *service_id)
{
	os_core_inst_t *core = OS_CORE_GET_CORE_INSTANCE_REF(os_core_cfg_ref_table[os_get_core_id()]);

	*service_id = core->last_error_info.service_id;
	return OS_STATUS_OK;
}

FUNC(void, OS_CODE) vservice_error_report(OSServiceIdType service_id, os_status_t error_status,
										  uint32 param_cnt, va_list ap)
{
	uint32 idx;
	const os_core_cfg_t *core_cfg = os_core_cfg_ref_table[os_get_core_id()];
	struct os_error_info_s *error_info = &(OS_CORE_GET_CORE_INSTANCE_REF(core_cfg)->last_error_info);

	DEBUGASSERT(param_cnt <= OS_ERROR_PARAM_INDEX_COUNT);

	error_info->service_id = service_id;
	set_last_error_info(error_info, error_status);

	for(idx = 0U; idx < param_cnt; idx++) {
		error_info->parameter[idx] = va_arg(ap, uintptr_t);
	}

#if defined(CONFIG_RT_FRAMEWORK_ERROR_HOOK) && (CONFIG_RT_FRAMEWORK_ERROR_HOOK == 1)
	os_call_error_hook(core_cfg, (StatusType) (error_status));
#endif
}

FUNC(void, OS_CODE) os_protection_error_report(StatusType error_status)
{
	const os_core_cfg_t *core_cfg = os_core_cfg_ref_table[os_get_core_id()];
	struct os_error_info_s *error_info = &(OS_CORE_GET_CORE_INSTANCE_REF(core_cfg)->last_error_info);

	set_last_error_info(error_info, (os_status_t)error_status);

#if defined(CONFIG_RT_FRAMEWORK_PROTECTION_HOOK) && (CONFIG_RT_FRAMEWORK_PROTECTION_HOOK == 1)
	protection_return_t repsonse = os_call_protection_hook(error_status);

	handle_protection_response(repsonse, error_status);
#endif
}

FUNC(void, OS_CODE) os_fatal_error_report(const char *func_name, uint32 line)
{
	UNUSED(func_name);
	UNUSED(line);

	os_int_disable_global();

#if defined(CONFIG_RT_FRAMEWORK_PANIC_HOOK) && (CONFIG_RT_FRAMEWORK_PANIC_HOOK == 1)
	os_panichook();
#endif
	for (;;) {
		/* loop forever */
	}
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
