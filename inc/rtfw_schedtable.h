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

#ifndef RT_FRAMEWORK_INC_OS_SCHED_TABLE_H
#define RT_FRAMEWORK_INC_OS_SCHED_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_counter_internal.h"
#include "rtfw_schedtable_internal.h"
#include "rtfw_counter_types.h"
#include "rtfw_core_types.h"

/*! SchedTable declare*/
#define DeclareExpTaskList(section, exp_task_list)    \
	RTFW_LINK_ENTRY_DECLARE(section, os_schdtbl_exp_point, const TaskType, exp_task_list)
#define DeclareExpEventList(section, exp_event_list)    \
	RTFW_LINK_ENTRY_DECLARE(section, os_schdtbl_exp_point, const os_schedtable_set_event_t, exp_event_list)
#define DeclareExpiryPoint(section, expire_point_list)    \
	RTFW_LINK_ENTRY_DECLARE(section, os_schdtbl_exp_point, const os_schedtable_expiry_point_t, expire_point_list)
#define DeclareSchedTable(section_const, section_var, schedtable_id)    \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_schedtable_inst_t, schedtable_id);    \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_schdtbl, const os_schedtable_cfg_t, schedtable_id)

#define SCHEDTABLECFGVAR(schedtable_id) RTFW_LINK_ENTRY_NAME(os_schdtbl, schedtable_id)
#define SCHEDTABLEOBJVAR(schedtable_id) RTFW_LINK_ENTRY_NAME(os_obj, schedtable_id)

#define EXPIREPOINTCFGVAR(ExpirePointName) RTFW_LINK_ENTRY_NAME(os_schdtbl_exp_point, ExpirePointName)
#define EXPIRE_POINT_GET(cfg_ptr, index) (&(cfg_ptr)->expire_point_list[(index)])
#define OS_GET_SCHEDTABLE_CFG_BY_ACTION_ADDR(action)    \
	((os_schedtable_cfg_t *)((char *)(action) - offsetof(os_schedtable_cfg_t, action_config)))

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

/* Global Function Declaration */

