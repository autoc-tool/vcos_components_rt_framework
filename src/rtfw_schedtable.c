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
#include "rtfw_schedtable.h"
#include "rtfw_event.h"
#include "rtfw_task.h"
#include "rtfw_core.h"
#include "rtfw_counter.h"
#include "schedtable_cfg.h"
#include "application_cfg.h"
#include "core_cfg.h"

/* Local Macro Definition */
#define SCHED_CHECK_ID(x)                                                                                          \
	((os_schedtable_cfg_ref_table_size != 0UL) && ((x) < os_schedtable_cfg_ref_table_size) &&                      \
	 (os_schedtable_cfg_ref_table[x]->instance->schedtable_id < os_schedtable_cfg_ref_table_size))
#ifndef MIN
#define MIN(_x, _y) (((_x) < (_y)) ? (_x) : (_y))
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
static FUNC(os_status_t, OS_CODE) os_start_scheduletable_rel_startup(ScheduleTableType sid, TickType offset);
static FUNC(os_status_t, OS_CODE) os_start_schedule_table_abs_startup(ScheduleTableType sid, TickType start);
static FUNC(os_status_t, OS_CODE) os_start_schedule_table_synchron_startup(ScheduleTableType schedtable_id);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

/* Local Function Implementation */
static inline os_schedtable_inst_t *os_get_sched_table_by_id(ScheduleTableType schedtable_id)
{
	return os_schedtable_cfg_ref_table[schedtable_id]->instance;
}

static inline const os_schedtable_cfg_t *os_get_sched_table_cfg_by_id(ScheduleTableType schedtable_id)
{
	return os_schedtable_cfg_ref_table[schedtable_id];
}

static inline TickType os_get_sched_table_initial_offset(const os_schedtable_inst_t *schtbl_ptr)
{
	const os_schedtable_cfg_t *cfg_ptr = os_get_sched_table_cfg_by_id(schtbl_ptr->schedtable_id);

	return EXPIRE_POINT_GET(cfg_ptr, 0)->offset;
}

static inline TickType os_get_sched_table_max_value(const os_schedtable_inst_t *schtbl_ptr)
{
	ScheduleTableType schedtable_id = schtbl_ptr->schedtable_id;
	return os_schedtable_cfg_ref_table[schedtable_id]->action_config.counter->max_allowed_value;
}

static inline boolean os_sched_table_is_next_sync_point(const os_schedtable_inst_t *stbl,
							const os_schedtable_cfg_t *cfg_ptr)
{
	/* The tail node will not adjust the virtual tail node */
	if (stbl->current_expiry_index == (cfg_ptr->expiry_point_cnt - 1U)) {
		return (boolean)FALSE;
	}
	/* Virtual tail node, check if the first node is an explicit synchronization point */
	else if (stbl->current_expiry_index == cfg_ptr->expiry_point_cnt) {
		if ((EXPIRE_POINT_GET(cfg_ptr, 0U)->adjust_expiry_point.max_shorten != 0U) ||
		    (EXPIRE_POINT_GET(cfg_ptr, 0U)->adjust_expiry_point.max_lengthen != 0U)) {
			return (boolean)TRUE;
		} else {
			return (boolean)FALSE;
		}
	}
	/* current_expiry_index range = [0, (cfg_ptr->expiry_point_cnt - 2)] */
	else {
		if ((EXPIRE_POINT_GET(cfg_ptr, (stbl->current_expiry_index + 1U))->adjust_expiry_point.max_shorten !=
		     0U) ||
		    (EXPIRE_POINT_GET(cfg_ptr, (stbl->current_expiry_index + 1U))->adjust_expiry_point.max_lengthen !=
		     0U)) {
			return (boolean)TRUE;
		} else {
			return (boolean)FALSE;
		}
	}
}

static inline TickType os_get_sched_table_max_length_of_next(const os_schedtable_inst_t *stbl,
							     const os_schedtable_cfg_t *cfg_ptr)
{
	/* Tail node will not adjust virtual tail node, direct error */
	if (stbl->current_expiry_index == (cfg_ptr->expiry_point_cnt - 1U)) {
		// ReportFatalError();
		return 0U;
	}
	/* Virtual tail node, check the value of the first node */
	else if (stbl->current_expiry_index == cfg_ptr->expiry_point_cnt) {
		return EXPIRE_POINT_GET(cfg_ptr, 0)->adjust_expiry_point.max_lengthen;
	}
	/* current_expiry_index range = [0, (cfg_ptr->expiry_point_cnt - 2)] */
	else {
		return EXPIRE_POINT_GET(cfg_ptr, (stbl->current_expiry_index + 1U))->adjust_expiry_point.max_lengthen;
	}
}

static inline TickType os_get_sched_table_max_shorten_of_next(const os_schedtable_inst_t *stbl,
							      const os_schedtable_cfg_t *cfg_ptr)
{
	/* Tail node will not adjust virtual tail node, direct error */
	if (stbl->current_expiry_index == (cfg_ptr->expiry_point_cnt - 1U)) {
		// ReportFatalError();
		return 0U;
	}
	/* Virtual tail node, check the value of the first node */
	else if (stbl->current_expiry_index == cfg_ptr->expiry_point_cnt) {
		return EXPIRE_POINT_GET(cfg_ptr, 0)->adjust_expiry_point.max_shorten;
	}
	/* current_expiry_index range = [0, (cfg_ptr->expiry_point_cnt - 2)] */
	else {
		return EXPIRE_POINT_GET(cfg_ptr, (stbl->current_expiry_index + 1U))->adjust_expiry_point.max_shorten;
	}
}

