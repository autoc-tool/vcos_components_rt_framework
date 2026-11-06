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
#ifndef RT_FRAMEWORK_INC_THREAD_TYPES_H
#define RT_FRAMEWORK_INC_THREAD_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <nuttx/config.h>
#include <nuttx/irq.h>
#include "rtfw_types.h"

#define STACKSIZE_ROUND_UP(size, data_type) (((size) + sizeof(data_type) - 1) & (~(sizeof(data_type) - 1)))

#define DeclareThreadStack(section, thread_id, byte_size)                                                              \
	RTFW_TABLE_DECLARE(section, os_stack, StackType_t, thread_id, STACKSIZE_ROUND_UP(byte_size, StackType_t))

#define THREADSTACKVAR(task_id) (&(RTFW_LINK_ENTRY_NAME(os_stack, task_id)[0]))
#define THREADSTACKSIZEVAR(byte_size) STACKSIZE_ROUND_UP(byte_size, StackType_t)

struct os_task_thread_inst_s;
typedef struct os_task_thread_inst_s os_task_thread_inst_t;

struct os_hook_thread_inst_s;
typedef struct os_hook_thread_inst_s os_hook_thread_inst_t;

#ifdef __cplusplus
}
#endif

#endif /*  RT_FRAMEWORK_INC_THREAD_TYPES_H */