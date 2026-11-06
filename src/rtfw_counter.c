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

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_counter.h"
#include "rtfw_core.h"
#include "rtfw_isr.h"
#include "rtfw_application.h"
#include "counter_cfg.h"

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

static FUNC(hrtimer_tick_t, OS_CODE) os_counter_hrtimer_current(struct hrtimer_queue_s *queue)
{
	os_counter_inst_t *counter_inst = (os_counter_inst_t *)queue;
	return counter_inst->hrtimer_sw_value;
}
static FUNC(void, OS_CODE) sw_counter_hrtimer_set_compare(struct hrtimer_queue_s *queue, hrtimer_tick_t value)
{
	os_counter_inst_t *counter_inst = (os_counter_inst_t *)queue;
	counter_inst->hrtimer_sw_compare = value;
}

static FUNC(void, OS_CODE) sw_counter_hrtimer_start(struct hrtimer_queue_s *queue)
{
	(void)queue;
}

static FUNC(void, OS_CODE) sw_counter_hrtimer_trigger(struct hrtimer_queue_s *queue)
{
	(void)queue;
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#define OS_START_SEC_CONST
#include "rtfw_memmap.h"
static CONST(struct hrtimer_ops_s, OS_CONST) sw_counter_hrtimer_ops = {
	.current = os_counter_hrtimer_current,
	.set_expire = sw_counter_hrtimer_set_compare,
	.start = sw_counter_hrtimer_start,
	.trigger = sw_counter_hrtimer_trigger,
};
#define OS_STOP_SEC_CONST
#include "rtfw_memmap.h"

static inline void os_counter_check_on_core(const os_app_cfg_t *const os_app_cfg_table[], const OsObjIndexType app_num)
{
	CounterType counter_id;
	OsObjIndexType app_index, counter_index;
	const os_counter_cfg_t *counter_cfg;
	os_counter_inst_t *counter_inst;
	const os_app_cfg_t *app_cfg;

	if (UNLIKELY((NULL == os_app_cfg_table) || ((OsObjIndexType)0 == app_num))) {
		return;
	}

	for (app_index = 0U; app_index < app_num; app_index++) {
		app_cfg = os_app_cfg_table[app_index];
		if ((NULL != app_cfg->counter_count) && (NULL != app_cfg->counter_refs)) {
			for (counter_index = 0U; counter_index < *(app_cfg->counter_count); counter_index++) {
				counter_id = app_cfg->counter_refs[counter_index]->counter_id;
				counter_cfg = os_counter_cfg_ref_table[counter_id];
				counter_inst = counter_cfg->obj_desc;
				if (counter_cfg->timer_type == TIMERTYPE_SOFTWARE) {
					counter_inst->hrtimer_sw_value++;
					hrtimer_process(&(counter_inst->hrtimer_queue));
				}
			}
		}
	}
}

static inline boolean os_counter_is_valid(CounterType counter_id)
{
	boolean return_value = TRUE;

	if (UNLIKELY(counter_id >= (CounterType)os_counter_cfg_ref_table_size)) {
		return_value = FALSE;
	} else {
		/* Do nothing.*/
	}

	return return_value;
}

static inline void os_counter_set_compare_value(const os_counter_cfg_t *counter_cfg,
						hrtimer_tick_t expiration_timestamp)
{
	os_counter_inst_t *counter_inst = counter_cfg->obj_desc;
	struct hrtimer_queue_s *hrtimer_queue = &(counter_inst->hrtimer_queue);
	hrtimer_set_expire(hrtimer_queue, expiration_timestamp);
}

static inline TickType os_counter_get_internal_value(const os_counter_cfg_t *counter_cfg)
{
	os_counter_inst_t *counter_inst = counter_cfg->obj_desc;
	struct hrtimer_queue_s *hrtimer_queue = &(counter_inst->hrtimer_queue);
	hrtimer_tick_t hrtime = hrtimer_get_time(hrtimer_queue);
	hrtimer_tick_t hrtime_passed = hrtimer_passed(hrtime, counter_inst->hrtimer_value);
	clock_t period = (clock_t)counter_cfg->max_allowed_value + 1;
	clock_t reduced = hrtime_passed % period;

	counter_inst->hrtimer_value = hrtime;
	counter_inst->counter_value =
		os_counter_add(counter_inst->counter_value, (TickType)reduced, counter_cfg->max_allowed_value);

	return counter_inst->counter_value;
}

/* global Function Implementation */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) os_counter_check(void)
{
	CoreIdType core_id = os_get_core_id();

	os_counter_check_on_core(os_core_cfg_ref_table[core_id]->app_refs,
				 *(os_core_cfg_ref_table[core_id]->app_count));
}

