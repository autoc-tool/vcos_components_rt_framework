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

#ifndef RT_FRAMEWORK_INC_OS_RESOURCE_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_RESOURCE_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_resource_types.h"
#include "rtfw_lock_internal.h"
#include "rtfw_compiler_cfg.h"
/* Global Macro Definition */
#define OS_GET_RESOURCE_INST_BY_LOCK_CFG_ADDR(_cfgAddr) \
(((os_resource_inst_t *)((uint8 *)(_cfgAddr) - RTFW_OFFSETOF(os_resource_inst_t, lock))))

/* Structure to define a resource instance */
struct os_resource_inst_s {
	ResourceType resource_id;   /* Resource identifier */
	os_lock_cfg_t lock;         /* lock information for LIFO support */
	uint32 previous_priority;   /* Task priority before occupying this resource */
	uint32 is_occupied;         /* Whether the resource is occupied */
};

/* Structure to define a resource */
struct os_resource_cfg_s {
	os_resource_inst_t *obj_desc;   /* Resource instance description */
	ResourceType resource_id;       /* Resource identifier */
	uint32 ceiling_priority;        /* Resource ceiling priority */
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
	uint32 access_app_mask;         /* Application access rights information */
#endif
};

/* Global Variable Definition */
DeclareRtfwRefTable_H(resource)

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

/**
 * @brief        Initialize system resources
 * @details      Resource validation check and variable initialization
 *
 * @param[in]    os_resource_cfg_table   First address of resource configuration table
 * @param[in]    size                    Number of resources
 * @param[out]   none
 * @return       none
 *
 * @context      none
 *
 * @reentrant    TRUE
 *
 * @synchronous  TRUE
 */
FUNC(void, OS_CODE) os_resource_init(const os_resource_cfg_t *const os_resource_cfg_table[], OsObjIndexType size);

/**
 * @brief        GetResource() Implementation
 * @details      see GetResource()
 *
 * @param[in]    resource_id      Reference to resource
 * @param[out]   None
 *
 * @return       Status of execution result
 *               E_OK:           [STANDARD:] No error
 *               E_OS_ID:        [EXTENDED:] Resource <ResID> is invalid
 *               E_OS_ACCESS:    [EXTENDED:] Attempt to get a resource which is already occupied by any task or ISR,
 *                                           or the statically assigned priority of the calling task or interrupt
 *                                           routine is higher than the calculated ceiling priority
 *
 * @context      TASK
 *
 * @reentrant    TRUE
 *
 * @synchronous  TRUE
 */
FUNC(os_status_t, OS_CODE) os_get_resource(ResourceType resource_id);

/**
 * @brief        ReleaseResource() Implementation
 * @details      see ReleaseResource()
 *
 * @param[in]    resource_id      Reference to resource
 * @param[out]   None
 *
 * @return       Status of execution result
 *               E_OK:           [STANDARD:] No error
 *               E_OS_ID:        [EXTENDED:] Resource <ResID> is invalid
 *               E_OS_NOFUNC:    [EXTENDED:] Attempt to release a resource which is not occupied by any task or ISR,
 *                                           or another resource shall be released before
 *               E_OS_ACCESS:    [EXTENDED:] Attempt to release a resource which has a lower ceiling priority than
 *                                           the statically assigned priority of the calling task or interrupt routine
 *
 * @context      TASK
 *
 * @reentrant    TRUE
 *
 * @synchronous  TRUE
 */
FUNC(os_status_t, OS_CODE) os_release_resource(ResourceType resource_id);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_RESOURCE_INTERNAL_H */
