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

#ifndef RT_FRAMEWORK_INC_RTFW_TYPES_H
#define RT_FRAMEWORK_INC_RTFW_TYPES_H

#include <nuttx/config.h>
#include <nuttx/sched.h>
#include <nuttx/event.h>
#include "Os_Types.h"
#include <stddef.h>
#include <stdint.h>

typedef int16_t isr_id_t;

typedef nxevent_mask_t os_event_mask_t;

/* Global Type & Structure Definition */
typedef uintptr_t int_global_state_t; /* Interrupt global state type */
typedef uintptr_t int_vector_num_t; /* Interrupt vector number */
typedef uintptr_t register_t; /* Register type */
typedef uint16_t os_spin_id_t;

typedef int_global_state_t os_int_global_state_t;

typedef uint32_t os_context_id_t;

typedef TryToGetSpinlockType try_get_spin_t;

typedef uint32_t os_status_t;

#define OS_STATUS_SHIFT   (sizeof(StatusType) * 8U)

/* 创建一个状态码的变体 */
#define OS_STATUS_TYPE_CONVERSION(Status, Variant) \
	(((os_status_t)(Variant) << OS_STATUS_SHIFT) | \
	 ((os_status_t)(Status) & ((1U << OS_STATUS_SHIFT) - 1U)))

#define OS_STATUS_OK                       ((os_status_t)E_OK)
/* OSEK Error Codes */
/* ===== E_OS_ACCESS related detailed status ===== */
#define OS_STATUS_ACCESS                   ((os_status_t)E_OS_ACCESS)
/* 指定对象无访问权限 */
#define OS_STATUS_ACCESS_RIGHT             (OS_STATUS_TYPE_CONVERSION(E_OS_ACCESS, 1u))
/* 对象的应用程序状态不可访问 */
#define OS_STATUS_NOT_ACCESSIBLE           (OS_STATUS_TYPE_CONVERSION(E_OS_ACCESS, 2u))
/* 调用者不是扩展任务 */
#define OS_STATUS_NOT_EXTENDED_TASK        (OS_STATUS_TYPE_CONVERSION(E_OS_ACCESS, 3u))
/* 给定的锁已经被锁定 */
#define OS_STATUS_LOCKED                   (OS_STATUS_TYPE_CONVERSION(E_OS_ACCESS, 4u))
/* 给定的对象具有无效的优先级 */
#define OS_STATUS_PRIORITY                 (OS_STATUS_TYPE_CONVERSION(E_OS_ACCESS, 5u))
/* 一般错误代码 */
#define OS_STATUS_NOT_OK                   (OS_STATUS_TYPE_CONVERSION(E_OS_ACCESS, 6u))

/* ===== E_OS_CALLEVEL related detailed status ===== */
#define OS_STATUS_CALLEVEL                 ((os_status_t)E_OS_CALLEVEL)
/* 调用上下文无效 */
#define OS_STATUS_CALLEVEL_SHARED_STACK    (OS_STATUS_TYPE_CONVERSION(E_OS_CALLEVEL, 1u))

/* ===== E_OS_ID related detailed status ===== */
#define OS_STATUS_ID                       ((os_status_t)E_OS_ID)
/* 参数1中给定的ID无效 */
#define OS_STATUS_ID_1                     (OS_STATUS_TYPE_CONVERSION(E_OS_ID, 1u))
/* 参数2中给定的ID无效 */
#define OS_STATUS_ID_2                     (OS_STATUS_TYPE_CONVERSION(E_OS_ID, 2u))
/* 参数1中给定的类型无效 */
#define OS_STATUS_ID_TYPE_1                (OS_STATUS_TYPE_CONVERSION(E_OS_ID, 3u))
/* 给定对象的类型分配到不同的计数器 */
#define OS_STATUS_COUNTER_TYPE_ID          (OS_STATUS_TYPE_CONVERSION(E_OS_ID, 4u))
/* 给定对象的类型具有不同的同步类型 */
#define OS_STATUS_SYNC_TYPE_ID             (OS_STATUS_TYPE_CONVERSION(E_OS_ID, 5u))

#define OS_STATUS_LIMIT                    ((os_status_t)E_OS_LIMIT)

/* ===== E_OS_NOFUNC related detailed status ===== */
#define OS_STATUS_NOFUNC                   ((os_status_t)E_OS_NOFUNC)
/* 操作系统对象当前处于解锁状态 */
#define OS_STATUS_UNLOCKED                 (OS_STATUS_TYPE_CONVERSION(E_OS_NOFUNC, 1u))
/* 参数1中给定的操作系统对象当前未被使用 */
#define OS_STATUS_NOFUNC_1                 (OS_STATUS_TYPE_CONVERSION(E_OS_NOFUNC, 2u))
/* 参数1中给定的操作系统对象当前停止 */
#define OS_STATUS_STOPPED_1                (OS_STATUS_TYPE_CONVERSION(E_OS_NOFUNC, 3u))
/* 操作系统对象的访问顺序错误 */
#define OS_STATUS_ORDER                    (OS_STATUS_TYPE_CONVERSION(E_OS_NOFUNC, 4u))