static inline void os_get_sched_table_sync_update_deviation(const os_schedtable_cfg_t *schedtable_cfg_ptr,
							    os_schedtable_inst_t *schedtable_ptr,
							    TickType sync_time_now)
{
	TickType sync_time_delta;
	TickType sync_time_delta_half_mod;
	TickType sync_time_calculated;
	TickType counter_time_delta;
	TickType counter_time_now;

	TickType duration_half = ((TickType)(schedtable_cfg_ptr->duration / 2U));
	TickType last_counter_timestamp = schedtable_ptr->sync.counter_time_stamp;
	TickType last_sync_timestamp = schedtable_ptr->sync.sync_time_stamp;

	counter_time_now = os_counter_get_user_value(schedtable_cfg_ptr->action_config.counter);

	/****************************** Calculate counter_time_delta ****************************************/
	counter_time_delta = counter_time_now - last_counter_timestamp;
	if (UNLIKELY(counter_time_now < last_counter_timestamp)) { /* counter value overflow scenario */
		counter_time_delta += (schedtable_cfg_ptr->action_config.counter->max_allowed_value + 1U);
	}

	/*
	* Reason for subtracting only one cycle:
	* last_counter_timestamp is refreshed every schedule table cycle, the difference cannot exceed two Durations,
	* subtraction operation is more efficient than modulo operation
	*/
	if (UNLIKELY(counter_time_delta >= schedtable_cfg_ptr->duration)) {
		counter_time_delta -= schedtable_cfg_ptr->duration;
	}

	/****************************** Calculate sync_time_calculated ***********************************/
	sync_time_calculated = last_sync_timestamp + counter_time_delta;
	if (UNLIKELY(sync_time_calculated >= schedtable_cfg_ptr->duration)) {
		sync_time_calculated -= schedtable_cfg_ptr->duration;
	}

	/****************************** Calculate sync_time_delta ****************************************/
	if (sync_time_now >= sync_time_calculated) {
		sync_time_delta = sync_time_now - sync_time_calculated;
	} else {
		sync_time_delta = (schedtable_cfg_ptr->duration - sync_time_calculated) + sync_time_now;
	}

	/****************************** Calculate sync_time_delta_half_mod ********************************/
	/* Time difference can be positive or negative, sync_time_delta_half_mod range [-DurationHalf, DurationHalf-1] */
	if (sync_time_delta >= duration_half) {
		sync_time_delta_half_mod = sync_time_delta - schedtable_cfg_ptr->duration;
	} else {
		sync_time_delta_half_mod = sync_time_delta;
	}

	/****************************** Calculate deviation *******************************************/
	TickType result = schedtable_ptr->sync.deviation - sync_time_delta_half_mod;

	if ((schedtable_ptr->sync.deviation < duration_half) &&
	    (sync_time_delta_half_mod >= duration_half) &&
	    (result >= duration_half)) {

		/* 1. the deviation is positive
		*  2. the sync_time_delta_half_mod is negative
		*  3. result exceeds the duration_half limit ,then move the result to negative part
		*/
		result -= schedtable_cfg_ptr->duration;
	} else if ((schedtable_ptr->sync.deviation >= duration_half) &&
		   (sync_time_delta_half_mod < duration_half) &&
		   ((-result) > duration_half)) {

		/* 1. the deviation is negative
		*  2. the duration_half is positive
		*  3. calculation result exceeds negative duration_half，then move the result to positive part
		*/
		result += schedtable_cfg_ptr->duration;
	} else {
		/* nothing to do */
	}

	schedtable_ptr->sync.deviation = result;

	/****************************** Record the current time for next calculation ****************************************/
	schedtable_ptr->sync.counter_time_stamp = counter_time_now;
	schedtable_ptr->sync.sync_time_stamp = sync_time_now;
}

static inline void os_get_sched_table_sync_update_status(const os_schedtable_cfg_t *schedtable_cfg_ptr)
{
	os_schedtable_inst_t *schedtable_ptr = schedtable_cfg_ptr->instance;

	if (schedtable_ptr->sync.sync_state == SCHTBL_SYNCSTATUS_PROVIDED) {
		if (schedtable_ptr->sync.deviation <= schedtable_cfg_ptr->precision) {
			schedtable_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
		} else if (-(schedtable_ptr->sync.deviation) <= schedtable_cfg_ptr->precision ) {
			schedtable_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
		} else {
			schedtable_ptr->state = SCHEDULETABLE_RUNNING;
		}
	} else {
		/* sync_state is NOT_PROVIDED — state remains as-is, no update needed */
	}
}

static inline void os_get_sched_table_reduce_deviation(os_schedtable_inst_t *stbl, const os_schedtable_cfg_t *cfg_ptr)
{
	TickType correction;
	TickType max_length_of_next = os_get_sched_table_max_length_of_next(stbl, cfg_ptr);
	TickType max_shorten_of_next = os_get_sched_table_max_shorten_of_next(stbl, cfg_ptr);

	if (((stbl->sync.deviation) > ((TickType)0)) && (stbl->sync.deviation > max_length_of_next)) {
		/* 0 < max_length_of_next < deviation */
		correction = max_length_of_next;
	} else if (((stbl->sync.deviation) <= ((TickType)0)) &&
		(stbl->sync.deviation < max_shorten_of_next)) {
		/* deviation < max_shorten_of_next < 0 */
		correction = max_shorten_of_next;
	} else {
		/* other conditions */
		correction = stbl->sync.deviation;
	}

	/* Single step deviation for subsequent execution */
	stbl->sync.one_step_adjust = correction;

	/* Record the remaining deviation to be eliminated */
	stbl->sync.deviation -= correction;
}

