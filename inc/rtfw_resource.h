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

#ifndef RT_FRAMEWORK_INC_OS_RESOURCE_H
#define RT_FRAMEWORK_INC_OS_RESOURCE_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_resource_internal.h"

/* Global Macro Definition */
#define DeclareResource(section_const, section_var, resource_id)                        \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_resource_inst_t, resource_id);    \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_resource, const os_resource_cfg_t, resource_id)

#define RESOURCECFGVAR(resource_id) RTFW_LINK_ENTRY_NAME(os_resource, resource_id)
#define RESOURCEOBJVAR(resource_id) RTFW_LINK_ENTRY_NAME(os_obj, resource_id)

#ifdef __cplusplus
}
#endif

#endif /*! RT_FRAMEWORK_INC_OS_RESOURCE_H */
