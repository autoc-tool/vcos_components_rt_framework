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
#include "rtfw_ioc.h"
#include "rtfw_error.h"
#include "rtfw_task.h"

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
#include "rtfw_hook.h"
#endif

#if (defined(CONFIG_RT_FRAMEWORK_RESOURCE) && (CONFIG_RT_FRAMEWORK_RESOURCE == 1)) ||                                  \
	(defined(CONFIG_RT_FRAMEWORK_SPINLOCK) && (CONFIG_RT_FRAMEWORK_SPINLOCK == 1))
#include "rtfw_spinlock_internal.h"
#endif /* CONFIG_RT_FRAMEWORK_RESOURCE || CONFIG_RT_FRAMEWORK_SPINLOCK */

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
#include "rtfw_application.h"
#include "application_cfg.h"
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
static inline void os_ioc_get_lock(const os_ioc_cfg_t *const ioc)
{
	switch (ioc->lock_type) {
		case OS_IOC_GUARD_NONE:
			break;
		case OS_IOC_GUARD_DISABLE_INTERRUPTS:
			os_suspend_os_interrupts();
			break;
#ifdef CONFIG_RT_FRAMEWORK_SPINLOCK
		case OS_IOC_GUARD_ENABLE_SPINLOCKS:
			(void)os_get_raw_spinlock(ioc->spin_id);
			break;
		case OS_IOC_GUARD_ALL:
			os_suspend_os_interrupts();
			(void)os_get_raw_spinlock(ioc->spin_id);
			break;
#endif /*CONFIG_RT_FRAMEWORK_SPINLOCK == 1*/
		default:
			break;
	}
}

static inline void os_ioc_release_lock(const os_ioc_cfg_t *const ioc)
{
	switch (ioc->lock_type) {
		case OS_IOC_GUARD_NONE:
			break;
		case OS_IOC_GUARD_DISABLE_INTERRUPTS:
			os_resume_os_interrupts();
			break;
#ifdef CONFIG_RT_FRAMEWORK_SPINLOCK
		case OS_IOC_GUARD_ENABLE_SPINLOCKS:
			(void)os_release_raw_spinlock(ioc->spin_id);
			break;
		case OS_IOC_GUARD_ALL:
			(void)os_release_raw_spinlock(ioc->spin_id);
			os_resume_os_interrupts();
			break;
#endif /*CONFIG_RT_FRAMEWORK_SPINLOCK == 1*/
		default:
			REPORT_FATAL_ERROR();
			break;
	}
}

static inline uint8 os_ioc_channel_is_active(const os_ioc_cfg_t *const ioc)
{
	const os_ioc_inst_t *inst = ioc->obj_desc;

	return ((inst->state) == OS_IOC_CHANNEL_OPENED) ? 1U : 0U;
}

static inline void os_ioc_close_channel(const os_ioc_cfg_t *const ioc)
{
	os_ioc_inst_t *inst = ioc->obj_desc;

	inst->state = OS_IOC_CHANNEL_CLOSED;
}

static inline os_status_t os_ioc_call_callbacks(const os_ioc_callbacks_config_t *const callbacks)
{
	os_status_t ret = OS_STATUS_OK;

	if (NULL != callbacks) {
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
		for (uint32 callback_index = 0U; callback_index < callbacks->callback_count; callback_index++) {
			if (E_OK != rtfw_call_void_callback_hook(callbacks->callback_refs[callback_index])) {
				ret = OS_STATUS_NOT_OK;
			}
		}
#else
		ret = OS_STATUS_NOT_OK;
#endif
	} else {
		ret = OS_STATUS_OK;
	}

	return ret;
}

static inline boolean os_ioc_ring_index_check(os_ioc_ring_idx write_idx, os_ioc_ring_idx read_idx, os_ioc_ring_idx size)
{
	return ((write_idx < size) && (read_idx < size));
}

static inline void os_ioc_ring_index_increase(os_ioc_ring_idx *index, os_ioc_ring_idx size)
{
	if ((*index) >= size) {
		REPORT_FATAL_ERROR();
	}

	if ((*index) == (size - 1U)) {
		(*index) = 0U;
	} else {
		(*index)++;
	}
}

static inline boolean os_ioc_ring_is_full(os_ioc_ring_idx write_idx, os_ioc_ring_idx read_idx, os_ioc_ring_idx size)
{
	os_ioc_ring_idx write_idx_temp = write_idx;

	os_ioc_ring_index_increase(&write_idx_temp, size);

	return (write_idx_temp == read_idx);
}

static inline boolean os_ioc_ring_is_empty(os_ioc_ring_idx write_idx, os_ioc_ring_idx read_idx)
{
	return (read_idx == write_idx);
}

