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

#ifndef RT_FRAMEWORK_INC_OS_APPLICATION_H
#define RT_FRAMEWORK_INC_OS_APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include "rtfw_application_internal.h"

/*! Application declare*/
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)

#define DeclareOsApplication(section_const, section_var, app_id)                    \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_app_inst_t, app_id);              \
	RTFW_LINK_ENTRY_DECLARE(section_const, OsApplication, const os_app_cfg_t, app_id)

#define OSAPPCFGVAR(app_id) RTFW_LINK_ENTRY_NAME(OsApplication, app_id)
#define OSAPPOBJVAR(app_id) RTFW_LINK_ENTRY_NAME(os_obj, app_id)

/* Global Variable Definition */

#endif

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_APPLICATION_H */
