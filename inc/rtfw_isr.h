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

#ifndef RT_FRAMEWORK_INC_OS_ISR_H
#define RT_FRAMEWORK_INC_OS_ISR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_isr_internal.h"
#include "rtfw_context_internal.h"

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
#include "rtfw_application.h"
#endif

#include "rtfw_task.h"

/* Global Macro Definition */

/*! 声明ISR */
#define DeclareISR(section_const, section_var, core, isr)                                 \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_isr_inst_t, core##isr);                 \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_isr, const os_isr_cfg_t, core##isr)
#define DeclareCounterISR(section_const, section_var, core, isr)                          \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_isr_inst_t, core##isr);                 \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_isr, const os_counter_isr_cfg_t, core##isr)

#define ISRCFGVAR(core, isr) RTFW_LINK_ENTRY_NAME(os_isr, core##isr)
#define ISROBJVAR(core, isr) RTFW_LINK_ENTRY_NAME(os_obj, core##isr)

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_ISR_H */
