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

#ifndef RT_FRAMEWORK_INC_OS_IOC_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_IOC_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_hook_types.h"
#include "rtfw_ioc_types.h"

/* Global Enumeration Definition */
typedef enum {
	ioc_recv_u8_type,
	ioc_send_u8_type,
	ioc_recv_u16_type,
	ioc_send_u16_type,
	ioc_recv_u32_type,
	ioc_send_u32_type,
	ioc_recv_blk_type,
	ioc_send_blk_type,
	ioc_read_type,
	ioc_write_type
} os_ioc_type;

typedef enum {
	OS_IOC_GUARD_NONE,
	OS_IOC_GUARD_DISABLE_INTERRUPTS,
#ifdef CONFIG_RT_FRAMEWORK_SPINLOCK
	OS_IOC_GUARD_ENABLE_SPINLOCKS,
	OS_IOC_GUARD_ALL
#endif /* CONFIG_RT_FRAMEWORK_SPINLOCK == 1 */
} os_ioc_guard_type;

typedef enum {
	OS_IOC_CHANNEL_CLOSED,
	OS_IOC_CHANNEL_OPENED
} os_ioc_channel_state;

/* Global Type & Structure Definition */
typedef uint8 os_ioc_drop_cnt_type;
typedef uint8 os_ioc_ring_idx;

typedef os_spin_id_t os_spin_cfg_ref_t;
typedef os_hook_cfg_t os_ioc_call_back_ref_t;

typedef void (*os_ioc_copy_cbk)(void const *source, void *target);
typedef void (*os_ioc_init_cbk)(void *data);
typedef void (*os_ioc_write_cbk)(void *buffer, uint8 idx, void const *data);
typedef void (*os_ioc_read_cbk)(void const *buffer, uint8 idx, void *data_ref);

struct os_ioc_callbacks_config_s {
	uint32 callback_count;
	const os_ioc_call_back_ref_t *const *callback_refs;
};

struct os_ioc_inst_s {
	os_ioc_channel_state state;
};

struct os_ioc_cfg_s {
	os_ioc_inst_t *obj_desc;
	IocIdType ioc_id;
	os_ioc_type ioc_type;
	os_ioc_guard_type lock_type;
#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	uint32 accessing_appl_mask;
#endif /* CONFIG_RT_FRAMEWORK_ACCESSCHECK == 1 */
#ifdef CONFIG_RT_FRAMEWORK_SPINLOCK
	os_spin_cfg_ref_t spin_id;
#endif
};

struct os_ioc_send_inst_s {
	os_ioc_inst_t base;
	os_ioc_ring_idx write_idx;
	os_ioc_drop_cnt_type drop_cnt;
};

struct os_ioc_send_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_recv_cfg_t *recv;
	os_ioc_callbacks_config_t callbacks;
	os_ioc_write_cbk write_cbk;
	void *data;
	uint8 size;
};

struct os_ioc_recv_inst_s {
	os_ioc_inst_t base;
	os_ioc_ring_idx read_idx;
	os_ioc_drop_cnt_type drop_cnt_snapshot;
};

struct os_ioc_recv_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_send_cfg_t *send;
	os_ioc_read_cbk read_cbk;
	void *data;
	uint8 size;
};

struct os_ioc_write_inst_s {
	os_ioc_inst_t base;
};

struct os_ioc_write_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_read_cfg_t *read;
	void *data;
	os_ioc_copy_cbk copy_cbk;
	os_ioc_init_cbk init_cbk;
	os_ioc_callbacks_config_t callbacks;
};

struct os_ioc_read_inst_s {
	os_ioc_inst_t base;
};

struct os_ioc_read_cfg_s {
	os_ioc_cfg_t ioc;
	const os_ioc_write_cfg_t *write;
	void *data;
	os_ioc_copy_cbk copy_cbk;
};

/* Global Variable Definition */
DeclareRtfwRefTable_H(ioc) /*! Reference to OS iocchannels configuration table  */

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
extern void os_ioc_init(const os_ioc_cfg_t *const os_ioc_cfg_table[], OsObjIndexType size);
extern FUNC(os_status_t, OS_CODE) os_ioc_send(IocIdType ioc_id, void const *data);
extern FUNC(os_status_t, OS_CODE) os_ioc_recv(IocIdType ioc_id, void *data);
extern FUNC(os_status_t, OS_CODE) os_ioc_empty_queue(IocIdType ioc_id);
extern FUNC(os_status_t, OS_CODE) os_ioc_write(IocIdType ioc_id, void const *data);
extern FUNC(os_status_t, OS_CODE) os_ioc_read(IocIdType ioc_id, void *data);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_IOC_INTERNAL_H */
