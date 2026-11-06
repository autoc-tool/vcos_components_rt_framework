
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
#include <nuttx/irq.h>

#include "rtfw_isolate_internal.h"
#include "rtfw_context_internal.h"
#include "rtfw_core_internal.h"
#include "application_cfg.h"
#include "isolate_ppc_cfg.h"

static inline_function void isolate_switch_privilege(uint32_t *regs, boolean origin_mode, boolean target_mode)
{
	if (origin_mode != target_mode) {
		up_syscall_set_prev_privilege(regs, target_mode);
	}
}

static inline_function void isolate_switch_mpu_set(uint32_t *regs, const uint8 current_set, const uint8 target_set)
{
	if (current_set != target_set) {
		up_set_prev_mpu_set_from_syscall(regs, target_set);
	}
}

static inline_function void isolate_ppc_enter_handler(struct os_isolate_ppc_context_s *ppc_ctx, uint32_t func_id, uint32_t *regs)
{
	const os_ctx_cfg_t *current_context = os_context_get_contexthdl();
	os_ctx_inst_t *context_instance = OS_CONTEXT_GET_CONTEXTINST_REF(current_context);

	/* read ppc cfg table */
	const uint32 app_id = ppc_func_config[func_id][os_get_core_id()];
	const os_app_cfg_t *target_app_cfg = os_app_cfg_ref_table[app_id];

	/* saving os cfg */
	context_instance->isolate_critical_cnt++;
	ppc_ctx->app_cfg = context_instance->current_app_cfg;
	context_instance->current_app_cfg = target_app_cfg;

	/* privilege switching */
	ppc_ctx->is_privilege = up_syscall_get_prev_privilege(regs);
	ppc_ctx->target_is_privilege = target_app_cfg->is_privileged;
	isolate_switch_privilege(regs, ppc_ctx->is_privilege, ppc_ctx->target_is_privilege);

	/* mpu switching */
	ppc_ctx->access_right = context_instance->current_mpu_access_right;
	ppc_ctx->target_access_right = target_app_cfg->mpu_access_right;
	context_instance->current_mpu_access_right = target_app_cfg->mpu_access_right;
	up_mpu_switch_access_right(ppc_ctx->access_right, ppc_ctx->target_access_right);

	ppc_ctx->set_num = up_get_prev_mpu_set_from_syscall(regs);
	ppc_ctx->target_set_num = target_app_cfg->mpu_access_right->set_num;
	isolate_switch_mpu_set(regs, ppc_ctx->set_num, ppc_ctx->target_set_num);
}

static inline_function void isolate_ppc_exit_handler(struct os_isolate_ppc_context_s *ppc_ctx, uint32_t *regs)
{
	const os_ctx_cfg_t *current_context = os_context_get_contexthdl();
	os_ctx_inst_t *context_instance = OS_CONTEXT_GET_CONTEXTINST_REF(current_context);

	/* restore os cfg */
	context_instance->isolate_critical_cnt--;
	context_instance->current_app_cfg = ppc_ctx->app_cfg;
	context_instance->current_mpu_access_right = ppc_ctx->access_right;

	/* privilege restore */
	isolate_switch_privilege(regs, ppc_ctx->target_is_privilege, ppc_ctx->is_privilege);

	/* mpu switching */
	up_mpu_switch_access_right(ppc_ctx->target_access_right, ppc_ctx->access_right);
	isolate_switch_mpu_set(regs, ppc_ctx->target_set_num, ppc_ctx->set_num);
}

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(boolean, OS_CODE) isolate_ppc_is_needed(const uint32_t func_id)
{
	/* TODO: Check whether the target OSA is identical to the current one.
	 * If they are the same, skip the PPC call to avoid unnecessary performance overhead.
	 */
	(void)func_id;
	return true;
}

FUNC(void, OS_CODE) rtfw_ppc_handler(const uint32_t ppc_flag, const uint32_t func_id,
									 void *ppc_ctx_args, uint32_t *regs)
{
	struct os_isolate_ppc_context_s *ppc_ctx = (struct os_isolate_ppc_context_s *)ppc_ctx_args;

	if (ISOLATE_PPC_FLAG_CALL_ENTER == ppc_flag) {
		isolate_ppc_enter_handler(ppc_ctx, func_id, regs);
	} else if (ISOLATE_PPC_FLAG_CALL_EXIT == ppc_flag) {
		isolate_ppc_exit_handler(ppc_ctx, regs);
	}
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