static inline void os_update_sched_table_sync_timestamp(os_schedtable_inst_t *stbl, const os_schedtable_cfg_t *cfg_ptr)
{
	TickType last_counter_timestamp = stbl->sync.counter_time_stamp;
	TickType last_sync_timestamp = stbl->sync.sync_time_stamp;

	TickType counter_time_now = os_counter_get_user_value(cfg_ptr->action_config.counter);

	/****************************** Calculate diff_counter ****************************************/
	TickType diff_counter = counter_time_now - last_counter_timestamp;

	if (counter_time_now < last_counter_timestamp) { /* Overflow scenario */
		diff_counter += (cfg_ptr->action_config.counter->max_allowed_value + 1U);
	}

	/*
	* Reason for subtracting only one cycle:
	* last_counter_timestamp is refreshed every schedule table cycle, the difference cannot exceed two Durations,
	* subtraction operation is more efficient than modulo operation
	*/
	if (diff_counter >= cfg_ptr->duration) {
		diff_counter -= cfg_ptr->duration;
	}

	/****************************** Update driving counter ****************************************/
	stbl->sync.counter_time_stamp = counter_time_now;
	stbl->sync.sync_time_stamp = last_sync_timestamp + diff_counter;
	if (stbl->sync.sync_time_stamp >= cfg_ptr->duration) {
		stbl->sync.sync_time_stamp -= cfg_ptr->duration;
	}
}

static inline void os_sched_table_sync_expiry_point(os_schedtable_inst_t *stbl, const os_schedtable_cfg_t *cfg_ptr)
{
	/* Update driving counter */
	os_update_sched_table_sync_timestamp(stbl, cfg_ptr);

	/* Eliminate deviation */
	if (stbl->sync.deviation != 0U) {
		os_get_sched_table_reduce_deviation(stbl, cfg_ptr);
	}

	/* Update schedule table status, external interface SyncScheduleTable may affect status transitions */
	os_get_sched_table_sync_update_status(cfg_ptr);
}

static inline void os_sched_table_sync_expiry_point_zero(os_schedtable_inst_t *stbl, const os_schedtable_cfg_t *cfg_ptr)
{
	/* Update driving counter */
	os_update_sched_table_sync_timestamp(stbl, cfg_ptr);

	/* Update schedule table status, external interface SyncScheduleTable may affect status transitions */
	os_get_sched_table_sync_update_status(cfg_ptr);
}

/* global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

static FUNC(void, OS_CODE) os_update_sched_table_state(os_schedtable_inst_t *stbl)
{
	TickType delta;
	TickType initial_offset;
	TickType final_offset;
	os_schedtable_inst_t *next_schedtable_ptr;
	const os_schedtable_cfg_t *next_schedtable_cfg_ptr;
	const os_schedtable_cfg_t *cfg_ptr = os_get_sched_table_cfg_by_id(stbl->schedtable_id);

	/* For explicit synchronization schedule tables at each expiry point + virtual tail node, update the synchronization counter */
	if (cfg_ptr->sync_strategy == EXPLICIT) {
		if (os_sched_table_is_next_sync_point(stbl, cfg_ptr) == (boolean)TRUE) {
			/* Next expiry point is an explicit synchronization point */
			os_sched_table_sync_expiry_point(stbl, cfg_ptr);
		} else {
			/* Next expiry point is not an explicit synchronization point */
			os_sched_table_sync_expiry_point_zero(stbl, cfg_ptr);
		}
	}

	if ((stbl->current_expiry_index) == (cfg_ptr->expiry_point_cnt - 1U)) { /* At the last expiry point */
		final_offset = cfg_ptr->duration - EXPIRE_POINT_GET(cfg_ptr, cfg_ptr->expiry_point_cnt - 1UL)->offset;

		/* Tail node does not do explicit synchronization adjustment */
		os_counter_reload_action(&cfg_ptr->action_config, final_offset);
		stbl->current_expiry_index++;
		return;
	}

	if ((stbl->current_expiry_index) == cfg_ptr->expiry_point_cnt) { /* At final offset */
		if ((cfg_ptr->repeating == SCHTBL_REPEATING) || (stbl->next != NULL)) {
			if (stbl->next == NULL) {  /* Calculate the first expiry point of a periodic schedule table */

				initial_offset = os_get_sched_table_initial_offset(stbl);

				/* initial_offset and one_step_adjust satisfy the following conditions:
				* 1. delta is only positive
				* 2. one_step_adjust can be positive or negative
				* 3. |delta| > |one_step_adjust|
				*/
				if (stbl->sync.one_step_adjust != 0U) {
					os_counter_reload_action(&cfg_ptr->action_config,
								 initial_offset + stbl->sync.one_step_adjust);
					stbl->sync.one_step_adjust = 0U;
				} else {
					os_counter_reload_action(&cfg_ptr->action_config, initial_offset);
				}
			} else { /* Next schedule table */
				next_schedtable_ptr = stbl->next;
				stbl->next = NULL_PTR;

				initial_offset = os_get_sched_table_initial_offset(next_schedtable_ptr);
				stbl->state = SCHEDULETABLE_STOPPED;
				next_schedtable_ptr->state = SCHEDULETABLE_RUNNING;
				next_schedtable_cfg_ptr = os_get_sched_table_cfg_by_id(next_schedtable_ptr->schedtable_id);

				os_counter_add_rel_action(&next_schedtable_cfg_ptr->action_config, initial_offset);
			}
		} else {
			stbl->state = SCHEDULETABLE_STOPPED;
		}
		stbl->current_expiry_index = 0;
	} else { /* Calculate between multiple expiry points in a schedule table */
		delta = EXPIRE_POINT_GET(cfg_ptr, stbl->current_expiry_index + 1UL)->offset -
			EXPIRE_POINT_GET(cfg_ptr, stbl->current_expiry_index)->offset;

		/* delta and one_step_adjust satisfy the following conditions:
		* 1. delta is only positive
		* 2. one_step_adjust can be positive or negative
		* 3. |delta| > |one_step_adjust|
		*/
		if (stbl->sync.one_step_adjust != 0U) {
			os_counter_reload_action(&cfg_ptr->action_config, delta + stbl->sync.one_step_adjust);
			stbl->sync.one_step_adjust = 0U;
		} else {
			os_counter_reload_action(&cfg_ptr->action_config, delta);
		}

		stbl->current_expiry_index++;
	}

	return;
}