FUNC(TickType, OS_CODE) os_counter_get_user_value(const os_counter_cfg_t *counter_cfg)
{
	return os_counter_get_internal_value(counter_cfg);
}

FUNC(void, OS_CODE) os_counter_init(const os_counter_cfg_t *const os_counter_cfg_table[], OsObjIndexType size)
{
	const os_counter_cfg_t *counter_cfg = NULL;
	os_counter_inst_t *counter_inst = NULL;
	const os_app_cfg_t *app_cfg = NULL;
	const struct hrtimer_ops_s *hrtimer_ops = NULL;
	CoreIdType core_id = os_get_core_id();

	for (OsObjIndexType i = 0U; i < size; i++) {
		counter_cfg = os_counter_cfg_table[i];
		app_cfg = os_app_cfg_ref_table[counter_cfg->owner_app_id];
		if (app_cfg->core_id != core_id) {
			continue;
		}
		counter_inst = counter_cfg->obj_desc;
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
		counter_inst->schedtable_list_first = NULL;
#endif
		counter_inst->counter_value = 0;
		counter_inst->hrtimer_value = 0;
		counter_inst->hrtimer_sw_compare = 0;
		counter_inst->hrtimer_sw_value = 0;

		if (counter_cfg->timer_type == TIMERTYPE_SOFTWARE) {
			hrtimer_ops = &sw_counter_hrtimer_ops;
		} else {
			hrtimer_ops = NULL;
		}
		hrtimer_init(&(counter_inst->hrtimer_queue), (struct hrtimer_s **)counter_cfg->action_queue,
			     (int)counter_cfg->queue_total_size, hrtimer_ops);
	}
}

FUNC(void, OS_CODE) os_counter_start(const os_counter_cfg_t *const os_counter_cfg_table[], OsObjIndexType size)
{
	const os_counter_cfg_t *counter_cfg = NULL;
	os_counter_inst_t *counter_inst = NULL;
	const os_app_cfg_t *app_cfg = NULL;
	struct hrtimer_queue_s *hrtimer_queue;
	CoreIdType core_id = os_get_core_id();

	for (OsObjIndexType i = 0U; i < size; i++) {
		counter_cfg = os_counter_cfg_table[i];
		app_cfg = os_app_cfg_ref_table[counter_cfg->owner_app_id];
		if (app_cfg->core_id != core_id) {
			continue;
		}
		counter_inst = counter_cfg->obj_desc;
		hrtimer_queue = &(counter_inst->hrtimer_queue);
		hrtimer_start(hrtimer_queue);

		counter_inst->hrtimer_value = hrtimer_get_time(hrtimer_queue);
		os_counter_set_compare_value(counter_cfg, counter_inst->hrtimer_value);
	}
}

FUNC(void, OS_CODE) os_counter_add_rel_action(const os_action_cfg_t *action_cfg, TickType increment)
{
	const os_counter_cfg_t *counter_cfg = action_cfg->counter;
	os_counter_inst_t *counter_inst = counter_cfg->obj_desc;
	struct hrtimer_queue_s *hrtimer_queue = &(counter_inst->hrtimer_queue);
	struct hrtimer_s *hrtimer = action_cfg->hrtimer;

	hrtimer->queue = hrtimer_queue;
	hrtimer->arg = (void *)action_cfg;
	hrtimer->callback = action_cfg->action_callback;

	hrtimer_add_rel(hrtimer, increment);
}

