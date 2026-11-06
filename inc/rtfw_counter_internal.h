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

#ifndef OS_COUNTER_INTERNAL_H
#define OS_COUNTER_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include <nuttx/hrtimer.h>
#include "rtfw_types.h"
#include "rtfw_counter_types.h"

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "rtfw_schedtable_types.h"
#endif

/* Global Enumeration Definition */
typedef enum {
	TIMERTYPE_SOFTWARE = 0,
	TIMERTYPE_HIGH_RESOLUTION,
} os_timer_t;

/* Global Type & Structure Definition */
typedef uint16 action_queue_size_t;
typedef hrtimer_callback_t callback_func_t;

struct os_action_cfg_s {
	struct hrtimer_s *hrtimer;
	os_counter_cfg_t *counter;
	callback_func_t action_callback;
};

struct os_action_queue_s {
	void *action_cfg;
};

struct os_counter_inst_s {
	struct hrtimer_queue_s hrtimer_queue; /* must come first !! */
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	os_schedtable_inst_t *schedtable_list_first;
#endif
	TickType counter_value;
	hrtimer_tick_t hrtimer_value;
	hrtimer_tick_t hrtimer_sw_compare;
	hrtimer_tick_t hrtimer_sw_value;
};

struct os_counter_cfg_s {
	os_counter_inst_t *obj_desc;
	os_action_queue_t *action_queue;
	action_queue_size_t queue_total_size;
	CounterType counter_id;
	const char *counter_name;
	TickType max_allowed_value;
	TickType tick_per_base;
	TickType min_cycle;
	TickType ticks_per_ms;
	os_timer_t timer_type;
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
	ApplicationIDType owner_app_id;
	uint32 access_app_mask;
#endif
};

/* Global Macro Definition */
#define MAX_ACTION_EXECS_CNT 4u

#ifdef __cplusplus
}
#endif

#endif /* OS_COUNTER_INTERNAL_H */
