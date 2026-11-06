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

#ifndef RT_FRAMEWORK_INC_OS_LOCK_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_LOCK_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_lock_types.h"
#include "rtfw_list.h"
/* Global Macro Definition */
#define OS_LOCK_CFG_PTR_GET(_nodeAddr) ((os_lock_cfg_t *)((uint8 *)(_nodeAddr) - offsetof(os_lock_cfg_t, lock_node)))
/* Global Enumeration Definition */
typedef enum {
	OS_LOCK_TYPE_RESOURCE, /*!< Resources. */
	OS_LOCK_TYPE_SPINLOCK /*!< Spinlock. */
} os_lock_t;

/* Global Type & Structure Definition */
struct os_lock_cfg_s {
	/* coverity[misra_c_2012_rule_5_9_violation:SUPPRESS] */
	os_lock_t type; /*! lock type */
	os_list_node_t lock_node; /*! lock node */
};

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_LOCK_INTERNAL_H */
