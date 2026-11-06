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
#include "nuttx/config.h"
#include "rtfw_atomic.h"
#include "rtfw_core.h"
#include "rtfw_error.h"
#include "core_cfg.h"

#if (CONFIG_RT_FRAMEWORK_NCORES > 1)
#define OS_CORE_SYNC_MASK ((0x1U << (CONFIG_RT_FRAMEWORK_NCORES)) - 1U)
#define OS_CORE_SYNC_FLAG ((0x1U << (CONFIG_RT_FRAMEWORK_NCORES)) - 1U)

/* TODO: if cache is enabled, these two variables should be mapped to none cache area*/
Atomic g_coresync_flag_one;
Atomic g_coresync_flag_two;
Atomic g_coreshutdown_sync;
#endif

#if (CONFIG_RT_FRAMEWORK_NCORES > 1)
static inline void os_core_sync_all_cores(Atomic *const sync_value_ref)
{
	sint32 core_id = (sint32)os_get_core_id();
	(void)Os_AtomicOr(sync_value_ref, (sint32)(1U << core_id));

	while (((*sync_value_ref) & OS_CORE_SYNC_MASK) != OS_CORE_SYNC_FLAG) {
	}
}
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

#if (CONFIG_RT_FRAMEWORK_NCORES > 1)
FUNC(void, OS_CODE) os_core_sync_phase_one(void)
{
	os_core_sync_all_cores(&g_coresync_flag_one);
}

FUNC(void, OS_CODE) os_core_sync_phase_two(void)
{
	os_core_sync_all_cores(&g_coresync_flag_two);
}

FUNC(void, OS_CODE) os_core_shutdown_sync(void)
{
	os_core_sync_all_cores(&g_coreshutdown_sync);
}
#endif

FUNC(void, OS_CODE) os_shutdown_core(StatusType error_status)
{
	os_int_disable_global();

	//TODO: kill app

#if defined(CONFIG_RT_FRAMEWORK_SHUTDOWN_HOOK) && (CONFIG_RT_FRAMEWORK_SHUTDOWN_HOOK == 1)
	os_call_shutdown_hook(error_status);
#else
	(void)error_status;
#endif

	while (TRUE) {
	}
}

FUNC(void, OS_CODE) os_shutdown_os(StatusType error_status)
{
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();

	if (UNLIKELY(FALSE == os_context_check_curcontext_id(current_context, OS_CONTEXT_SHUTDOWNOS))) {
		REPORT_PROTECTION_ERROR(E_OS_CALLEVEL);
	} else {
		os_shutdown_core(error_status);
	}
}

FUNC(void, OS_CODE) os_shutdown_all_cores(StatusType error_status)
{
#if (CONFIG_RT_FRAMEWORK_NCORES > 1)
	CoreIdType core_id = os_get_core_id();

	uint32 core_mask = ((0x00000001U<<(uint32)OS_CORE_CNT) - 1U) & (~(0x00000001U<<(uint32)core_id));

	os_ipicall_shutdown_all_cores(core_mask, error_status);
#endif

	os_shutdown_core(error_status);
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
