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

#ifndef RT_FRAMEWORK_INC_OS_APPLICATION_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_APPLICATION_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include <nuttx/irq.h>
#include "rtfw_types.h"
#include "rtfw_task_types.h"
#include "rtfw_isr_types.h"
#include "rtfw_application_types.h"
#include "rtfw_counter_types.h"
#include "Compiler.h"
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
#include "rtfw_schedtable.h"
#endif
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
#include "rtfw_hook_types.h"
#endif

/* Global Macro Definition */
#define OS_APPLICATION_GET_APPID(ApplicationRef) ((ApplicationRef)->app_id)
#define OS_APPLICATION_GET_APP_OWNERCOREID(ApplicationRef) ((ApplicationRef)->core_id)
#define INVALID_OSAPPLICATION (os_app_cfg_ref_table_size)

struct os_app_inst {
	ApplicationStateType state; /*! Application State. */
};

/*! Constant application configuration information. */
struct os_app_cfg_s {
	os_app_inst_t *instance; /*! Application Instance Description */
	const char *app_name; /*! App name */

	const os_task_cfg_t *const *task_refs; /*! Tasks. */
	const OsObjIndexType *task_count; /*! Number of tasks. */

	const os_isr_cfg_t *const *isr_refs; /*! ISRs of category 2. */
	const OsObjIndexType *isr_count; /*! Number of ISRs of category 2. */
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	const os_schedtable_cfg_t *const *scht_refs; /*! List of Schedule Tables of this core. */
	const OsObjIndexType *scht_count; /*! Number of Schedule Tables. */
#endif
#if defined(CONFIG_RT_FRAMEWORK_COUNTER) && (CONFIG_RT_FRAMEWORK_COUNTER == 1)
	const os_counter_cfg_t *const *counter_refs;
	const OsObjIndexType *counter_count;
#endif
#if defined(CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION) && (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION == 1)
	const struct mpu_access_right_s *mpu_access_right;
#endif
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
	const os_hook_cfg_t *startup_hook;
	const os_hook_cfg_t *shutdown_hook;
	const os_hook_cfg_t *error_hook;
	const os_hook_cfg_t *const *hook_refs;
	const OsObjIndexType *hook_count;
#endif
	CoreIdType core_id; /*! The core to which this application belongs. */
	ApplicationIDType app_id; /*! Application ID. */
	boolean is_privileged; /*! Tells whether the application is privileged. */
};

/* Global Variable Definition */
DeclareRtfwRefTable_H(app)

/* Global Function Declaration */

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
extern FUNC(os_status_t, OS_CODE) os_get_app_state(ApplicationType app_id, ApplicationStateRefType state);
extern FUNC(ApplicationType, OS_CODE) os_get_app_id(void);
extern FUNC(ApplicationType, OS_CODE) os_get_current_app_id(void);
extern FUNC(os_status_t, OS_CODE) os_allow_access(void);
extern FUNC(ApplicationType, OS_CODE) os_check_object_ownership(ObjectTypeType object_type, Os_ObjectIdType object_id);
extern FUNC(void, OS_CODE) os_app_init(const os_app_cfg_t *const os_app_cfg_table[], OsObjIndexType size);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
static inline os_status_t os_app_check_access(ApplicationIDType app_id, uint32 access_mask)
{
	if (app_id > (ApplicationIDType)31) {
		return OS_STATUS_ACCESS_RIGHT;
	}
	return (((access_mask & (RTFW_APPID2MASK(app_id))) > 0U) ? OS_STATUS_OK : OS_STATUS_ACCESS_RIGHT);
}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK */

static inline os_status_t os_app_check_is_accessible(const os_app_cfg_t *app_cfg)
{
	ApplicationStateType app_state;
	os_status_t status;

	status = os_get_app_state(app_cfg->app_id, &app_state);
	if ((status == OS_STATUS_OK) && (app_state == APPLICATION_ACCESSIBLE)) {
		return OS_STATUS_OK;
	}
	return OS_STATUS_NOT_OK;
}

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_APPLICATION_INTERNAL_H */
