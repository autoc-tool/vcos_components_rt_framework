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

#ifndef RT_FRAMEWORK_INC_OS_TASK_TYPES_H
#define RT_FRAMEWORK_INC_OS_TASK_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
struct os_task_event_s;
typedef struct os_task_event_s os_task_event_t;

struct os_task_inst_s;
typedef struct os_task_inst_s os_task_inst_t;

struct os_task_cfg_s;
typedef struct os_task_cfg_s os_task_cfg_t;

#if defined(CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK) && (CONFIG_RT_FRAMEWORK_SHARE_TASK_STACK == 1)
struct task_temp_buffer_s;
typedef struct task_temp_buffer_s task_temp_buffer_t;

struct task_share_stack_s;
typedef struct task_share_stack_s task_share_stack_info_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_TASK_TYPES_H */
