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

#ifndef RT_FRAMEWORK_INC_OS_INTERRUPT_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_INTERRUPT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_interrupt_types.h"

/* Global Macro Definition */
#define GET_APP_CFG_BY_ISR_ID(isr_id) os_isr_cfg_ref_table[isr_id]->context.app_owner_cfg

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

/* 函数说明详见 DisableAllInterrupts */
extern FUNC(void, OS_CODE) os_disable_all_interrupts(void);

/* 函数说明详见 EnableAllInterrupts */
extern FUNC(void, OS_CODE) os_enable_all_interrupts(void);

/* 函数说明详见 SuspendAllInterrupts */
extern FUNC(void, OS_CODE) os_suspend_all_interrupts(void);

/* 函数说明详见 ResumeAllInterrupts */
extern FUNC(void, OS_CODE) os_resume_all_interrupts(void);

/* 函数说明详见 SuspendOSInterrupts */
extern FUNC(void, OS_CODE) os_suspend_os_interrupts(void);

/* 函数说明详见 ResumeOSInterrupts */
extern FUNC(void, OS_CODE) os_resume_os_interrupts(void);

/* 函数说明详见 EnableInterruptSource */
extern FUNC(os_status_t, OS_CODE) os_enable_interrupt_source(isr_id_t isr_id, boolean clear_pending);

/* 函数说明详见 DisableInterruptSource */
extern FUNC(os_status_t, OS_CODE) os_disable_interrupt_source(isr_id_t isr_id);

/* 函数说明详见 ClearPendingInterrupt */
extern FUNC(os_status_t, OS_CODE) os_clear_pending_interrupt(isr_id_t isr_id);

/*!  \brief         检查是否已经关闭中断
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         TRUE:  中断已经关闭，即已经调用 DisableAllInterrupts/SuspendAllInterrupts/SuspendOSInterrupts 之一
 *                  FALSE: 中断尚未关闭
 *
 *  \context        ANY|TASK|ISR1|ISR2|KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
extern FUNC(boolean, OS_CODE) os_interrupt_is_disabled(void);

/*!  \brief         关闭内核使用的中断
 *  \details        None
 *
 *  \param[in]      int_state: Reference to Interrupt State
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *
 *  \context        KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) os_int_suspend_kernel(irqlevel_t *const int_state);

/*!  \brief         恢复内核使用的中断
 *  \details        None
 *
 *  \param[in]      IntStateRef： Reference to Interrupt State
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *
 *  \context        KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) os_int_resume_kernel(const irqlevel_t *const int_state);

/*!  \brief         Disable Global Interrupt State  Controlled by CPU.
 *  \details        Nesting is not supported.
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         void
 *
 *  \context        ANY|TASK|KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) os_int_disable_global(void);

/*!  \brief         Enable Global Interrupt State Controlled by CPU.
 *  \details        Nesting is not supported.
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         void
 *
 *  \context        ANY|TASK|KERNEL
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(void, OS_CODE) os_int_enable_global(void);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

/* Global Function Implementation */

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_INTERRUPT_INTERNAL_H */
