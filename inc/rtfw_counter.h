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

#ifndef __OS_COUNTER_H
#define __OS_COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_counter_internal.h"

/* Global Macro Definition */
#define COUNTERCFGVAR(counter_id) RTFW_LINK_ENTRY_NAME(os_counter, counter_id)
#define COUNTEROBJVAR(counter_id) RTFW_LINK_ENTRY_NAME(os_counter_obj, counter_id)
#define COUNTERACTIONQUEUE(counter_id) RTFW_LINK_ENTRY_NAME(os_action_queue, counter_id)

#define GET_APP_CFG_BY_COUNTER_ID(counter_id) os_app_cfg_ref_table[os_counter_cfg_ref_table[counter_id]->owner_app_id]
#define GET_COUNTER_ADDR(id) (os_counter_cfg_ref_table[id]->obj_desc)
#define GET_COUNTER_MAX_VALUE(id) (os_counter_cfg_ref_table[id]->max_allowed_value)

#define DeclareCounter(section_const, section_var, counter_id, num)                                          \
	RTFW_TABLE_DECLARE_BY_ARRAY_MEMBER(section_var, os_action_queue, os_action_queue_t, counter_id, num);      \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_counter_obj, os_counter_inst_t, counter_id);                       \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_counter, const os_counter_cfg_t, counter_id)


/* Global Variable Declaration */
#define OS_START_SEC_CONST
#include "rtfw_memmap.h"
extern CONSTP2CONST(os_counter_cfg_t, AUTOMATIC, OS_CONST) os_counter_cfg_ref_table[];
extern CONST(OsObjIndexType, OS_CONST) os_counter_cfg_ref_table_size;
#define OS_STOP_SEC_CONST
#include "rtfw_memmap.h"

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
FUNC(void, OS_CODE) os_counter_init(const os_counter_cfg_t *const os_counter_cfg_table[], OsObjIndexType size);

FUNC(void, OS_CODE) os_counter_start(const os_counter_cfg_t *const os_counter_cfg_table[], OsObjIndexType size);

FUNC(os_status_t, OS_CODE) os_counter_increment(CounterType counter_id);

FUNC(void, OS_CODE) os_counter_check(void);

FUNC(void, OS_CODE) os_counter_hrt_handler(void);

FUNC(os_status_t, OS_CODE) os_counter_get_value(CounterType counter_id, TickRefType value);

FUNC(os_status_t, OS_CODE)
os_counter_get_elapsed_value(CounterType counter_id, TickRefType value, TickRefType elapsed_value_ref);

FUNC(void, OS_CODE) os_counter_add_abs_action(const os_action_cfg_t *action_cfg, TickType start);

FUNC(void, OS_CODE) os_counter_add_rel_action(const os_action_cfg_t *action_cfg, TickType increment);

FUNC(void, OS_CODE) os_counter_reload_action(const os_action_cfg_t *action_cfg, TickType cycle);

FUNC(void, OS_CODE) os_counter_delete_action(const os_action_cfg_t *action_cfg);

FUNC(TickType, OS_CODE) os_counter_get_user_value(const os_counter_cfg_t *counter_cfg);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

static inline_function TickType os_counter_diff(TickType curr, TickType old, TickType max)
{
	return (curr >= old) ? (curr - old) : (curr + (max - old) + 1u);
}

static inline_function TickType os_counter_add(TickType curr, TickType add, TickType max)
{
	return curr + add > max ? (curr + add - max - 1u) : (curr + add);
}


#ifdef __cplusplus
}
#endif

#endif /* __OS_COUNTER_H */