static FUNC(os_status_t, OS_CODE) os_sched_table_check_consistency(const os_schedtable_inst_t *schedtbl_ptr)
{
	const os_schedtable_cfg_t *cfg_ptr = os_get_sched_table_cfg_by_id(schedtbl_ptr->schedtable_id);
	TickType max_allowed_value = os_get_sched_table_max_value(schedtbl_ptr);
	TickType prev_offset = 0U;
	TickType curr_offset = 0U;
	TickType last_offset = 0U;
	TickType offset_diff = 0U;
	TickType final_diff = 0U;
	sint32 i = 0;

	/* Check if expiry points exist */
	if (cfg_ptr->expiry_point_cnt == 0UL) {
		return OS_STATUS_NOT_OK;
	}

	/* Check sync strategy constraints */
	if (cfg_ptr->sync_strategy == IMPLICIT) {
		/* For IMPLICIT sync, duration must equal max_allowed_value + 1 */
		if (cfg_ptr->duration != (max_allowed_value + 1UL)) {
			return OS_STATUS_NOT_OK;
		}
	} else if (cfg_ptr->sync_strategy == EXPLICIT) {
		/* For EXPLICIT sync, duration must not exceed max_allowed_value + 1 */
		if (cfg_ptr->duration > (max_allowed_value + 1UL)) {
			return OS_STATUS_NOT_OK;
		}
	} else {
		/* NO_SYNC strategy: no duration constraint applies */
	}

	/* Validate each expiry point's offset */
	for (i = 0U; i < (sint32)cfg_ptr->expiry_point_cnt; i++) {
		curr_offset = EXPIRE_POINT_GET(cfg_ptr, i)->offset;
		offset_diff = curr_offset - prev_offset;

		/* Check if offset is monotonically increasing */
		if (curr_offset <= prev_offset && i > 0) {
			return OS_STATUS_NOT_OK;
		}

		/* Check if offset difference exceeds max allowed value */
		if (offset_diff > max_allowed_value) {
			return OS_STATUS_NOT_OK;
		}

		prev_offset = curr_offset;
	}

	last_offset = EXPIRE_POINT_GET(cfg_ptr, cfg_ptr->expiry_point_cnt - 1UL)->offset;
	final_diff = cfg_ptr->duration - last_offset;

	/* Final duration must be positive and not exceed max allowed value */
	/* For IMPLICIT sync, final_diff can equal max_allowed_value + 1 when last_offset is 0 */
	if (cfg_ptr->sync_strategy == IMPLICIT) {
		if ((final_diff == 0UL) || (final_diff > (max_allowed_value + 1UL))) {
			return OS_STATUS_NOT_OK;
		}
	} else {
		if ((final_diff == 0UL) || (final_diff > max_allowed_value)) {
			return OS_STATUS_NOT_OK;
		}
	}

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_sched_table_validity_paras(const os_schedtable_cfg_t *os_schedtable_cfg)
{
	const os_schedtable_inst_t *schedtable_ptr;
	os_status_t os_schedtbl_para_validity_result;

	if (os_schedtable_cfg == NULL) {
		return OS_STATUS_NOT_OK;
	}

	schedtable_ptr = os_schedtable_cfg->instance;

	os_schedtbl_para_validity_result = os_sched_table_check_consistency(schedtable_ptr);
	if (os_schedtbl_para_validity_result != OS_STATUS_OK) {
		return OS_STATUS_NOT_OK;
	}

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_start_schedule_table_rel(ScheduleTableType schedtable_id, TickType offset)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	TickType max_offset;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_STARTSCHEDULETABLEREL);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schtbl_ptr = os_get_sched_table_by_id(schedtable_id);
		cfg_ptr = os_get_sched_table_cfg_by_id(schtbl_ptr->schedtable_id);

		max_offset = os_get_sched_table_max_value(schtbl_ptr);
		if ((offset == (TickType)0) ||
		    ((offset + os_get_sched_table_initial_offset(schtbl_ptr)) > max_offset)) {
			return_value = OS_STATUS_VALUE_OUT_OF_RANGE_2;
			return return_value;
		}

		os_int_suspend_kernel(&initial_state);
		if (schtbl_ptr->state != SCHEDULETABLE_STOPPED) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED;
		}

		os_counter_add_rel_action(&cfg_ptr->action_config,
					  offset + os_get_sched_table_initial_offset(schtbl_ptr));
		schtbl_ptr->state = SCHEDULETABLE_RUNNING;
		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_start_schedule_table_rel((uint32)core_id, schedtable_id, offset);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE) os_start_schedule_table_abs(ScheduleTableType schedtable_id, TickType start)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schedtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_STARTSCHEDULETABLEABS);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schedtbl_ptr = os_get_sched_table_by_id(schedtable_id);
		cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);

		if (start > os_get_sched_table_max_value(schedtbl_ptr)) {
			return OS_STATUS_VALUE_OUT_OF_RANGE_1;
		}

		os_int_suspend_kernel(&initial_state);

		if (schedtbl_ptr->state != SCHEDULETABLE_STOPPED) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED;
		}

		os_counter_add_abs_action(&cfg_ptr->action_config,
				  os_counter_add(start, os_get_sched_table_initial_offset(schedtbl_ptr),
						 os_get_sched_table_max_value(schedtbl_ptr)));

		if (cfg_ptr->sync_strategy == IMPLICIT) {
			schedtbl_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
		} else {
			schedtbl_ptr->state = SCHEDULETABLE_RUNNING;
		}

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_start_schedule_table_abs((uint32)core_id, schedtable_id, start);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE) os_start_schedule_table_synchron(ScheduleTableType schedtable_id)
{
	os_schedtable_inst_t *schedtable_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	irqlevel_t initial_state;
	os_status_t return_value = OS_STATUS_OK;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_STARTSCHEDULETABLESYNCHRON);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);
		cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);

		if (cfg_ptr->sync_strategy != EXPLICIT) {
			return OS_STATUS_SYNC_TYPE_ID;
		}

		os_int_suspend_kernel(&initial_state);

		if (schedtable_ptr->state != SCHEDULETABLE_STOPPED) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED;
		}

		schedtable_ptr->state = SCHEDULETABLE_WAITING;

		/*Initial variable */
		schedtable_ptr->sync.sync_state = SCHTBL_SYNCSTATUS_NOT_PROVIDED;
		schedtable_ptr->sync.deviation = 0;
		schedtable_ptr->sync.sync_time_stamp = 0;
		schedtable_ptr->sync.counter_time_stamp = 0;
		schedtable_ptr->sync.one_step_adjust = 0;

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_start_schedule_table_synchron((uint32)core_id, schedtable_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE) os_stop_schedule_table(ScheduleTableType schedtable_id)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_STOPSCHEDULETABLE);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schtbl_ptr = os_get_sched_table_by_id(schedtable_id);
		cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);

		os_int_suspend_kernel(&initial_state);

		if (schtbl_ptr->state == SCHEDULETABLE_STOPPED) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED_1;
		}

		if ((schtbl_ptr->state == SCHEDULETABLE_RUNNING) ||
		    (schtbl_ptr->state == SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS)) {
			os_counter_delete_action(&cfg_ptr->action_config);
		}

		if (schtbl_ptr->next != NULL_PTR) {
			schtbl_ptr->next->state = SCHEDULETABLE_STOPPED;
			schtbl_ptr->next = NULL_PTR;
		}

		schtbl_ptr->current_expiry_index = 0U;
		schtbl_ptr->state = SCHEDULETABLE_STOPPED;

		os_int_resume_kernel(&initial_state);

	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_stop_schedule_table((uint32)core_id, schedtable_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE) os_set_schedule_table_app_id(ScheduleTableType schedtable_id, ApplicationIDType app_id)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schedtable_ptr;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_SETSCHEDULETABLEAPPID);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);

		os_int_suspend_kernel(&initial_state);

		/* Notify all running schedule tables */
		if ((schedtable_ptr->state == SCHEDULETABLE_RUNNING) ||
		    (schedtable_ptr->state == SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS)) {
			schedtable_ptr->app_status |= 1U << (uint32)app_id;
		}

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_set_schedule_table_app_id((uint32)core_id, schedtable_id, app_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE) os_clear_schedule_table_app_id(ScheduleTableType schedtable_id, ApplicationIDType app_id)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schedtable_ptr;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_CLEARSCHEDULETABLEAPPID);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);

		os_int_suspend_kernel(&initial_state);

		schedtable_ptr->app_status_mask |= 1U << (uint32)app_id;

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_clear_schedule_table_app_id((uint32)core_id, schedtable_id, app_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE)
os_next_schedule_table(ScheduleTableType schedule_table_id_from, ScheduleTableType schedule_table_id_to)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *sched_from_ptr;
	os_schedtable_inst_t *sched_to_ptr;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedule_table_id_from, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_NEXTSCHEDULETABLE);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedule_table_id_from) == (boolean)FALSE ||
		SCHED_CHECK_ID(schedule_table_id_to) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedule_table_id_from)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedule_table_id_to)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		sched_from_ptr = os_get_sched_table_by_id(schedule_table_id_from);
		sched_to_ptr = os_get_sched_table_by_id(schedule_table_id_to);

		os_int_suspend_kernel(&initial_state);

		if (UNLIKELY((sched_from_ptr->state == SCHEDULETABLE_STOPPED) || (sched_from_ptr->state == SCHEDULETABLE_NEXT))) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED_1;
		}

		if (UNLIKELY(sched_to_ptr->state != SCHEDULETABLE_STOPPED)) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED;
		}

		if (UNLIKELY(sched_from_ptr->next != NULL)) {
			sched_from_ptr->next->state = SCHEDULETABLE_STOPPED;
		}

		sched_from_ptr->next = sched_to_ptr;
		sched_to_ptr->state = SCHEDULETABLE_NEXT;
		sched_to_ptr->current_expiry_index = 0;

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_next_schedule_table((uint32)core_id, schedule_table_id_from, schedule_table_id_to);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE)
os_next_schedule_table_from_stop(ScheduleTableType schedule_table_id_from, ScheduleTableType schedule_table_id_to)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *sched_from_ptr;
	os_schedtable_inst_t *sched_to_ptr;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedule_table_id_from, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_NEXTSCHEDULETABLEFROMSTOP);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedule_table_id_from) == (boolean)FALSE ||
			SCHED_CHECK_ID(schedule_table_id_to) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedule_table_id_from)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedule_table_id_to)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		sched_from_ptr = os_get_sched_table_by_id(schedule_table_id_from);
		sched_to_ptr = os_get_sched_table_by_id(schedule_table_id_to);

		os_int_suspend_kernel(&initial_state);

		if (UNLIKELY(sched_from_ptr->state != SCHEDULETABLE_STOPPED)) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED_1;
		}

		if (UNLIKELY(sched_to_ptr->state != SCHEDULETABLE_STOPPED)) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED;
		}

		if (UNLIKELY(sched_from_ptr->next != NULL)) {
			sched_from_ptr->next->state = SCHEDULETABLE_STOPPED;
		}

		sched_from_ptr->next = sched_to_ptr;
		sched_to_ptr->state = SCHEDULETABLE_NEXT;
		sched_to_ptr->current_expiry_index = 0;

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_next_schedule_table_from_stop((uint32)core_id, schedule_table_id_from,
								schedule_table_id_to);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE) os_sync_schedule_table(ScheduleTableType schedtable_id, TickType sync_time_now)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schedtable_ptr;
	const os_schedtable_cfg_t *schedtable_cfg_ptr;
	TickType startTimeToWait = 0U;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_SYNCSCHEDULETABLE);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);
		schedtable_cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);

		os_int_suspend_kernel(&initial_state);
		if (UNLIKELY((schedtable_ptr->state == SCHEDULETABLE_STOPPED) || (schedtable_ptr->state == SCHEDULETABLE_NEXT))) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED;
		}

		switch (schedtable_ptr->state) {
			case SCHEDULETABLE_WAITING:
				schedtable_ptr->sync.deviation = 0;
				schedtable_ptr->sync.sync_time_stamp = sync_time_now;
				schedtable_ptr->sync.counter_time_stamp =
					os_counter_get_user_value(schedtable_cfg_ptr->action_config.counter);
				schedtable_ptr->sync.sync_state = SCHTBL_SYNCSTATUS_PROVIDED;
				schedtable_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
				schedtable_ptr->sync.one_step_adjust = 0;

				startTimeToWait =
					(schedtable_cfg_ptr->duration - sync_time_now) % schedtable_cfg_ptr->duration;

				os_counter_add_rel_action(&schedtable_cfg_ptr->action_config,
							  startTimeToWait +
								  os_get_sched_table_initial_offset(schedtable_ptr));
				break;

			case SCHEDULETABLE_RUNNING:
			case SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS:
				if (schedtable_ptr->sync.sync_state == SCHTBL_SYNCSTATUS_NOT_PROVIDED) {
					schedtable_ptr->sync.sync_state = SCHTBL_SYNCSTATUS_PROVIDED;
				}

				/* Update deviation */
				os_get_sched_table_sync_update_deviation(schedtable_cfg_ptr, schedtable_ptr,
									 sync_time_now);
				/* Update status */
				os_get_sched_table_sync_update_status(schedtable_cfg_ptr);

				break;

			default:
				/* do nothing */
				break;
		}

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_sync_schedule_table((uint32)core_id, schedtable_id, sync_time_now);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE) os_deviate_schedule_table(ScheduleTableType schedtable_id, DeviateType deviate_value)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schedtable_ptr;
	const os_schedtable_cfg_t *schedtable_cfg_ptr;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_DEVIATESCHEDULETABLE);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);
		schedtable_cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);

		if (schedtable_cfg_ptr->sync_strategy != EXPLICIT) {
			return OS_STATUS_SYNC_TYPE_ID;
		}

		if (deviate_value > ((DeviateType)0)) {
			if ((TickType)deviate_value > schedtable_cfg_ptr->duration) {
				return OS_STATUS_VALUE_TOO_HIGH_2;
			}
		} else {
			TickType result_inverted = (TickType)(-deviate_value);
			if (result_inverted > schedtable_cfg_ptr->duration) {
				return OS_STATUS_VALUE_TOO_LOW_2;
			}
		}

		os_int_suspend_kernel(&initial_state);

		if ((schedtable_ptr->state == SCHEDULETABLE_STOPPED) ||
		    (schedtable_ptr->state == SCHEDULETABLE_NEXT)) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED;
		}

		if (schedtable_ptr->sync.sync_state == SCHTBL_SYNCSTATUS_NOT_PROVIDED) {
			schedtable_ptr->sync.sync_state = SCHTBL_SYNCSTATUS_PROVIDED;
		}

		/* Update deviation value */
		schedtable_ptr->sync.deviation = deviate_value;
		schedtable_ptr->sync.one_step_adjust = 0U;
		/* Update driving counter */
		os_update_sched_table_sync_timestamp(schedtable_ptr, schedtable_cfg_ptr);
		/* Update status */
		os_get_sched_table_sync_update_status(schedtable_cfg_ptr);

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_deviate_schedule_table((uint32)core_id, schedtable_id, deviate_value);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE)
os_get_schedule_table_status(ScheduleTableType schedtable_id, ScheduleTableStatusRefType ScheduleStatus)
{
	os_status_t return_value = OS_STATUS_OK;
	const os_schedtable_inst_t *schedtable_ptr;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_GETSCHEDULETABLESTATUS);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);

		os_int_suspend_kernel(&initial_state);

		switch (schedtable_ptr->state) {
			case SCHEDULETABLE_STOPPED:
			case SCHEDULETABLE_NEXT:
			case SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS:
			case SCHEDULETABLE_WAITING:
			case SCHEDULETABLE_RUNNING:
				*ScheduleStatus = schedtable_ptr->state;
				break;
			default:
				/* do nothing */
				break;
		}

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_get_schedule_table_status((uint32)core_id, schedtable_id, ScheduleStatus);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

