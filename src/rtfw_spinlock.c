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

#include <nuttx/config.h>
#include <string.h>
#include "rtfw_spinlock.h"
#include "rtfw_core.h"
#include "rtfw_application.h"
#include "rtfw_error.h"

static inline void __spin_init(os_spin_t *lock)
{
	(void)Os_AtomicSet(lock, OS_SPINLOCK_UNLOCK);
}

static inline boolean __spin_trylock(os_spin_t *lock)
{
	return (boolean)Os_AtomicCmpSwap(lock, OS_SPINLOCK_UNLOCK, OS_SPINLOCK_LOCK);
}

static inline void __spin_lock(os_spin_t *lock)
{
	while (__spin_trylock(lock) == FALSE) {
	};
}

static inline void __spin_unlock(os_spin_t *lock)
{
	(void)Os_AtomicSet(lock, OS_SPINLOCK_UNLOCK);
}

static inline boolean __spin_rd_trylock(os_rw_spin_t *lock)
{
	boolean ret = FALSE;
	volatile os_rw_spin_t cur_lock;
	os_rw_spin_t new_lock;

	cur_lock = *lock;
	if ((cur_lock & OS_SPINLOCK_WRITE_LOCKED) == OS_SPINLOCK_WRITE_UNLOCKED) {
		new_lock = cur_lock + 1;
		if (Os_AtomicCmpSwap(lock, cur_lock, new_lock)) {
			ret = TRUE;
		}
	}
	return ret;
}

static inline boolean __spin_wr_trylock(os_rw_spin_t *lock)
{
	return (boolean)Os_AtomicCmpSwap(lock, OS_SPINLOCK_WRITE_UNLOCKED, OS_SPINLOCK_WRITE_LOCKED);
}

static inline void __spin_rd_lock(os_rw_spin_t *lock)
{
	while (__spin_rd_trylock(lock) == FALSE) {
	};
}

static inline void __spin_wr_lock(os_rw_spin_t *lock)
{
	while (__spin_wr_trylock(lock) == FALSE) {
	};
}

static inline void __spin_rd_unlock(os_rw_spin_t *lock)
{
	Os_AtomicDec(lock);
}

static inline void __spin_wr_unlock(os_rw_spin_t *lock)
{
	Os_AtomicSet(lock, 0U);
}

static inline os_status_t spin_order_check(const os_list_t *lock_list, const os_spin_id_t order)
{
	os_status_t ret = OS_STATUS_NOT_OK;
	const os_lock_cfg_t *lock_cfg;
	os_spin_id_t last_order;
	const os_list_node_t *last_node;

	if (LIKELY(OS_LIST_IS_EMPTY(lock_list))) {
		ret = OS_STATUS_OK;
	} else {
		last_node = OS_LIST_LAST(lock_list);
		do {
			lock_cfg = OS_LOCK_CFG_PTR_GET(last_node);
			if (LIKELY(lock_cfg->type == OS_LOCK_TYPE_SPINLOCK)) {
				last_order = os_spinlock_cfg_ref_table[(OS_SPINLOCK_ID_GET(lock_cfg))]->order;
				if (last_order < order) {
					ret = OS_STATUS_OK;
				} else {
					ret = OS_STATUS_NOT_OK;
				}
				break;
			} else {
				ret = OS_STATUS_OK;
			}
			last_node = last_node->prev;
		} while (last_node != NULL);
	}
	return ret;
}

static inline void spin_suspend_lock_method(const os_spin_method_t method)
{
	boolean check_ret;

	switch (method) {
	case LOCK_NOTHING:
		/* do nothing */
		break;
	case LOCK_ALL_INTERRUPTS:
		os_suspend_all_interrupts();
		break;
	case LOCK_CAT2_INTERRUPTS:
		os_suspend_os_interrupts();
		break;
	case LOCK_WITH_RES_SCHEDULER:
		check_ret = os_context_check_curcontext_id(os_context_get_contexthdl(), OS_TASK_CONTEXT_ID);
		if (check_ret == TRUE) {
			sched_lock(); /* lock scheduler in task context */
		}
		/* do nothing in isr context */
		break;
	default:
		break;
	}
}