#define OS_STATUS_RESOURCE                 ((os_status_t)E_OS_RESOURCE)

/* ===== E_OS_STATE related detailed status ===== */
#define OS_STATUS_STATE                    ((os_status_t)E_OS_STATE)
/* 当前应用程序不处于重新启动状态 */
#define OS_STATUS_NO_RESTART               (OS_STATUS_TYPE_CONVERSION(E_OS_STATE, 1u))
/* 操作系统对象处于停止状态，不允许执行请求的操作 */
#define OS_STATUS_STOPPED                  (OS_STATUS_TYPE_CONVERSION(E_OS_STATE, 2u))

/* ===== E_OS_VALUE related detailed status ===== */
#define OS_STATUS_VALUE                    ((os_status_t)E_OS_VALUE)
/* 参数1中给定的值过高 */
#define OS_STATUS_VALUE_TOO_HIGH_1         (OS_STATUS_TYPE_CONVERSION(E_OS_VALUE, 1u))
/* 参数2中给定的值过高 */
#define OS_STATUS_VALUE_TOO_HIGH_2         (OS_STATUS_TYPE_CONVERSION(E_OS_VALUE, 2u))
/* 参数1中给定的值超出了范围 */
#define OS_STATUS_VALUE_OUT_OF_RANGE_1     (OS_STATUS_TYPE_CONVERSION(E_OS_VALUE, 3u))
/* 参数2中给定的值超出了范围 */
#define OS_STATUS_VALUE_OUT_OF_RANGE_2     (OS_STATUS_TYPE_CONVERSION(E_OS_VALUE, 4u))
/* 参数1中给定的值过低 */
#define OS_STATUS_VALUE_TOO_LOW_1          (OS_STATUS_TYPE_CONVERSION(E_OS_VALUE, 5u))
/* 参数2中给定的值过低 */
#define OS_STATUS_VALUE_TOO_LOW_2          (OS_STATUS_TYPE_CONVERSION(E_OS_VALUE, 6u))

/* AUTOSAR OS Error Classification */
#define OS_STATUS_SERVICEID                ((os_status_t)E_OS_SERVICEID)
#define OS_STATUS_ILLEGAL_ADDRESS          ((os_status_t)E_OS_ILLEGAL_ADDRESS)
#define OS_STATUS_MISSINGEND               ((os_status_t)E_OS_MISSINGEND)
#define OS_STATUS_DISABLEDINT              ((os_status_t)E_OS_DISABLEDINT)
#define OS_STATUS_STACKFAULT               ((os_status_t)E_OS_STACKFAULT)
#define OS_STATUS_PROTECTION_MEMORY        ((os_status_t)E_OS_PROTECTION_MEMORY)
#define OS_STATUS_PROTECTION_TIME          ((os_status_t)E_OS_PROTECTION_TIME)
#define OS_STATUS_PROTECTION_ARRIVAL       ((os_status_t)E_OS_PROTECTION_ARRIVAL)
#define OS_STATUS_PROTECTION_LOCKED        ((os_status_t)E_OS_PROTECTION_LOCKED)
#define OS_STATUS_PROTECTION_EXCEPTION     ((os_status_t)E_OS_PROTECTION_EXCEPTION)
#define OS_STATUS_CORE                     ((os_status_t)E_OS_CORE)
#define OS_STATUS_SPINLOCK                 ((os_status_t)E_OS_SPINLOCK)
#define OS_STATUS_INTERFERENCE_DEADLOCK    ((os_status_t)E_OS_INTERFERENCE_DEADLOCK)
#define OS_STATUS_NESTING_DEADLOCK         ((os_status_t)E_OS_NESTING_DEADLOCK)

/* ===== E_OS_PARAM_POINTER related detailed status ===== */
#define OS_STATUS_PARAM_POINTER            ((os_status_t)E_OS_PARAM_POINTER)
/* 参数1中给定的指针为null */
#define OS_STATUS_PARAM_POINTER_1          (OS_STATUS_TYPE_CONVERSION(E_OS_PARAM_POINTER, 1u))
/* 参数2中给定的指针为null */
#define OS_STATUS_PARAM_POINTER_2          (OS_STATUS_TYPE_CONVERSION(E_OS_PARAM_POINTER, 2u))
/* 参数3中给定的指针为null */
#define OS_STATUS_PARAM_POINTER_3          (OS_STATUS_TYPE_CONVERSION(E_OS_PARAM_POINTER, 3u))