FUNC(os_status_t, OS_CODE) os_set_schedule_table_async(ScheduleTableType schedtable_id)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schedtable_ptr;
	irqlevel_t initial_state;
	CoreIdType core_id = OS_CORE_CNT;
	const os_ctx_cfg_t *cur_context = os_context_get_contexthdl();
	boolean check_result;

	return_value = get_sched_table_core_id(schedtable_id, &core_id);
	if (UNLIKELY((OS_STATUS_OK != return_value) || (((OsObjIndexType)core_id) >= os_core_cfg_ref_table_size))) {
		return OS_STATUS_ID_1;
	}

	if (core_id == os_get_core_id()) {
		check_result = os_context_check_curcontext_id(cur_context, OS_CONTEXT_SETSCHEDULETABLEASYNC);
		if (UNLIKELY(FALSE == check_result)) {
			return OS_STATUS_CALLEVEL;
		}
		check_result = os_interrupt_is_disabled();
		if (UNLIKELY(TRUE == check_result)) {
			return OS_STATUS_DISABLEDINT;
		}
		if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
			return OS_STATUS_ID_1;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		if (UNLIKELY(os_app_check_is_accessible(GET_APP_CFG_BY_SCHEDULETABLE_ID \
			(schedtable_id)) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif

		schedtable_ptr = os_get_sched_table_by_id(schedtable_id);

		os_int_suspend_kernel(&initial_state);

		if (UNLIKELY((schedtable_ptr->state != SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS) &&
			     (schedtable_ptr->state != SCHEDULETABLE_RUNNING))) {
			os_int_resume_kernel(&initial_state);
			return OS_STATUS_STOPPED_1;
		}

		schedtable_ptr->state = SCHEDULETABLE_RUNNING;

		os_int_resume_kernel(&initial_state);
	} else {
#if defined(CONFIG_RT_FRAMEWORK_IPICALL) && (CONFIG_RT_FRAMEWORK_IPICALL == 1)
		return os_ipicall_set_schedule_table_async((uint32)core_id, schedtable_id);
#else
		return OS_STATUS_ID_1;
#endif /* CONFIG_RT_FRAMEWORK_IPICALL */
	}

	return return_value;
}

static FUNC(os_status_t, OS_CODE) os_start_scheduletable_rel_startup(ScheduleTableType sid, TickType offset)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	TickType max_offset;
	irqlevel_t initial_state;

	if (SCHED_CHECK_ID(sid) == (boolean)FALSE) {
		return OS_STATUS_ID_1;
	}

	schtbl_ptr = os_get_sched_table_by_id(sid);
	cfg_ptr = os_get_sched_table_cfg_by_id(sid);

	/* EXPLICIT or IMPLICIT */
	if (UNLIKELY(cfg_ptr->sync_strategy == IMPLICIT)) {
		return OS_STATUS_ID_1;
	}

	max_offset = os_get_sched_table_max_value(schtbl_ptr);
	if (UNLIKELY((offset == 0UL) || ((offset + os_get_sched_table_initial_offset(schtbl_ptr)) > max_offset))) {
		return OS_STATUS_VALUE_OUT_OF_RANGE_2;
	}

	os_int_suspend_kernel(&initial_state);

	if (UNLIKELY(schtbl_ptr->state != SCHEDULETABLE_STOPPED)) {
		os_int_resume_kernel(&initial_state);
		return OS_STATUS_STOPPED;
	}

	/* calculate the expire value.. */
	os_counter_add_rel_action(&cfg_ptr->action_config, offset + os_get_sched_table_initial_offset(schtbl_ptr));
	schtbl_ptr->state = SCHEDULETABLE_RUNNING;

	os_int_resume_kernel(&initial_state);

	return return_value;
}

