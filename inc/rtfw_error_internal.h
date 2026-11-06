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

#ifndef RT_FRAMEWORK_INC_OS_ERROR_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_ERROR_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <nuttx/config.h>
#include "Compiler.h"
#include "rtfw_types.h"

typedef enum {
	OS_ERROR_PARAM_INDEX_1 = 0,
	OS_ERROR_PARAM_INDEX_2,
	OS_ERROR_PARAM_INDEX_3,

	OS_ERROR_PARAM_INDEX_COUNT
} os_error_param_index_t;

typedef union {
	TaskType task_id;
	isr_id_t isr_id;
} os_error_call_id_t;

struct os_error_info_s {
	OSServiceIdType service_id;
	StatusType status;
	os_status_t detailed_status;
	ApplicationIDType app_id;
	os_context_id_t context_id;
	os_error_call_id_t call_id;
	uintptr_t parameter[OS_ERROR_PARAM_INDEX_COUNT];
};

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
FUNC(void, OS_CODE) os_fatal_error_report(const char *func_name, uint32 line);
FUNC(void, OS_CODE) vservice_error_report(OSServiceIdType service_id, os_status_t error_status,
										  uint32 param_cnt, va_list ap);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#if defined(CONFIG_RT_FRAMEWORK_PANIC_HOOK) && (CONFIG_RT_FRAMEWORK_PANIC_HOOK == 1)
#define OS_START_SEC_CALLOUT_CODE
#include "rtfw_memmap.h"
extern FUNC(void, OS_CALLOUT_CODE) os_panichook(void);

#define OS_STOP_SEC_CALLOUT_CODE
#include "rtfw_memmap.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_ERROR_INTERNAL_H */