static inline void spin_remove_lock_method(const os_spin_method_t method)
{
	boolean check_ret;

	switch (method) {
	case LOCK_NOTHING:
		/* do nothing */
		break;
	case LOCK_ALL_INTERRUPTS:
		os_resume_all_interrupts();
		break;
	case LOCK_CAT2_INTERRUPTS:
		os_resume_os_interrupts();
		break;
	case LOCK_WITH_RES_SCHEDULER:
		check_ret = os_context_check_curcontext_id(os_context_get_contexthdl(), OS_TASK_CONTEXT_ID);
		if (check_ret == TRUE) {
			sched_unlock(); /* unlock scheduler in task context */
		}
		/* do nothing in isr context */
		break;
	default:
		break;
	}
}

/*
 * if spinlock is occupied by task with lower priority, function returns error code
 * if cpu switches to a higher priority task or interrupt after the check, the entity must
 * release the lock after switching back the origin task.
 */
static inline os_status_t spin_local_core_check(const os_spin_inst_t *lock_inst)
{
	os_status_t status = OS_STATUS_NOT_OK;
	const os_ctx_cfg_t *lock_owner = lock_inst->owner_context;

	if (LIKELY(lock_owner == NULL_PTR)) {
		status = OS_STATUS_OK;
#if (CONFIG_RT_FRAMEWORK_NCORES > 1)
	} else if (OS_CONTEXT_GET_OWNER_COREID(lock_owner) != os_get_core_id()) {
		status = OS_STATUS_OK;
	} else {
		status = OS_STATUS_NOT_OK;
	}
#else
	} else {
		status = OS_STATUS_NOT_OK;
	}
#endif /* CONFIG_RT_FRAMEWORK_NCORES > 1 */
	return status;
}

static inline os_status_t spin_entity_opccupy_check(const os_ctx_cfg_t *cur_context,
						      const os_ctx_cfg_t *owner_context)
{
	os_status_t status = OS_STATUS_NOT_OK;

	if (LIKELY(cur_context == owner_context)) {
		status = OS_STATUS_OK;
	}
	return status;
}

static inline os_status_t spin_fifo_check(const os_ctx_cfg_t *cur_context, const os_list_node_t *lock_node)
{
	os_status_t status = OS_STATUS_NOT_OK;

	if (LIKELY(OS_LIST_LAST(OS_CONTEXT_GET_LOCKLIST_REF(cur_context)) == lock_node)) {
		status = OS_STATUS_OK;
	} else {
		status = OS_STATUS_NOT_OK;
	}
	return status;
}

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) os_spinlock_init(void)
{
	const os_spinlock_cfg_t *spin_cfg;
	os_spin_inst_t *lock_inst;
	OsObjIndexType index;

	if (UNLIKELY(os_spinlock_cfg_ref_table_size == (OsObjIndexType)0U)) {
		return;
	}

	for (index = 0U; index < os_spinlock_cfg_ref_table_size; index++) {
		spin_cfg = os_spinlock_cfg_ref_table[index];
		if ((spin_cfg == NULL) || (spin_cfg->spin_id >= os_spinlock_cfg_ref_table_size) ||
		    (os_spinlock_cfg_ref_table[spin_cfg->spin_id] == NULL)) {
			continue;
		}

		lock_inst = os_spinlock_cfg_ref_table[spin_cfg->spin_id]->obj_desc;
		if (lock_inst == NULL) {
			continue;
		} else {
			lock_inst = spin_cfg->obj_desc;
			lock_inst->spin_id = spin_cfg->spin_id;
			lock_inst->lock.type = OS_LOCK_TYPE_SPINLOCK;
			lock_inst->owner_context = NULL_PTR;
			__spin_init(&(lock_inst->spin));
		}
	}
}