static FUNC(os_status_t, OS_CODE) os_start_schedule_table_abs_startup(ScheduleTableType sid, TickType start)
{
	os_status_t return_value = OS_STATUS_OK;
	os_schedtable_inst_t *schedtbl_ptr;
	const os_schedtable_cfg_t *cfg_ptr;
	irqlevel_t initial_state;

	if (UNLIKELY(SCHED_CHECK_ID(sid) == (boolean)FALSE)) {
		return OS_STATUS_ID_1;
	}

	schedtbl_ptr = os_get_sched_table_by_id(sid);
	cfg_ptr = os_get_sched_table_cfg_by_id(sid);

	if (UNLIKELY(start > os_get_sched_table_max_value(schedtbl_ptr))) {
		return OS_STATUS_VALUE_TOO_HIGH_1;
	}

	os_int_suspend_kernel(&initial_state);

	if (UNLIKELY(schedtbl_ptr->state != SCHEDULETABLE_STOPPED)) {
		os_int_resume_kernel(&initial_state);
		return OS_STATUS_STOPPED;
	}

	os_counter_add_abs_action(&cfg_ptr->action_config,
				  os_counter_add(start, os_get_sched_table_initial_offset(schedtbl_ptr),
						 os_get_sched_table_max_value(schedtbl_ptr)));

	if (cfg_ptr->sync_strategy == IMPLICIT) {
		schedtbl_ptr->state = SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS;
	} else {
		schedtbl_ptr->state = SCHEDULETABLE_RUNNING;
	}

	os_int_resume_kernel(&initial_state);

	return return_value;
}

