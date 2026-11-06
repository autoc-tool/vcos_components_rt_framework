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

#ifndef RT_FRAMEWORK_INC_RTFW_TRAP_TYPES_H
#define RT_FRAMEWORK_INC_RTFW_TRAP_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	os_expt_thread_type_task,
	os_expt_thread_type_isr,
	os_expt_thread_type_hook,
} os_expt_thread_t;

typedef struct os_expt_detail_s os_expt_detail_t;
typedef struct os_expt_full_cotext_s os_expt_full_context_t;

#ifdef __cplusplus
}
#endif

#endif  /* RT_FRAMEWORK_INC_RTFW_TRAP_TYPES_H */