FUNC(os_status_t, OS_CODE) os_get_raw_spinlock(const os_spin_id_t spin_id)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;

	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	spin_suspend_lock_method(spin_cfg->method);

	__spin_lock(&(lock_inst->spin));

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_release_raw_spinlock(const os_spin_id_t spin_id)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;

	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	__spin_unlock(&(lock_inst->spin));

	spin_remove_lock_method(spin_cfg->method);

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_get_spinlock(const os_spin_id_t spin_id)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;
	irqlevel_t int_state;
	os_status_t check_stat;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
	if (UNLIKELY(spin_id >= os_spinlock_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
#endif
	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		check_stat = os_context_check_curcontext_id(cur_context, OS_CONTEXT_GETSPINLOCK);
		if (UNLIKELY(check_stat == FALSE)) {
			return OS_STATUS_CALLEVEL;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		app_id = OS_CONTEXT_GET_CURRENT_APPID(cur_context);
#endif

		check_stat = spin_local_core_check(lock_inst);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_INTERFERENCE_DEADLOCK;
		}

		check_stat = spin_order_check(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), spin_cfg->order);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_NESTING_DEADLOCK;
		}
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		check_stat = os_app_check_access(app_id, spin_cfg->access_app_mask);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	} else {
		/* do nothing */
	}

	spin_suspend_lock_method(spin_cfg->method);

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		os_int_suspend_kernel(&int_state);
		__spin_lock(&(lock_inst->spin));
		lock_inst->owner_context = cur_context;
		os_list_append(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), &(lock_inst->lock.lock_node));
		os_int_resume_kernel(&int_state);
	} else {
		__spin_lock(&(lock_inst->spin));
	}
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_release_spinlock(const os_spin_id_t spin_id)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;
	irqlevel_t int_state;
	os_status_t check_stat;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
	if (UNLIKELY(spin_id >= os_spinlock_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
#endif
	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		check_stat = os_context_check_curcontext_id(cur_context, OS_CONTEXT_RELEASESPINLOCK);
		if (UNLIKELY(check_stat == FALSE)) {
			return OS_STATUS_CALLEVEL;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		app_id = OS_CONTEXT_GET_CURRENT_APPID(cur_context);
#endif

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
		check_stat = os_app_check_access(app_id, spin_cfg->access_app_mask);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1 */
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
		check_stat = spin_entity_opccupy_check(cur_context, lock_inst->owner_context);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			/* 调用实体未拥有此锁 */
			return OS_STATUS_STATE;
		}
		check_stat = spin_fifo_check(cur_context, &(lock_inst->lock.lock_node));
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			/* 锁的释放不符合LIFO */
			return OS_STATUS_ORDER;
		}
	} else {
		/* do nothing */
	}

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		os_int_suspend_kernel(&int_state);
		os_list_remove(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), &(lock_inst->lock.lock_node));
		lock_inst->owner_context = NULL_PTR;
		__spin_unlock(&(lock_inst->spin));
		os_int_resume_kernel(&int_state);
	} else {
		__spin_unlock(&(lock_inst->spin));
	}

	spin_remove_lock_method(spin_cfg->method);
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_try_to_get_spinlock(const os_spin_id_t spin_id, try_get_spin_t *success)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;
	boolean lock_succ;
	irqlevel_t int_state;
	os_status_t check_stat;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