/* RTFW Error codes */
#define OS_STATUS_RATE                     ((os_status_t)E_OS_RATE)
#define OS_STATUS_PROTECTION_RATE          ((os_status_t)E_OS_PROTECTION_RATE)

#define OS_STATUS_TIMEOUT                  ((os_status_t)E_OS_TIMEOUT)
#define OS_STATUS_CRC                      ((os_status_t)E_OS_CRC)
#define OS_STATUS_QUEUE_FULL               ((os_status_t)E_OS_QUEUE_FULL)
#define OS_STATUS_QUEUE_EMPTY              ((os_status_t)E_OS_QUEUE_EMPTY)
#define OS_STATUS_QUEUE_OVERWRITTEN        ((os_status_t)E_OS_QUEUE_OVERWRITTEN)
#define OS_STATUS_QUEUE_NOCMPFUNC          ((os_status_t)E_OS_QUEUE_NOCMPFUNC)
#define OS_STATUS_NOTFOUND                 ((os_status_t)E_OS_NOTFOUND)

#define OS_STATUS_IPICALL_UNAVAILABLE      ((os_status_t)E_OS_IPICALL_UNAVAILABLE)
#define OS_STATUS_IPICALL_OVERFLOW         ((os_status_t)E_OS_IPICALL_OVERFLOW)

#define OS_STATUS_IOC_CHECK_FAILED         ((os_status_t)E_OS_IOC_CHECK_FAILED)
#define OS_STATUS_IOC_LOST_DATA            ((os_status_t)E_OS_IOC_LOST_DATA)
#define OS_STATUS_IOC_NO_DATA              ((os_status_t)E_OS_IOC_NO_DATA)
#define OS_STATUS_IOC_INVALID_CLASS        ((os_status_t)E_OS_IOC_INVALID_CLASS)
#define OS_STATUS_IOC_UNAVAILABLE          ((os_status_t)E_OS_IOC_UNAVAILABLE)

#define OS_STATUS_IOC_LOST_AND_NO_DATA     ((os_status_t)(E_OS_IOC_NO_DATA | E_OS_IOC_LOST_DATA))
#define OS_STATUS_IOC_CHANNEL_CLOSED       ((os_status_t)OS_STATUS_TYPE_CONVERSION(E_OS_IOC_UNAVAILABLE, 1u))
#define OS_STATUS_IOC_CLOSE_CHANNEL        ((os_status_t)OS_STATUS_TYPE_CONVERSION(E_OS_IOC_UNAVAILABLE, 2u))

#define OS_STATUS_FIFO_FULL                ((os_status_t)E_OS_FIFO_FULL)
#define OS_STATUS_FIFO_EMPTY               ((os_status_t)E_OS_FIFO_EMPTY)

#define OS_STATUS_INT_API_OVERFLOW         ((os_status_t)E_OS_INT_API_OVERFLOW)
#define OS_STATUS_INT_API_ORDER            ((os_status_t)E_OS_INT_API_ORDER)

#define OS_STATUS_ILLEGAL                  ((os_status_t)E_OS_ILLEGAL)

#define RTFW_OFFSETOF(type, member) offsetof(type, member) ///< 获取结构体成员的偏移量

#define NELEMENTS(array) (sizeof(array) / sizeof((array)[0])) ///< 获取数组元素的个数

#define RTFW_APPID2MASK(appid) (0x00000001U << (appid)) ///< 获取应用程序ID掩码

#define RTFW_LINK_ENTRY_NAME(_class, _id) _class##_##_id

#define RTFW_LINK_ENTRY_DECLARE(_section, _class, _type, _id)                                                          \
	_type RTFW_LINK_ENTRY_NAME(_class, _id) locate_data(_section)

#define RTFW_TABLE_DECLARE(_section, _class, _type, _id, _size)                                                        \
	_type RTFW_LINK_ENTRY_NAME(_class, _id)[(_size) / sizeof(_type)] locate_data(_section)

#define RTFW_TABLE_DECLARE_BY_ARRAY_MEMBER(_section, _class, _type, _id, _num)                                         \
	_type RTFW_LINK_ENTRY_NAME(_class, _id)[_num] locate_data(_section)

#define DeclareRtfwRefTable(_section, _class, _size)                                                                   \
	const locate_data(_section) OsObjIndexType os_##_class##_cfg_ref_table_size = (_size);                           \
	const locate_data(_section) os_##_class##_cfg_t *const os_##_class##_cfg_ref_table[(_size)]

#define DeclareRtfwRefTable_H(_class)                                                                                  \
	extern const OsObjIndexType os_##_class##_cfg_ref_table_size;                                                  \
	extern const os_##_class##_cfg_t *const os_##_class##_cfg_ref_table[];

#endif /* RT_FRAMEWORK_INC_RTFW_TYPES_H */