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

#ifndef AUTOSAROS_INC_CORE_H
#define AUTOSAROS_INC_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_core_internal.h"

/* Get Core Configuration Information Handler according ID */
#define OS_CORE_CFG_GET(_id) (os_core_cfg_ref_table[_id])

/*! Core declare*/
#define DeclareCore(section_const, section_var, core_id)                         \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_core_inst_t, core_id);         \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_core, const os_core_cfg_t, core_id)

#define OSCORECFGVAR(core_id) RTFW_LINK_ENTRY_NAME(os_core, core_id)

#define OSCOREOBJVAR(core_id) RTFW_LINK_ENTRY_NAME(os_obj, core_id)

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

/*!  \brief         启动时第一次核间同步
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */

#if (CONFIG_RT_FRAMEWORK_NCORES > 1)
FUNC(void, OS_CODE) os_core_sync_phase_one(void);
#else
#define os_core_sync_phase_one()
#endif

/*!  \brief         启动时第二次核间同步
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
#if (CONFIG_RT_FRAMEWORK_NCORES > 1)
FUNC(void, OS_CODE) os_core_sync_phase_two(void);
#else
#define os_core_sync_phase_two()
#endif

/*!  \brief         下电时核间同步
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
#if (CONFIG_RT_FRAMEWORK_NCORES > 1)
FUNC(void, OS_CODE) os_core_shutdown_sync(void);
#else
#define os_core_shutdown_sync()
#endif

/*!  \brief         After this service the OS on all AUTOSAR cores is shut down. Allowed at TASK level and ISR level
 *                  and also internally by the OS. The function will never return. The function will force other cores
 *                  into a shutdown.
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        ANY|TASK|ISR1|ISR2|KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) os_shutdown_all_cores(StatusType error_status);

/*!  \brief         Shut down the specific core.
 *  \details        None
 *
 *  \param[in]      error_status: The error status
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        ANY|TASK|ISR1|ISR2|KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) os_shutdown_core(StatusType error_status);

/*!  \brief         Shut down the os.
 *  \details        None
 *
 *  \param[in]      error_status: The error status
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        ANY|TASK|ISR1|ISR2|KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) os_shutdown_os(StatusType error_status);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* AUTOSAROS_INC_CORE_H */
