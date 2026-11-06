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

#ifndef RT_FRAMEWORK_INC_OS_IPICALL_H
#define RT_FRAMEWORK_INC_OS_IPICALL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include "rtfw_ipicall_internal.h"

/* Global Macro Definition */
#define DeclareIpicall(section_const, section_var, ipicall_id)                                   \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_ipicall_inst_t, ipicall_id);                   \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_ipicall, const os_ipicall_cfg_t, ipicall_id)

#define OSIPICALLCFGVAR(ipicall_id) RTFW_LINK_ENTRY_NAME(os_ipicall, ipicall_id)
#define IPICALLOBJVAR(ipicall_id) RTFW_LINK_ENTRY_NAME(os_obj, ipicall_id)

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
FUNC(os_status_t, OS_CODE) os_ipicall_send_async(uint32 target_core_id, os_ipicall_data_t *data);
FUNC(os_status_t, OS_CODE) os_ipicall_send_sync(uint32 target_core_id, os_ipicall_data_t *data);
FUNC(os_status_t, OS_CODE) os_ipicall_broad_cast(uint32 target_core_mask, os_ipicall_data_t *data);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_IPICALL_H */
