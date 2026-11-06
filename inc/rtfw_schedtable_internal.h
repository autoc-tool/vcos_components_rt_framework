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

#ifndef RT_FRAMEWORK_INC_OS_SCHED_TABLE_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_SCHED_TABLE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif
/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_schedtable_types.h"
#include "rtfw_counter_types.h"

/* Global Macro Declaration */
#define GET_APP_CFG_BY_SCHEDULETABLE_ID(schedtable_id)                                                               \
	os_app_cfg_ref_table[os_schedtable_cfg_ref_table[schedtable_id]->app_owner_id]

typedef uint8 os_schedtable_sync_strategy;
#define NONE ((os_schedtable_sync_strategy)0x00U)      /*! Schedule table without synchronization */
#define EXPLICIT ((os_schedtable_sync_strategy)0x01U)  /*! Schedule table with explicit synchronization */
#define IMPLICIT ((os_schedtable_sync_strategy)0x02U)  /*! Schedule table with implicit synchronization */

typedef uint8 os_schedtable_autostart_type;
#define SCHTBL_AUTOSTART_ABSOLUTE                                                                                      \
	((os_schedtable_autostart_type)0x00U)              /*! Automatic absolute start, calls StartScheduleTableAbs() */
#define SCHTBL_AUTOSTART_RELATIVE                                                                                      \
	((os_schedtable_autostart_type)0x01U)              /*! Automatic relative start, calls StartScheduleTableRel() */
#define SCHTBL_AUTOSTART_SYNCHRONE                                                                                     \
	((os_schedtable_autostart_type)0x02U)              /*! Automatic synchronous start, calls StartScheduleTableSyncon() */

typedef uint8 os_schedtable_repeating_type;
#define SCHTBL_SINGLE_SHOT ((os_schedtable_repeating_type)0x00U)  /*! Single execution */
#define SCHTBL_REPEATING ((os_schedtable_repeating_type)0x01U)    /*! Periodic execution */

typedef uint8 os_schedtable_sync_status;
#define SCHTBL_SYNCSTATUS_NOT_PROVIDED ((os_schedtable_sync_status)0x00U)  /*! No synchronization counter value provided */
#define SCHTBL_SYNCSTATUS_PROVIDED                                                                                     \
	((os_schedtable_sync_status)0x01U)                 /*! Synchronization counter value provided, counter starts working */

/* Global Type & Structure Definition */
struct os_schedtable_adjust_expiry_point {
	TickType max_shorten;                              /*! Maximum advancement for single adjustment */
	TickType max_lengthen;                             /*! Maximum delay for single adjustment */
};

struct os_schedtable_expiry_point {
	const TaskType *task_list;                         /*! List of tasks activated at expiry point */
	const os_schedtable_set_event_t *event_list;       /*! List of events activated at expiry point */
	os_schedtable_adjust_expiry_point_t adjust_expiry_point; /*! Maximum single adjustment value for schedule table */
	TickType offset;                                   /*! offset value of expiry point */
	uint8_t task_list_cnt;                             /*! Number of tasks activated at expiry point */
	uint8_t event_list_cnt;                            /*! Number of events activated at expiry point */
};

struct os_schedtable_sync {
	TickType sync_time_stamp;                          /*! Synchronization counter */
	TickType counter_time_stamp;                       /*! Driving counter */
	TickType deviation;                                /*! Overall synchronization deviation, note that the highest bit is the sign bit */
	TickType one_step_adjust;                          /*! Single synchronization adjustment bias, note that the highest bit is the sign bit */
	os_schedtable_sync_status sync_state;              /*! Whether synchronization counter is provided */
};

struct os_schedtable_autostart {
	AppModeType autostart_mode;                        /*! Auto-start mode */
	os_schedtable_autostart_type type;                 /*! Auto-start type */
	TickType offset;                                   /*! Start offset value */
};

struct os_schedtable_inst {
	struct hrtimer_s hrtimer;                          /*! Next expiry point for schedule table */
	os_schedtable_sync_t sync;                         /*! Schedule table synchronization information */
	os_schedtable_inst_t *next;                        /*! next schedule table to start after current schedule table ends, NULL if none, see NextScheduleTable */
	os_schedtable_inst_t *list_schedtable_node;        /*! Used during initialization to form a singly linked list of all schedule tables on the same Counter */
	uint32 current_expiry_index;                       /*! Current expiry point index value for schedule table */
	enum ScheduleTableStatus  state;                   /*! Schedule table status */
	uint32 app_status;                                 /*! Used to determine if app restart occurred, each bit identifies an App */
	uint32 app_status_mask;                            /*! Used to determine whether to clear the flag at the end of schedule table cycle */
	ScheduleTableType schedtable_id;                   /*! Schedule table ID value */
};

struct os_schedtable_cfg {
	os_schedtable_inst_t *instance;                    /*! Application object instance description */
	const char *schedtable_name;                       /*! Schedule table name, describes schedule table functionality */
	const os_schedtable_expiry_point_t *expire_point_list;   /* Schedule table expiry point attributes */
	os_action_cfg_t action_config;                     /*! action Config, Use for action Priority queue*/
	const os_schedtable_autostart_t autostart;         /* Schedule table auto-start properties */
	TickType duration;                                 /* Schedule table duration */
	uint32 expiry_point_cnt;                           /* Schedule table expiry point attributes */
	TickType precision;                                /* Precision for explicit synchronization */
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
	uint32 access_app_mask;                            /* Application access permissions */
#endif
	ScheduleTableType schedtable_id;                   /*! Schedule table identifier */
	os_schedtable_repeating_type repeating;            /* Schedule table operating mode */
	os_schedtable_sync_strategy sync_strategy;         /* Schedule table synchronization properties */
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
	ApplicationIDType app_owner_id;                    /* Application ID that the schedule table belongs to */
#endif
};

struct os_schedtable_set_event {
	TaskType task;                                     /*! The task, who's event has to be set. */
	os_event_mask_t mask;                              /*! The events, to be set. */
};

/* Global Macro Definition */

DeclareRtfwRefTable_H(schedtable)

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
/* Global Function Declaration */
extern FUNC(void, OS_CODE) os_schedule_table_init(const os_schedtable_cfg_t *const os_schedtable_cfg_table[], OsObjIndexType size);
extern FUNC(void, OS_CODE) os_schedule_table_kill(const os_schedtable_cfg_t *const schedtable_ref);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_SCHED_TABLE_INTERNAL_H */