FUNC(void, OS_CODE) os_counter_add_abs_action(const os_action_cfg_t *action_cfg, TickType start)
{
	const os_counter_cfg_t *counter_cfg = action_cfg->counter;
	os_counter_inst_t *counter_inst = counter_cfg->obj_desc;
	struct hrtimer_queue_s *hrtimer_queue = &(counter_inst->hrtimer_queue);
	struct hrtimer_s *hrtimer = action_cfg->hrtimer;

	TickType now = os_counter_get_internal_value(counter_cfg);
	TickType diff = os_counter_diff(start, now, counter_cfg->max_allowed_value);

	hrtimer_tick_t hrtime_start = counter_inst->hrtimer_value + diff;

	hrtimer->queue = hrtimer_queue;
	hrtimer->arg = (void *)action_cfg;
	hrtimer->callback = action_cfg->action_callback;

	hrtimer_add_abs(hrtimer, hrtime_start);
}

FUNC(void, OS_CODE) os_counter_reload_action(const os_action_cfg_t *action_cfg, TickType cycle)
{
	const os_counter_cfg_t *counter_cfg = action_cfg->counter;
	os_counter_inst_t *counter_inst = counter_cfg->obj_desc;
	struct hrtimer_queue_s *hrtimer_queue = &(counter_inst->hrtimer_queue);
	struct hrtimer_s *hrtimer = action_cfg->hrtimer;

	hrtimer->queue = hrtimer_queue;
	hrtimer->arg = (void *)action_cfg;
	hrtimer->callback = action_cfg->action_callback;

	hrtimer_reload(hrtimer, cycle);
}

FUNC(void, OS_CODE) os_counter_delete_action(const os_action_cfg_t *action_cfg)
{
	struct hrtimer_s *hrtimer = action_cfg->hrtimer;

	hrtimer_delete(hrtimer);
}

FUNC(os_status_t, OS_CODE) os_counter_get_value(CounterType counter_id, TickRefType value)
{
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();
	const os_counter_cfg_t *counter_cfg;
	const os_app_cfg_t *app_cfg;
	irqlevel_t int_state;

	if (UNLIKELY(FALSE == os_context_check_curcontext_id(current_context, OS_CONTEXT_GETCOUNTERVALUE))) {
		return OS_STATUS_CALLEVEL;
	}
	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(FALSE == os_counter_is_valid(counter_id))) {
		return OS_STATUS_ID_1;
	}
#endif
	counter_cfg = os_counter_cfg_ref_table[counter_id];
	if (UNLIKELY(NULL == value)) {
		return OS_STATUS_PARAM_POINTER_2;
	}
	app_cfg = os_app_cfg_ref_table[counter_cfg->owner_app_id];
	if (UNLIKELY(app_cfg->core_id != os_get_core_id())) {
		return OS_STATUS_CORE;
	}
	if (UNLIKELY(APPLICATION_ACCESSIBLE != app_cfg->instance->state)) {
		return OS_STATUS_NOT_ACCESSIBLE;
	}
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	if (UNLIKELY(OS_STATUS_OK != os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(current_context),
							 counter_cfg->access_app_mask))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif

	os_int_suspend_kernel(&int_state);
	*value = os_counter_get_user_value(counter_cfg);
	os_int_resume_kernel(&int_state);

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE)
os_counter_get_elapsed_value(CounterType counter_id, TickRefType value, TickRefType elapsed_value_ref)
{
	const os_ctx_cfg_t *const current_context = os_context_get_contexthdl();
	const os_counter_cfg_t *counter_cfg;
	TickType max_allowed_value, current_tick;
	const os_app_cfg_t *app_cfg;
	irqlevel_t int_state;

	if (UNLIKELY(FALSE == os_context_check_curcontext_id(current_context, OS_CONTEXT_GETELAPSEDVALUE))) {
		return OS_STATUS_CALLEVEL;
	}
	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(FALSE == os_counter_is_valid(counter_id))) {
		return OS_STATUS_ID_1;
	}
