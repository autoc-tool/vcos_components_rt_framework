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
#include "rtfw_types.h"
#include "rtfw_task.h"
#include "rtfw_isr.h"
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
#include "rtfw_ipicall.h"
#endif
#include "rtfw_counter.h"
#include "rtfw_application.h"
#include "syslog.h"
#include "rtfw_core.h"
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "rtfw_schedtable.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
#include "rtfw_spinlock.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
#include "rtfw_resource.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
#include "rtfw_hook.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_MEMPOOL) && (CONFIG_RT_FRAMEWORK_MEMPOOL == 1)
#include "rtfw_mempool.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_ERROR) && (CONFIG_RT_FRAMEWORK_ERROR == 1)
#include "rtfw_error.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
#include "rtfw_ioc.h"
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) StartOS(void)
{
	os_context_set_contexthdl(NULL);
	CoreIdType core_id = os_get_core_id();
	const os_core_cfg_t *core = os_core_cfg_ref_table[core_id];
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	if (0U != up_mpu_core_init(os_core_access_right_ref_table[core_id])) {
		//todo
	}
#endif
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
	if ((core->app_refs != NULL_PTR) && (core->app_count != NULL_PTR)) {
		os_app_init(core->app_refs, *(core->app_count));
	}
#else
	os_task_init(os_task_cfg_ref_table, os_task_cfg_ref_table_size);
	rtfw_isr_init(os_isr_cfg_ref_table, os_isr_cfg_ref_table_size);
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
	os_counter_init(os_counter_cfg_ref_table, os_counter_cfg_ref_table_size);
	os_counter_start(os_counter_cfg_ref_table, os_counter_cfg_ref_table_size);
#endif
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	os_schedule_table_init(os_schedtable_cfg_ref_table, os_schedtable_cfg_ref_table_size);
#endif
#endif /* CONFIG_RT_FRAMEWORK_MULTIAPP */

#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
	os_ipicall_init(os_ipicall_cfg_ref_table, os_ipicall_cfg_ref_table_size);
#endif

#if defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
	if (os_get_core_id() == 0U) {
		os_spinlock_init();
	}
#endif /* CONFIG_RT_FRAMEWORK_SPINLOCK */

#if defined(CONFIG_RT_FRAMEWORK_IOC) && (CONFIG_RT_FRAMEWORK_IOC == 1)
	if ((core->ioc_refs != NULL_PTR) && (core->ioc_count != NULL_PTR)) {
		os_ioc_init(core->ioc_refs, *(core->ioc_count));
	}
#endif /* CONFIG_RT_FRAMEWORK_IOC */

#if defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)
	if ((core->resource_refs != NULL_PTR) && (core->resource_count != NULL_PTR)) {
		os_resource_init(core->resource_refs, *(core->resource_count));
	}
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE */

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
	if ((core->hook_refs != NULL_PTR) && (core->hook_count != NULL_PTR)) {
		os_hook_init(core->hook_refs, *(core->hook_count));
	}
#endif

#if defined(CONFIG_RT_FRAMEWORK_MEMPOOL) && (CONFIG_RT_FRAMEWORK_MEMPOOL == 1)
	os_global_mempools_init();
#endif

#if defined(CONFIG_RT_FRAMEWORK_ERROR) && (CONFIG_RT_FRAMEWORK_ERROR == 1)
	os_error_init();
#endif

	os_core_sync_phase_one();
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