static inline void os_ioc_increase_drop_cnt(os_ioc_drop_cnt_type *const drop_cnt)
{
	(*drop_cnt)++;
}

static inline os_status_t os_ioc_check_drop_cnt(os_ioc_drop_cnt_type *const recv_counter,
							   os_ioc_drop_cnt_type *const send_counter)
{
	const uint8 lost = (uint8)(*send_counter != *recv_counter);

	*recv_counter = (lost) ? (*send_counter) : (*recv_counter);

	return lost ? OS_STATUS_IOC_CHECK_FAILED : OS_STATUS_OK;
}

static inline void os_ioc_ring_reset(os_ioc_ring_idx const *write_idx, os_ioc_ring_idx *read_idx)
{
	(*read_idx) = (*write_idx);
}

static FUNC(void, OS_CODE) os_ioc_send_init(const os_ioc_send_cfg_t *send_cfg)
{
	os_ioc_send_inst_t *send_inst;

	if ((NULL != send_cfg) && (NULL != send_cfg->ioc.obj_desc)) {
		send_inst = (os_ioc_send_inst_t *)(send_cfg->ioc.obj_desc);
		send_inst->base.state = OS_IOC_CHANNEL_OPENED;
		send_inst->write_idx = 0;
		send_inst->drop_cnt = 0;
	}
}

static FUNC(void, OS_CODE) os_ioc_recv_init(const os_ioc_recv_cfg_t *recv_cfg)
{
	os_ioc_recv_inst_t *recv_inst;

	if ((NULL != recv_cfg) && (NULL != recv_cfg->ioc.obj_desc)) {
		recv_inst = (os_ioc_recv_inst_t *)(recv_cfg->ioc.obj_desc);
		recv_inst->base.state = OS_IOC_CHANNEL_OPENED;
		recv_inst->read_idx = 0;
		recv_inst->drop_cnt_snapshot = 0;
	}
}

static FUNC(void, OS_CODE) os_ioc_read_init(const os_ioc_read_cfg_t *read_cfg)
{
	os_ioc_read_inst_t *read_inst;

	if ((NULL != read_cfg) && (NULL != read_cfg->ioc.obj_desc)) {
		read_inst = (os_ioc_read_inst_t *)(read_cfg->ioc.obj_desc);
		read_inst->base.state = OS_IOC_CHANNEL_OPENED;
	}
}

static FUNC(void, OS_CODE) os_ioc_write_init(const os_ioc_write_cfg_t *write_cfg)
{
	os_ioc_write_inst_t *write_inst;

	if ((NULL != write_cfg) && (NULL != write_cfg->ioc.obj_desc)) {
		write_inst = (os_ioc_write_inst_t *)(write_cfg->ioc.obj_desc);
		write_inst->base.state = OS_IOC_CHANNEL_OPENED;
		write_cfg->init_cbk(write_cfg->data);
	}
}

static inline void os_ioc_store_data(void *buf, os_ioc_ring_idx idx, uint8 type, const void *data_in)
{
	switch (type) {
		case ioc_send_u8_type:
			((uint8 *)buf)[idx] = *(const uint8 *)data_in;
			break;
		case ioc_send_u16_type:
			((uint16 *)buf)[idx] = *(const uint16 *)data_in;
			break;
		case ioc_send_u32_type:
			((uint32 *)buf)[idx] = *(const uint32 *)data_in;
			break;
		default:
			REPORT_FATAL_ERROR();
			break;
	}
}

static inline void os_ioc_load_data(const void *buf, os_ioc_ring_idx idx, uint8 type, void *data_out)
{
	switch (type) {
		case ioc_recv_u8_type:
			*(uint8 *)data_out = ((const uint8 *)buf)[idx];
			break;
		case ioc_recv_u16_type:
			*(uint16 *)data_out = ((const uint16 *)buf)[idx];
			break;
		case ioc_recv_u32_type:
			*(uint32 *)data_out = ((const uint32 *)buf)[idx];
			break;
		default:
			REPORT_FATAL_ERROR();
			break;
	}
}

