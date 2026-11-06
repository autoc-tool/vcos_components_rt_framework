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

#ifndef RT_FRAMEWORK_INC_RTFW_TRAP_INTERNAL_H
#define RT_FRAMEWORK_INC_RTFW_TRAP_INTERNAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <nuttx/config.h>
#include <arch/trap.h>
#include "rtfw_trap_types.h"

#define MAX_BACKTRACE_NUM        10U

struct os_expt_detail_s{
	uint32_t core_id;
	uint32_t application_id;
	os_expt_thread_t thread_type;
	uint32_t thread_id;
	uint32_t backtrace_depth;
	uint32_t backtrace_addr[MAX_BACKTRACE_NUM];
};

struct os_expt_full_cotext_s{
	up_expt_frame_t expt_frame;
	os_expt_detail_t expt_detail;
};

#ifdef __cplusplus
}
#endif

#endif  /* RT_FRAMEWORK_INC_RTFW_TRAP_INTERNAL_H */
