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

#ifndef RT_FRAMEWORK_INC_OS_ERROR_H
#define RT_FRAMEWORK_INC_OS_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_RT_FRAMEWORK_ERROR) && (CONFIG_RT_FRAMEWORK_ERROR == 1)

#include "rtfw_error_internal.h"

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
FUNC(void, OS_CODE) os_error_init(void);

FUNC(StatusType, OS_CODE) os_get_error_info(struct os_error_info_s *error_info);
FUNC(os_status_t, OS_CODE) os_get_error_service_id(OSServiceIdType *service_id);

FUNC(void, OS_CODE) service_error_report(OSServiceIdType service_id, os_status_t error_status, uint32 param_cnt, ...);
FUNC(void, OS_CODE) os_protection_error_report(StatusType error_status);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#define REPORT_FATAL_ERROR()    os_fatal_error_report(__func__ , __LINE__)
#define REPORT_PROTECTION_ERROR(status)    os_protection_error_report(status)
#define REPORT_SERVICE_ERROR(service_id, status, param_cnt, ...) \
	do { \
		if (UNLIKELY((status) != OS_STATUS_OK)) { \
			service_error_report((OSServiceIdType)(service_id), (status), (param_cnt), ##__VA_ARGS__); \
		} \
	} while(0)

#else

static inline_function void
service_error_report(OSServiceIdType service_id, os_status_t error_status, uint32 param_cnt, ...)
{
	UNUSED(service_id);
	UNUSED(error_status);
	UNUSED(param_cnt);
}

#define REPORT_FATAL_ERROR()
#define REPORT_PROTECTION_ERROR(status)    UNUSED(status)
#define REPORT_SERVICE_ERROR(service_id, status, param_cnt, ...) \
		service_error_report(service_id, status, param_cnt, ##__VA_ARGS__)

#endif

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_ERROR_H */
