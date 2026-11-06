
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
#ifndef RT_FRAMEWORK_INC_OS_SPINLOCK_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_SPINLOCK_INTERNAL_H
#ifdef __cplusplus
extern "C" {
#endif
/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_atomic.h"
#include "rtfw_lock_internal.h"
#include "rtfw_list.h"
#include "rtfw_context_internal.h"
#include "rtfw_spinlock_types.h"
/* Global Macro Definition */
#define OS_SPINLOCK_UNLOCK ((os_spin_t)0)
#define OS_SPINLOCK_LOCK ((os_spin_t)1)
#define OS_SPINLOCK_WRITE_LOCKED ((os_rw_spin_t)(1U << 31U))
#define OS_SPINLOCK_WRITE_UNLOCKED ((os_rw_spin_t)(OS_SPINLOCK_UNLOCK))
/* Get Spinlock ID  according SpinlockNode addr */
#define OS_SPINLOCK_ID_GET(_nodeAddr)                                                                                  \
	(((os_spin_inst_t *)((uint8 *)(_nodeAddr) - RTFW_OFFSETOF(os_spin_inst_t, lock)))->spin_id)
/* Global Enumeration Definition */
typedef enum {
	LOCK_ALL_INTERRUPTS, /*! Disable all interrupts. */
	LOCK_CAT2_INTERRUPTS, /*! Disable up to Cat. 2 interrupts. */
	LOCK_WITH_RES_SCHEDULER, /*! Disable task switching. */
	LOCK_NOTHING /*! Disable nothing, just get the lock. */
} os_spin_method_t;
typedef enum {
	SPINLOCKCHECK_DISABLED, /*! Spinlock checks are disabled. */
	SPINLOCKCHECK_ENABLED /*! Spinlock checks are enabled. */
} os_spin_check_t;
/* Global Type & Structure Definition */
typedef Atomic os_spin_t; /*! Data type for spinlocks. */
typedef Atomic os_rw_spin_t; /*! Data type for read-write spinlocks. */
/*! Spinlock Entry Description */
struct os_spin_inst_s {
	os_lock_cfg_t lock; /*! lock info for FIFO */
	os_spin_t spin; /*! SpinlockLock */
	const os_ctx_cfg_t *owner_context; /*! Entity occupies the lock */
	os_spin_id_t spin_id; /*! Spinlock identifier */
};

/*! Spinlock configuration by user */
struct os_spin_cfg_s {
	os_spin_inst_t *obj_desc; /*! Object instance description */
	os_spin_method_t method; /*! Lock method when a spinlock is taken */
#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP== 1)
	uint32 access_app_mask; /*! Applications that may access Spinlock */
#endif
	os_spin_check_t check; /*! Defines whether checks are enabled*/
	os_spin_id_t spin_id; /*! Spinlock identifier */
	os_spin_id_t order; /*! The order when spinlocks are nested*/
};

/* Global Variable Definition */
DeclareRtfwRefTable_H(spinlock) /*! Reference to OS spinlocks cfg table */
/* Global Function Declaration */
/*!  \brief         Init AUTOSAR OS Spinlock
 *  \details        None
 *
 *  \param[in]      None
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        ANY|TASK|KERNEL
 *
 *  \reentrant      FALSE
 *
 *  \synchronous    TRUE
 */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
FUNC(void, OS_CODE) os_spinlock_init(void);
/*! \brief         get_spinlock() actual Implementation
 *  \details       None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_get_raw_spinlock(const os_spin_id_t spin_id);
/*!  \brief         release_spinlock() actual Implementation
 *  \details        None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_release_raw_spinlock(const os_spin_id_t spin_id);
/*! \brief         get_spinlock() Implementation
 *  \details       None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *                  E_OS_ID:        [EXTENDED:] The spin_id is invalid
 *                  E_OS_ACCESS:    [EXTENDED:] The spinlock cannot be accessed
 *                  E_OS_INTERFERENCE_DEADLOCK: [EXTENDED:] A TASK tries to occupy the spinlock
 *                                               while the lock is already occupied by a TASK on
 *                                               the same core. This would cause a deadlock.
 *                  E_OS_NESTING_DEADLOCK:      [EXTENDED:] A TASK tries to occupy the spinlock
 *                                               while a TASK on the same core is holding a
 *                                               different spinlock in a way that may
 *                                               cause a deadlock.
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_get_spinlock(const os_spin_id_t spin_id);
/*!  \brief         release_spinlock() Implementation
 *  \details        None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *                  E_OS_ID:        [EXTENDED:] The spin_id is invalid
 *                  E_OS_ACCESS:    [EXTENDED:] The Spinlock cannot be accessed
 *                  E_OS_STATE:     [EXTENDED:] Spinlock is not occupied by the TASK
 *                  E_OS_NOFUNC:    [EXTENDED:] Attempt to release a spinlock while
 *                                  another spinlock (or resource) has to be released before.
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_release_spinlock(const os_spin_id_t spin_id);
/*!  \brief         try_to_get_spinlock() Implementation
 *  \details        None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     success         Returns if the lock has been occupied or not
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *                  E_OS_ID:        [EXTENDED:] The spin_id is invalid
 *                  E_OS_ACCESS:    [EXTENDED:] The Spinlock cannot be accessed
 *                  E_OS_INTERFERENCE_DEADLOCK: [EXTENDED:] A TASK tries to occupy the spinlock
						 while the lock is already occupied by a TASK
 *                                               on the same core. This would cause a deadlock.
 *                  E_OS_NESTING_DEADLOCK:      [EXTENDED:] A TASK tries to occupy the spinlock
 *                                               while a TASK on the same core is holding a
 *                                               different spinlock in a way that
 *                                               may cause a deadlock.
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_try_to_get_spinlock(const os_spin_id_t spin_id, try_get_spin_t *success);
/*! \brief         get_spinlock_read() Implementation
 *  \details       None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *                  E_OS_ID:        [EXTENDED:] The spin_id is invalid
 *                  E_OS_ACCESS:    [EXTENDED:] The spinlock cannot be accessed
 *                  E_OS_INTERFERENCE_DEADLOCK: [EXTENDED:] A TASK tries to occupy
 *                                  the read-spinlock while the lock is already occupied by the
 *                                  same TASK on the same core. This would cause a deadlock.
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_get_spinlock_read(const os_spin_id_t spin_id);
/*! \brief         get_spinlock_write() Implementation
 *  \details       None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *                  E_OS_ID:        [EXTENDED:] The spin_id is invalid
 *                  E_OS_ACCESS:    [EXTENDED:] The spinlock cannot be accessed
 *                  E_OS_INTERFERENCE_DEADLOCK: [EXTENDED:] A TASK tries to occupy the
 *                                  write-spinlock while the lock is already occupied by any
 *                                  TASK including itself on the same core.
 *                                  This would cause a deadlock.
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_get_spinlock_write(const os_spin_id_t spin_id);
/*!  \brief         release_spinlock_read() Implementation
 *  \details        None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *                  E_OS_ID:        [EXTENDED:] The spin_id is invalid
 *                  E_OS_ACCESS:    [EXTENDED:] The Spinlock cannot be accessed
 *                  E_OS_STATE:     [EXTENDED:] Spinlock is not occupied by the TASK
 *                  E_OS_NOFUNC:    [EXTENDED:] Attempt to release a spinlock while another
 *                                  spinlock (or resource) hasto be released before.
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_release_spinlock_read(const os_spin_id_t spin_id);
/*!  \brief         release_spinlock_read() Implementation
 *  \details        None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     None
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *                  E_OS_ID:        [EXTENDED:] The spin_id is invalid
 *                  E_OS_ACCESS:    [EXTENDED:] The Spinlock cannot be accessed
 *                  E_OS_STATE:     [EXTENDED:] Spinlock is not occupied by the TASK
 *                  E_OS_NOFUNC:    [EXTENDED:] Attempt to release a spinlock while another
 *                                  spinlock (or resource) has to be released before.
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_release_spinlock_write(const os_spin_id_t spin_id);
/*!  \brief         try_to_get_spinlock_read() Implementation
 *  \details        None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     success         Returns if the lock has been occupied or not
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *                  E_OS_ID:        [EXTENDED:] The spin_id is invalid
 *                  E_OS_ACCESS:    [EXTENDED:] The Spinlock cannot be accessed
 *                  E_OS_INTERFERENCE_DEADLOCK: [EXTENDED:] A TASK tries to occupy the spinlock
 *                                   while the lock is already occupied by a TASK on the same
 *                                   core. This would cause a deadlock.
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_try_to_get_spinlock_read(const os_spin_id_t spin_id, try_get_spin_t *success);
/*!  \brief         try_to_get_spinlock_write() Implementation
 *  \details        None
 *
 *  \param[in]      spin_id      The value refers to the spinlock instance that shall be locked.
 *  \param[out]     success         Returns if the lock has been occupied or not
 *
 *  \return         Status of execution result
 *                  E_OK:           [STANDARD:] No error
 *                  E_OS_ID:        [EXTENDED:] The spin_id is invalid
 *                  E_OS_ACCESS:    [EXTENDED:] The Spinlock cannot be accessed
 *                  E_OS_INTERFERENCE_DEADLOCK: [EXTENDED:] A TASK tries to occupy the spinlock
 *                                   while the lock is already occupied by a TASK on the same
 *                                   core. This would cause a deadlock.
 *
 *  \context        TASK|ISR2
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    TRUE
 */
FUNC(os_status_t, OS_CODE) os_try_to_get_spinlock_write(const os_spin_id_t spin_id, try_get_spin_t *success);
/* Global Function Implementation */
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
#ifdef __cplusplus
}
#endif
#endif /* RT_FRAMEWORK_INC_OS_SPINLOCK_INTERNAL_H */