static FUNC(os_status_t, OS_CODE) os_start_schedule_table_synchron_startup(ScheduleTableType schedtable_id)
{
	os_schedtable_inst_t *schedtable_ptr;
	const os_schedtable_cfg_t *schedtable_cfg_ptr;
	irqlevel_t initial_state;

	os_status_t return_value = OS_STATUS_OK;

	if (UNLIKELY(os_interrupt_is_disabled() == (boolean)TRUE)) {
		return OS_STATUS_DISABLEDINT;
	}
	if (UNLIKELY(SCHED_CHECK_ID(schedtable_id) == (boolean)FALSE)) {
		return OS_STATUS_ID_1;
	}
	schedtable_ptr = os_get_sched_table_by_id(schedtable_id);
	schedtable_cfg_ptr = os_get_sched_table_cfg_by_id(schedtable_id);
	if (UNLIKELY(schedtable_cfg_ptr->sync_strategy != EXPLICIT)) {
        return OS_STATUS_SYNC_TYPE_ID;
    }

	os_int_suspend_kernel(&initial_state);

	if (UNLIKELY(schedtable_ptr->state != SCHEDULETABLE_STOPPED)) {
		os_int_resume_kernel(&initial_state);
		return OS_STATUS_STOPPED;
	}

	schedtable_ptr->state = SCHEDULETABLE_WAITING;

	os_int_resume_kernel(&initial_state);

	return return_value;
}