#if (CONFIG_RT_FRAMEWORK_EXTENDED_STATUS == 1)
	if (UNLIKELY(spin_id >= os_spinlock_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
#endif

	if (UNLIKELY(success == NULL)) {
		return OS_STATUS_PARAM_POINTER_2;
	}

	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		check_stat = os_context_check_curcontext_id(cur_context, OS_CONTEXT_TRYTOGETSPINLOCK);
		if (UNLIKELY(check_stat == FALSE)) {
			return OS_STATUS_CALLEVEL;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		app_id = OS_CONTEXT_GET_CURRENT_APPID(cur_context);
#endif

		check_stat = spin_local_core_check(lock_inst);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_INTERFERENCE_DEADLOCK;
		}
		check_stat = spin_order_check(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), spin_cfg->order);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_NESTING_DEADLOCK;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		check_stat = os_app_check_access(app_id, spin_cfg->access_app_mask);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	} else {
		/* do nothing */
	}

	spin_suspend_lock_method(spin_cfg->method);
	*success = TRYTOGETSPINLOCK_NOSUCCESS;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		os_int_suspend_kernel(&int_state);
		lock_succ = __spin_trylock(&(lock_inst->spin));
		if (LIKELY(lock_succ == TRUE)) {
			*success = TRYTOGETSPINLOCK_SUCCESS;
			lock_inst->owner_context = cur_context;
			os_list_append(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), &(lock_inst->lock.lock_node));
		}
		os_int_resume_kernel(&int_state);
	} else {
		lock_succ = __spin_trylock(&(lock_inst->spin));
		if (LIKELY(lock_succ == TRUE)) {
			*success = TRYTOGETSPINLOCK_SUCCESS;
		}
	}

	if (UNLIKELY(lock_succ != TRUE)) {
		spin_remove_lock_method(spin_cfg->method);
	}

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_get_spinlock_read(const os_spin_id_t spin_id)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;
	os_status_t check_stat;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (UNLIKELY(spin_id >= os_spinlock_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		check_stat = os_context_check_curcontext_id(cur_context, OS_CONTEXT_GETSPINLOCK);
		if (UNLIKELY(check_stat == FALSE)) {
			return OS_STATUS_CALLEVEL;
		}

		if (UNLIKELY(spin_local_core_check(lock_inst) != OS_STATUS_OK)) {
			return OS_STATUS_INTERFERENCE_DEADLOCK;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		app_id = OS_CONTEXT_GET_CURRENT_APPID(cur_context);
		check_stat = os_app_check_access(app_id, spin_cfg->access_app_mask);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	} else {
		/* do nothing */
	}

	spin_suspend_lock_method(spin_cfg->method);
	__spin_rd_lock((os_rw_spin_t *)(&(lock_inst->spin)));

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_get_spinlock_write(const os_spin_id_t spin_id)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;
	irqlevel_t int_state;
	os_status_t check_stat;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (UNLIKELY(spin_id >= os_spinlock_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		check_stat = os_context_check_curcontext_id(cur_context, OS_CONTEXT_GETSPINLOCK);
		if (UNLIKELY(check_stat == FALSE)) {
			return OS_STATUS_CALLEVEL;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		app_id = OS_CONTEXT_GET_CURRENT_APPID(cur_context);
#endif

		check_stat = spin_local_core_check(lock_inst);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_INTERFERENCE_DEADLOCK;
		}
		check_stat = spin_order_check(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), spin_cfg->order);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_NESTING_DEADLOCK;
		}
#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		check_stat = os_app_check_access(app_id, spin_cfg->access_app_mask);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	} else {
		/* do nothing */
	}

	spin_suspend_lock_method(spin_cfg->method);

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		os_int_suspend_kernel(&int_state);

		__spin_wr_lock((os_rw_spin_t *)(&(lock_inst->spin)));
		lock_inst->owner_context = cur_context;
		os_list_append(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), &(lock_inst->lock.lock_node));

		os_int_resume_kernel(&int_state);
	} else {
		__spin_wr_lock((os_rw_spin_t *)(&(lock_inst->spin)));
	}

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_release_spinlock_read(const os_spin_id_t spin_id)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;
	os_status_t check_stat;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (UNLIKELY(spin_id >= os_spinlock_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		check_stat = os_context_check_curcontext_id(cur_context, OS_CONTEXT_RELEASESPINLOCK);
		if (UNLIKELY(check_stat == FALSE)) {
			return OS_STATUS_CALLEVEL;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		app_id = OS_CONTEXT_GET_CURRENT_APPID(cur_context);
		check_stat = os_app_check_access(app_id, spin_cfg->access_app_mask);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

		if (UNLIKELY(lock_inst->spin == OS_SPINLOCK_UNLOCK)) {
			return OS_STATUS_NOT_OK;
		}

		if (UNLIKELY((lock_inst->spin & OS_SPINLOCK_WRITE_LOCKED) != (os_spin_t)OS_SPINLOCK_WRITE_UNLOCKED)) {
			return OS_STATUS_NOT_OK;
		}
	} else {
		/* do nothing */
	}

	__spin_rd_unlock((os_rw_spin_t *)(&(lock_inst->spin)));
	spin_remove_lock_method(spin_cfg->method);

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_release_spinlock_write(const os_spin_id_t spin_id)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;
	irqlevel_t int_state;
	os_status_t check_stat;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (UNLIKELY(spin_id >= os_spinlock_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}
	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		check_stat = os_context_check_curcontext_id(cur_context, OS_CONTEXT_RELEASESPINLOCK);
		if (UNLIKELY(check_stat == FALSE)) {
			return OS_STATUS_CALLEVEL;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		app_id = OS_CONTEXT_GET_CURRENT_APPID(cur_context);
		check_stat = os_app_check_access(app_id, spin_cfg->access_app_mask);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

		if (UNLIKELY(lock_inst->spin == OS_SPINLOCK_UNLOCK)) {
			return OS_STATUS_NOT_OK;
		}

		if (UNLIKELY(lock_inst->owner_context != cur_context)) {
			return OS_STATUS_NOT_OK;
		}
	} else {
		/* do nothing */
	}

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		os_int_suspend_kernel(&int_state);

		os_list_remove(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), &(lock_inst->lock.lock_node));
		lock_inst->owner_context = NULL_PTR;
		__spin_wr_unlock((os_rw_spin_t *)(&(lock_inst->spin)));

		os_int_resume_kernel(&int_state);
	} else {
		__spin_wr_unlock((os_rw_spin_t *)(&(lock_inst->spin)));
	}

	spin_remove_lock_method(spin_cfg->method);
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_try_to_get_spinlock_read(const os_spin_id_t spin_id, try_get_spin_t *success)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;
	boolean lock_succ;
	os_status_t check_stat;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (UNLIKELY(spin_id >= os_spinlock_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (UNLIKELY(success == NULL)) {
		return OS_STATUS_PARAM_POINTER_2;
	}

	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {

		check_stat = spin_local_core_check(lock_inst);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_INTERFERENCE_DEADLOCK;
		}

		check_stat = os_context_check_curcontext_id(cur_context, OS_CONTEXT_TRYTOGETSPINLOCK);
		if (UNLIKELY(check_stat == FALSE)) {
			return OS_STATUS_CALLEVEL;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		app_id = OS_CONTEXT_GET_CURRENT_APPID(cur_context);
		if (UNLIKELY(os_app_check_access(app_id, spin_cfg->access_app_mask) != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	} else {
		/* do nothing */
	}

	spin_suspend_lock_method(spin_cfg->method);
	*success = TRYTOGETSPINLOCK_NOSUCCESS;

	lock_succ = __spin_rd_trylock((os_rw_spin_t *)(&(lock_inst->spin)));
	if (LIKELY(lock_succ == TRUE)) {
		*success = TRYTOGETSPINLOCK_SUCCESS;
	} else {
		spin_remove_lock_method(spin_cfg->method);
	}

	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_try_to_get_spinlock_write(const os_spin_id_t spin_id, try_get_spin_t *success)
{
	os_spin_inst_t *lock_inst;
	const os_spinlock_cfg_t *spin_cfg;
	boolean lock_succ;
	irqlevel_t int_state;
	os_status_t check_stat;
	os_ctx_cfg_t *const cur_context = os_context_get_contexthdl();

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
	ApplicationIDType app_id = 0;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	if (UNLIKELY(spin_id >= os_spinlock_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	if (UNLIKELY(success == NULL)) {
		return OS_STATUS_PARAM_POINTER_2;
	}

	spin_cfg = os_spinlock_cfg_ref_table[spin_id];
	lock_inst = spin_cfg->obj_desc;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		check_stat = os_context_check_curcontext_id(cur_context, OS_CONTEXT_TRYTOGETSPINLOCK);
		if (UNLIKELY(check_stat == FALSE)) {
			return OS_STATUS_CALLEVEL;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		app_id = OS_CONTEXT_GET_CURRENT_APPID(cur_context);
#endif

		check_stat = spin_local_core_check(lock_inst);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_INTERFERENCE_DEADLOCK;
		}

		check_stat = spin_order_check(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), spin_cfg->order);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_NESTING_DEADLOCK;
		}

#if (CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1)
		check_stat = os_app_check_access(app_id, spin_cfg->access_app_mask);
		if (UNLIKELY(check_stat != OS_STATUS_OK)) {
			return OS_STATUS_ACCESS_RIGHT;
		}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
	} else {
		/* do nothing */
	}

	spin_suspend_lock_method(spin_cfg->method);
	*success = TRYTOGETSPINLOCK_NOSUCCESS;

	if (UNLIKELY(SPINLOCKCHECK_ENABLED == spin_cfg->check)) {
		os_int_suspend_kernel(&int_state);

		lock_succ = __spin_wr_trylock((os_rw_spin_t *)(&(lock_inst->spin)));
		if (LIKELY(lock_succ == TRUE)) {
			*success = TRYTOGETSPINLOCK_SUCCESS;
			lock_inst->owner_context = cur_context;
			os_list_append(OS_CONTEXT_GET_LOCKLIST_REF(cur_context), &(lock_inst->lock.lock_node));
		}

		os_int_resume_kernel(&int_state);
	} else {
		lock_succ = __spin_wr_trylock((os_rw_spin_t *)(&(lock_inst->spin)));
		if (LIKELY(lock_succ == TRUE)) {
			*success = TRYTOGETSPINLOCK_SUCCESS;
		}
	}

	if (UNLIKELY(lock_succ != TRUE)) {
		spin_remove_lock_method(spin_cfg->method);
	}

	return OS_STATUS_OK;
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