#endif
	counter_cfg = os_counter_cfg_ref_table[counter_id];
	if (UNLIKELY(NULL == value)) {
		return OS_STATUS_PARAM_POINTER_2;
	}
	if (UNLIKELY(NULL == elapsed_value_ref)) {
		return OS_STATUS_PARAM_POINTER_3;
	}
	max_allowed_value = counter_cfg->max_allowed_value;
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY((*value) > max_allowed_value)) {
		return OS_STATUS_VALUE_TOO_HIGH_2;
	}
#endif
	app_cfg = os_app_cfg_ref_table[counter_cfg->owner_app_id];
	if (UNLIKELY(app_cfg->core_id != os_get_core_id())) {
		return OS_STATUS_CORE;
	}
	if (UNLIKELY(APPLICATION_ACCESSIBLE != app_cfg->instance->state)) {
		return OS_STATUS_NOT_ACCESSIBLE;
	}
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	if (UNLIKELY(OS_STATUS_OK != os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(current_context),
							 counter_cfg->access_app_mask))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif

	os_int_suspend_kernel(&int_state);
	current_tick = os_counter_get_user_value(counter_cfg);
	os_int_resume_kernel(&int_state);
	*elapsed_value_ref = os_counter_diff(current_tick, *value, max_allowed_value);
	*value = current_tick;

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_counter_increment(CounterType counter_id)
{
	const os_ctx_cfg_t *current_context = os_context_get_contexthdl();
	const os_counter_cfg_t *counter_cfg;
	const os_app_cfg_t *app_cfg;
	irqlevel_t int_state;

	if (UNLIKELY(FALSE == os_context_check_curcontext_id(current_context, OS_CONTEXT_INCREMENTCOUNTER))) {
		return OS_STATUS_CALLEVEL;
	}
	if (UNLIKELY(TRUE == os_interrupt_is_disabled())) {
		return OS_STATUS_DISABLEDINT;
	}
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(FALSE == os_counter_is_valid(counter_id))) {
		return OS_STATUS_ID_1;
	}
#endif
	counter_cfg = os_counter_cfg_ref_table[counter_id];
#ifdef CONFIG_RT_FRAMEWORK_EXTENDED_STATUS
	if (UNLIKELY(TIMERTYPE_SOFTWARE != counter_cfg->timer_type)) {
		return OS_STATUS_ID_TYPE_1;
	}
#endif
	app_cfg = os_app_cfg_ref_table[counter_cfg->owner_app_id];
	if (UNLIKELY(app_cfg->core_id != os_get_core_id())) {
		return OS_STATUS_CORE;
	}
	if (UNLIKELY(APPLICATION_ACCESSIBLE != app_cfg->instance->state)) {
		return OS_STATUS_NOT_ACCESSIBLE;
	}
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	if (UNLIKELY(OS_STATUS_OK != os_app_check_access(OS_CONTEXT_GET_CURRENT_APPID(current_context),
							 counter_cfg->access_app_mask))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif

	os_int_suspend_kernel(&int_state);
	if (counter_cfg->timer_type == TIMERTYPE_SOFTWARE) {
		counter_cfg->obj_desc->hrtimer_sw_value++;
		hrtimer_process(&(counter_cfg->obj_desc->hrtimer_queue));
	}
	os_int_resume_kernel(&int_state);

	return OS_STATUS_OK;
}

FUNC(void, OS_CODE) os_counter_hrt_handler(void)
{
	const os_counter_cfg_t *counter_cfg =
		((os_counter_isr_cfg_t *)OS_CONTEXT_GET_ISR_CFG_REF(os_context_get_contexthdl()))->counter;

	if (LIKELY((NULL != counter_cfg) && (TIMERTYPE_HIGH_RESOLUTION == counter_cfg->timer_type))) {
		hrtimer_process(&(counter_cfg->obj_desc->hrtimer_queue));
	}
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