FUNC(void, OS_CODE)
os_schedule_table_init(const os_schedtable_cfg_t *const os_schedtable_cfg_table[], OsObjIndexType size)
{
	OsObjIndexType index;
	const os_schedtable_cfg_t *schdtable_cfg;
	os_counter_inst_t *counter_ptr;
	os_schedtable_inst_t *schtbl_ptr;
	os_status_t os_schedtable_init_result;
	const os_app_cfg_t *application_cfg = NULL;
	CoreIdType core_id = os_get_core_id();

	if ((os_schedtable_cfg_table == NULL) || (size == 0U)) {
		return;
	}

	/* Initialize all schedule table IDs (maintain global consistency) */
	for (index = 0U; index < size; index++) {
		schdtable_cfg = (const os_schedtable_cfg_t *)os_schedtable_cfg_table[index];
		schtbl_ptr = os_schedtable_cfg_table[index]->instance;
		schtbl_ptr->schedtable_id = schdtable_cfg->schedtable_id;
	}

	/* Connect schedule tables to counters - only process tables for current core */
	for (ScheduleTableType i = 0U; i < size; i++) {
		schdtable_cfg = os_schedtable_cfg_table[i];
		application_cfg = os_app_cfg_ref_table[schdtable_cfg->app_owner_id];

		/* Skip schedule tables not belonging to current core */
		if (application_cfg->core_id != core_id) {
			continue;
		}

		os_schedtable_init_result = os_sched_table_validity_paras(os_schedtable_cfg_table[i]);
		if (os_schedtable_init_result != OS_STATUS_OK) {
			os_schedtable_cfg_table[i]->instance->schedtable_id = (uint16)OS_SCHED_TABLE_CNT;
			continue;
		}

		schtbl_ptr = os_schedtable_cfg_table[i]->instance;
		counter_ptr = schdtable_cfg->action_config.counter->obj_desc;
		schtbl_ptr->list_schedtable_node = counter_ptr->schedtable_list_first;
		counter_ptr->schedtable_list_first = schtbl_ptr;
	}

	/* Initialize auto-start schedule tables - only process tables for current core */
	for (ScheduleTableType j = 0U; j < size; j++) {
		schdtable_cfg = os_schedtable_cfg_table[j];
		application_cfg = os_app_cfg_ref_table[schdtable_cfg->app_owner_id];

		/* Skip schedule tables not belonging to current core */
		if (application_cfg->core_id != core_id) {
			continue;
		}

		const os_schedtable_autostart_t *autostart_ptr = &(schdtable_cfg->autostart);

		if (autostart_ptr->autostart_mode == TRUE) {
			if (autostart_ptr->type == SCHTBL_AUTOSTART_ABSOLUTE) {
				(void)os_start_schedule_table_abs_startup(schdtable_cfg->schedtable_id, autostart_ptr->offset);
			} else if (autostart_ptr->type == SCHTBL_AUTOSTART_RELATIVE) {
				(void)os_start_scheduletable_rel_startup(schdtable_cfg->schedtable_id, autostart_ptr->offset);
			} else if (autostart_ptr->type == SCHTBL_AUTOSTART_SYNCHRONE) {
				(void)os_start_schedule_table_synchron_startup(schdtable_cfg->schedtable_id);
			} else {
				/* do nothing */
			}
		}
	}
}

FUNC(void, OS_CODE) schedule_table_action_callback(struct hrtimer_s *hrtimer)
{
	const os_action_cfg_t *action = (const os_action_cfg_t *)(hrtimer->arg);
	const os_schedtable_cfg_t *cfg_ptr = OS_GET_SCHEDTABLE_CFG_BY_ACTION_ADDR(action);
	os_schedtable_inst_t *schedtable_ptr = cfg_ptr->instance;

	if ((schedtable_ptr->state == SCHEDULETABLE_RUNNING) ||
	    (schedtable_ptr->state == SCHEDULETABLE_RUNNING_AND_SYNCHRONOUS)) {
		ScheduleTableType sid = schedtable_ptr->schedtable_id;

		cfg_ptr = os_get_sched_table_cfg_by_id(sid);
		if (schedtable_ptr->current_expiry_index < cfg_ptr->expiry_point_cnt) {
			const os_schedtable_expiry_point_t *exp_point;
			uint32 i;

			exp_point = EXPIRE_POINT_GET(cfg_ptr, schedtable_ptr->current_expiry_index);

			for (i = 0U; i < (uint32_t)exp_point->task_list_cnt; i++) {
				(void)os_activate_task_asyn(exp_point->task_list[i]);
			}

			for (i = 0U; i < (uint32_t)exp_point->event_list_cnt; i++) {
				(void)os_set_event_asyn(exp_point->event_list[i].task, exp_point->event_list[i].mask);
			}
		}

		os_update_sched_table_state(schedtable_ptr);
	}
}

FUNC(void, OS_CODE) os_schedule_table_kill(const os_schedtable_cfg_t *const schedtable_ref)
{
	os_stop_schedule_table(schedtable_ref->schedtable_id);
}

FUNC(os_status_t, OS_CODE) get_sched_table_core_id(ScheduleTableType schedtable_id, CoreIdType *core_id_ref)
{
	ApplicationIDType application_id;
	const os_app_cfg_t *application_cfg;
	const os_schedtable_cfg_t *schedtable_cfg_ptr;

	if (UNLIKELY(core_id_ref == NULL)) {
		return OS_STATUS_PARAM_POINTER_2;
	}

	if (UNLIKELY(schedtable_id >= os_schedtable_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	schedtable_cfg_ptr = os_schedtable_cfg_ref_table[schedtable_id];

	if (UNLIKELY(schedtable_cfg_ptr == NULL)) {
		return OS_STATUS_ID_1;
	}

	application_id = schedtable_cfg_ptr->app_owner_id;
	if (UNLIKELY(application_id >= os_app_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	application_cfg = os_app_cfg_ref_table[application_id];

	if (UNLIKELY(application_cfg == NULL)) {
		return OS_STATUS_ID_1;
	}

	*core_id_ref = application_cfg->core_id;

	return OS_STATUS_OK;
}
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