/*! \brief          Start the schedule table with a relative offset from the current time.
 *  \details        None
 *
 *  \param[in]      schedtable_id The schedule table to be started
 *  \param[in]      offset The counter Tick value before schedule table starts
 *
 *  \return         E_OK                 No error
 *                  E_OS_DISABLEDINT     Interrupts are disabled or suspended
 *                  E_OS_ID              Invalid schedule table ID
 *                  E_OS_ACCESS          No permission
 *                  E_OS_VALUE           Parameter offset is 0 or schedule table offset exceeds maximum allowed value
 *                  E_OS_STATE           Schedule table status is incorrect
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_start_schedule_table_rel(ScheduleTableType schedtable_id, TickType offset);

/*! \brief          Start the schedule table at absolute time.
 *  \details        None
 *
 *  \param[in]      schedtable_id The schedule table to be started
 *  \param[in]      start The absolute start time of the schedule table
 *
 *  \return         E_OK                 No error
 *                  E_OS_DISABLEDINT     Interrupts are disabled or suspended
 *                  E_OS_ID              Invalid schedule table ID
 *                  E_OS_ACCESS          No permission
 *                  E_OS_VALUE           Start exceeds maximum allowed value
 *                  E_OS_STATE           Schedule table status is incorrect
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_start_schedule_table_abs(ScheduleTableType schedtable_id, TickType start);

/*! \brief          Stop the schedule table
 *  \details        None
 *
 *  \param[in]      schedtable_id      The schedule table to be stopped
 *
 *  \return         E_OK                 No error
 *                  E_OS_DISABLEDINT     Interrupts are disabled or suspended
 *                  E_OS_ID              Invalid schedule table ID
 *                  E_OS_ACCESS          No permission
 *                  E_OS_STATE           Schedule table status is incorrect
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_stop_schedule_table(ScheduleTableType schedtable_id);

/*! \brief          Set the bit corresponding to AppId in the app_status internal variable of the schedule table
 *  \details        None
 *
 *  \param[in]      schedtable_id        Index value of the schedule table to be set
 *  \param[in]      app_id               Index value of the restarted APP to be recorded
 *
 *  \return         E_OK                 No error
 *                  E_OS_CALLEVEL        Call level error
 *                  E_OS_DISABLEDINT     Interrupts are disabled or suspended
 *                  E_OS_ID              Invalid schedule table ID
 *                  E_OS_ACCESS          No permission
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
FUNC(os_status_t, OS_CODE) os_set_schedule_table_app_id(ScheduleTableType schedtable_id, ApplicationIDType app_id);

/*! \brief          Set the bit corresponding to AppId in the app_status_mask internal variable of the schedule table
 *  \details        None
 *
 *  \param[in]      schedtable_id        Index value of the schedule table to be set
 *  \param[in]      app_id               Index value of the restored APP to be recorded
 *
 *  \return         E_OK                 No error
 *                  E_OS_CALLEVEL        Call level error
 *                  E_OS_DISABLEDINT     Interrupts are disabled or suspended
 *                  E_OS_ID              Invalid schedule table ID
 *                  E_OS_ACCESS          No permission
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_clear_schedule_table_app_id(ScheduleTableType schedtable_id, ApplicationIDType app_id);
#endif

/*! \brief          Gets the running state of a schedule table.
 *  \details        None
 *
 *  \param[in]      schedtable_id      Target schedule table
 *  \param[out]     ScheduleStatus       Running state of the target schedule table
 *
 *  \return         E_OK                 No error
 *                  E_OS_DISABLEDINT     Interrupts are disabled or suspended
 *                  E_OS_ID              Invalid schedule table ID
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_get_schedule_table_status(ScheduleTableType schedtable_id, ScheduleTableStatusRefType ScheduleStatus);

/*! \brief          Stop the current schedule table CurrSchedTableID, start the next schedule table NextSchedTableID
 *  \details        None
 *
 *  \param[in]      curr_schedtable_id     Current schedule table ID
 *  \param[in]      next_schedtable_id     Next schedule table ID
 *
 *  \return         E_OK                      Execution successful
 *                  E_OS_NOFUNC               Error, current schedule table status incorrect
 *                  E_OS_STATE                Error, next schedule table status incorrect
 *                  E_OS_DISABLEDINT          Error, interrupts are disabled or suspended
 *                  E_OS_ID                   Error, invalid schedule table ID
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_next_schedule_table(ScheduleTableType schedule_table_id_from, ScheduleTableType schedule_table_id_to);

/*! \brief          Current schedule table CurrSchedTableID is in stopped state, and the next schedule table NextSchedTableID follows schedule table CurrSchedTableID to start
 *  \details        None
 *
 *  \param[in]      curr_schedtable_id     Current schedule table ID
 *  \param[in]      next_schedtable_id     Next schedule table ID
 *
 *  \return         E_OK                      Execution successful
 *                  E_OS_NOFUNC               Error, current schedule table status incorrect
 *                  E_OS_STATE                Error, next schedule table status incorrect
 *                  E_OS_DISABLEDINT          Error, interrupts are disabled or suspended
 *                  E_OS_ID                   Error, invalid schedule table ID
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_next_schedule_table_from_stop(ScheduleTableType schedule_table_id_from, ScheduleTableType schedule_table_id_to);

/*! \brief          Start schedule table synchronization
 *  \details        None
 *
 *  \param[in]      schedtable_id    Target schedule table ID
 *
 *  \return         E_OK                      Execution successful
 *                  E_OS_DISABLEDINT          Error, interrupts are disabled or suspended
 *                  E_OS_ID                   Error, invalid schedule table ID
 *                  E_OS_STATE                Error, next schedule table status incorrect
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_start_schedule_table_synchron(ScheduleTableType schedtable_id);

/*! \brief          Synchronize schedule table and correct time
 *  \details        None
 *
 *  \param[in]      schedtable_id    Target schedule table ID
 *  \param[in]      tick_value       Synchronization correction value
 *
 *  \return         E_OK                      Execution successful
 *                  E_OS_DISABLEDINT          Error, interrupts are disabled or suspended
 *                  E_OS_ID                   Error, invalid schedule table ID
 *                  E_OS_VALUE                Error, synchronization correction value exceeds schedule table cycle
 *                  E_OS_STATE                Error, next schedule table status incorrect
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_sync_schedule_table(ScheduleTableType schedtable_id, TickType sync_time_now);

/*! \brief          Set time offset to schedule table
 *  \details        None
 *
 *  \param[in]      schedtable_id             Target schedule table ID
 *  \param[in]      deviate_value             Synchronization correction value
 *
 *  \return         E_OK                      Execution successful
 *                  E_OS_DISABLEDINT          Error, interrupts are disabled or suspended
 *                  E_OS_ID                   Error, invalid schedule table ID
 *                  E_OS_VALUE                Error, synchronization correction value exceeds schedule table cycle
 *                  E_OS_STATE                Error, next schedule table status incorrect
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_deviate_schedule_table(ScheduleTableType schedtable_id, DeviateType deviate_value);

/*! \brief          Set schedule table to running state
 *  \details        None
 *
 *  \param[in]      schedtable_id    Target schedule table ID
 *
 *  \return         E_OK                      Execution successful
 *                  E_OS_DISABLEDINT          Error, interrupts are disabled or suspended
 *                  E_OS_ID                   Error, invalid schedule table ID
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_set_schedule_table_async(ScheduleTableType schedtable_id);

/*! \brief          Schedule table Action callback function
 *  \details        None
 *
 *  \param[in]      Action    Action to execute upon expiry
 *
 *  \return         void
 *
 *
 *  \context        ISR2|TASK
 *
 *  \reentrant      FALSE
 *
 *  \synchronous    TRUE
 */
void schedule_table_action_callback(struct hrtimer_s *hrtimer);

/* Global Function Implementation */
/*! \brief          Get the core ID that the schedule table belongs to
 *  \details        None
 *
 *  \param[in]      schedtable_id   Schedule table ID
 *
 *  \return         void
 *
 *
 *  \context        ISR2|TASK
 *
 *  \reentrant      FALSE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) get_sched_table_core_id(ScheduleTableType schedtable_id, CoreIdType *core_id_ref);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_SCHED_TABLE_H */