FUNC(void, OS_CODE) os_ioc_init(const os_ioc_cfg_t *const os_ioc_cfg_table[], OsObjIndexType size)
{
	if (size == (OsObjIndexType)0U) {
		return;
	}

	const os_ioc_cfg_t *base_cfg;
	uint16 index = 0;

	for (index = 0U; index < size; index++) {
		base_cfg = os_ioc_cfg_table[index];
		if (base_cfg == NULL_PTR) {
			continue;
		}
		switch (base_cfg->ioc_type) {
			case ioc_send_u8_type:
			case ioc_send_u16_type:
			case ioc_send_u32_type:
			case ioc_send_blk_type:
				os_ioc_send_init((os_ioc_send_cfg_t *)base_cfg);
				break;
			case ioc_recv_u8_type:
			case ioc_recv_u16_type:
			case ioc_recv_u32_type:
			case ioc_recv_blk_type:
				os_ioc_recv_init((os_ioc_recv_cfg_t *)base_cfg);
				break;
			case ioc_read_type:
				os_ioc_read_init((os_ioc_read_cfg_t *)base_cfg);
				break;
			case ioc_write_type:
				os_ioc_write_init((os_ioc_write_cfg_t *)base_cfg);
				break;
			default:
				break;
		}
	}
}

FUNC(os_status_t, OS_CODE) os_ioc_send(IocIdType ioc_id, void const *data)
{
	const os_ioc_cfg_t *base = (const os_ioc_cfg_t *)os_ioc_cfg_ref_table[ioc_id];
	const os_ioc_send_cfg_t *send_cfg = (const os_ioc_send_cfg_t *)base;
	const os_ioc_recv_cfg_t *recv_cfg = send_cfg->recv;

	os_ioc_send_inst_t *send_inst = (os_ioc_send_inst_t *)(send_cfg->ioc.obj_desc);
	os_ioc_recv_inst_t *recv_inst = (os_ioc_recv_inst_t *)(recv_cfg->ioc.obj_desc);

#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	ApplicationIDType app_id = 0;
	if ((OS_STATUS_OK != get_task_app_id(&app_id)) ||
	    (OS_STATUS_OK != os_app_check_access(app_id, send_cfg->ioc.accessing_appl_mask))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_status_t ret = OS_STATUS_OK;

	os_ioc_get_lock(&(send_cfg->ioc));

	if (os_ioc_channel_is_active(&(send_cfg->ioc)) == 0u) {
		ret = OS_STATUS_LIMIT;
	} else if (os_ioc_channel_is_active(&(send_cfg->recv->ioc)) == 0u) {
		os_ioc_close_channel(&(send_cfg->ioc));
		ret = OS_STATUS_LIMIT;
	} else if (!os_ioc_ring_index_check(send_inst->write_idx, recv_inst->read_idx, send_cfg->size)) {
		os_ioc_close_channel(&(send_cfg->ioc));
		ret = OS_STATUS_LIMIT;
	} else if (os_ioc_ring_is_full(send_inst->write_idx, recv_inst->read_idx, send_cfg->size)) {
		os_ioc_increase_drop_cnt(&send_inst->drop_cnt);
		ret = OS_STATUS_IOC_LOST_DATA;
	} else {
		if (send_cfg->write_cbk != NULL_PTR) {
			send_cfg->write_cbk(send_cfg->data, send_inst->write_idx, data);
		} else {
			os_ioc_store_data(send_cfg->data, send_inst->write_idx, send_cfg->ioc.ioc_type, data);
		}
		os_ioc_ring_index_increase(&send_inst->write_idx, send_cfg->size);
	}

	os_ioc_release_lock(&(send_cfg->ioc));

	if (ret == OS_STATUS_OK) {
		ret = os_ioc_call_callbacks(&(send_cfg->callbacks));
	}

	return ret;
}

FUNC(os_status_t, OS_CODE) os_ioc_recv(IocIdType ioc_id, void *data)
{
	const os_ioc_cfg_t *base = (const os_ioc_cfg_t *)os_ioc_cfg_ref_table[ioc_id];
	const os_ioc_recv_cfg_t *recv_cfg = (const os_ioc_recv_cfg_t *)base;
	const os_ioc_send_cfg_t *send_cfg = recv_cfg->send;

	os_ioc_recv_inst_t *recv_inst = (os_ioc_recv_inst_t *)(recv_cfg->ioc.obj_desc);
	os_ioc_send_inst_t *send_inst = (os_ioc_send_inst_t *)(send_cfg->ioc.obj_desc);

#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	ApplicationIDType app_id = 0;
	if ((OS_STATUS_OK != get_task_app_id(&app_id)) ||
	    (OS_STATUS_OK != os_app_check_access(app_id, recv_cfg->ioc.accessing_appl_mask))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_status_t ret = OS_STATUS_OK;

	os_ioc_get_lock(&(recv_cfg->ioc));

	if (os_ioc_channel_is_active(&(recv_cfg->ioc)) == 0u) {
		ret = OS_STATUS_IOC_NO_DATA;
	} else if (os_ioc_channel_is_active(&(recv_cfg->send->ioc)) == 0u) {
		os_ioc_close_channel(&(recv_cfg->ioc));
		ret = OS_STATUS_IOC_NO_DATA;
	} else if (!os_ioc_ring_index_check(send_inst->write_idx, recv_inst->read_idx, recv_cfg->size)) {
		os_ioc_close_channel(&(recv_cfg->ioc));
		ret = OS_STATUS_IOC_NO_DATA;
	} else {
		const os_status_t lost =
			os_ioc_check_drop_cnt(&recv_inst->drop_cnt_snapshot, &send_inst->drop_cnt);

		if (os_ioc_ring_is_empty(send_inst->write_idx, recv_inst->read_idx)) {
			ret = (lost == OS_STATUS_IOC_CHECK_FAILED) ? OS_STATUS_IOC_LOST_AND_NO_DATA :
									   OS_STATUS_IOC_NO_DATA;
		} else {
			if (recv_cfg->read_cbk != NULL_PTR) {
				recv_cfg->read_cbk(recv_cfg->data, recv_inst->read_idx, data);
			} else {
				os_ioc_load_data(recv_cfg->data, recv_inst->read_idx, recv_cfg->ioc.ioc_type, data);
			}
			os_ioc_ring_index_increase(&recv_inst->read_idx, recv_cfg->size);
			ret = (lost == OS_STATUS_IOC_CHECK_FAILED) ? OS_STATUS_IOC_LOST_DATA : OS_STATUS_OK;
		}
	}

	os_ioc_release_lock(&(recv_cfg->ioc));

	return (os_status_t)ret;
}

FUNC(os_status_t, OS_CODE) os_ioc_empty_queue(IocIdType ioc_id)
{
	const os_ioc_cfg_t *base = (const os_ioc_cfg_t *)os_ioc_cfg_ref_table[ioc_id];
	const os_ioc_recv_cfg_t *recv_cfg = (const os_ioc_recv_cfg_t *)base;
	const os_ioc_send_cfg_t *send_cfg = recv_cfg->send;

	os_ioc_recv_inst_t *recv_inst = (os_ioc_recv_inst_t *)(recv_cfg->ioc.obj_desc);
	os_ioc_send_inst_t *send_inst = (os_ioc_send_inst_t *)(send_cfg->ioc.obj_desc);

#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	ApplicationIDType app_id = 0;
	if ((OS_STATUS_OK != get_task_app_id(&app_id)) ||
	    (OS_STATUS_OK != os_app_check_access(app_id, recv_cfg->ioc.accessing_appl_mask))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_status_t ret = OS_STATUS_NOT_OK;

	os_ioc_get_lock(&(recv_cfg->ioc));
	os_ioc_ring_reset(&(send_inst->write_idx), &(recv_inst->read_idx));
	os_ioc_release_lock(&(recv_cfg->ioc));

	ret = OS_STATUS_OK;

	return ret;
}

FUNC(os_status_t, OS_CODE) os_ioc_write(IocIdType ioc_id, void const *data)
{
	const os_ioc_cfg_t *base = (const os_ioc_cfg_t *)os_ioc_cfg_ref_table[ioc_id];
	const os_ioc_write_cfg_t *write_cfg = (const os_ioc_write_cfg_t *)base;

#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	ApplicationIDType app_id = 0;
	if ((OS_STATUS_OK != get_task_app_id(&app_id)) ||
	    (OS_STATUS_OK != os_app_check_access(app_id, write_cfg->ioc.accessing_appl_mask))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_status_t ret = OS_STATUS_NOT_OK;

	os_ioc_get_lock(&(write_cfg->ioc));

	write_cfg->copy_cbk(data, write_cfg->data);

	os_ioc_release_lock(&(write_cfg->ioc));

	ret = os_ioc_call_callbacks(&(write_cfg->callbacks));

	return ret;
}

FUNC(os_status_t, OS_CODE) os_ioc_read(IocIdType ioc_id, void *data)
{
	const os_ioc_cfg_t *base = (const os_ioc_cfg_t *)os_ioc_cfg_ref_table[ioc_id];
	const os_ioc_read_cfg_t *read_cfg = (const os_ioc_read_cfg_t *)base;

#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	ApplicationIDType app_id = 0;
	if ((OS_STATUS_OK != get_task_app_id(&app_id)) ||
	    (OS_STATUS_OK != os_app_check_access(app_id, read_cfg->ioc.accessing_appl_mask))) {
		return OS_STATUS_ACCESS_RIGHT;
	}
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */

	os_status_t ret = OS_STATUS_NOT_OK;

	os_ioc_get_lock(&(read_cfg->ioc));

	read_cfg->copy_cbk(read_cfg->data, data);

	os_ioc_release_lock(&(read_cfg->ioc));

	ret = OS_STATUS_OK;

	return ret;
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
