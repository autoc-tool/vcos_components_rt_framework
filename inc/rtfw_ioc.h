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

#ifndef RT_FRAMEWORK_INC_OS_IOC_H
#define RT_FRAMEWORK_INC_OS_IOC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_ioc_internal.h"

/* Global Macro Definition */
#define DeclareIocSend(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_send, os_ioc_send_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_send, const os_ioc_send_cfg_t, ioc_id)

#define DeclareIocReceive(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_recv, os_ioc_recv_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_recv, const os_ioc_recv_cfg_t, ioc_id)

#define DeclareIocWrite(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_write, os_ioc_write_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_write, const os_ioc_write_cfg_t, ioc_id)

#define DeclareIocRead(section_const, section_var, ioc_id) \
		RTFW_LINK_ENTRY_DECLARE(section_var, os_obj_ioc_read, os_ioc_read_inst_t, ioc_id); \
		RTFW_LINK_ENTRY_DECLARE(section_const, os_ioc_read, const os_ioc_read_cfg_t, ioc_id)

#define IOCSENDCFGVAR(ioc_id)        RTFW_LINK_ENTRY_NAME(os_ioc_send, ioc_id)
#define IOCRECEIVECFGVAR(ioc_id)     RTFW_LINK_ENTRY_NAME(os_ioc_recv, ioc_id)
#define IOCWRITECFGVAR(ioc_id)       RTFW_LINK_ENTRY_NAME(os_ioc_write, ioc_id)
#define IOCREADCFGVAR(ioc_id)        RTFW_LINK_ENTRY_NAME(os_ioc_read, ioc_id)

#define IOCSENDOBJVAR(ioc_id)        RTFW_LINK_ENTRY_NAME(os_obj_ioc_send, ioc_id)
#define IOCRECEIVEOBJVAR(ioc_id)     RTFW_LINK_ENTRY_NAME(os_obj_ioc_recv, ioc_id)
#define IOCWRITEOBJVAR(ioc_id)       RTFW_LINK_ENTRY_NAME(os_obj_ioc_write, ioc_id)
#define IOCREADOBJVAR(ioc_id)        RTFW_LINK_ENTRY_NAME(os_obj_ioc_read, ioc_id)

/* Global Function Declaration */
#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
FUNC(StatusType, OS_CODE) IocSend(IocIdType ioc_id, void const *data);
FUNC(StatusType, OS_CODE) IocReceive(IocIdType ioc_id, void *data);
FUNC(StatusType, OS_CODE) IocEmptyQueue(IocIdType ioc_id);
FUNC(StatusType, OS_CODE) IocWrite(IocIdType ioc_id, void const *data);
FUNC(StatusType, OS_CODE) IocRead(IocIdType ioc_id, void *data);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /*! RT_FRAMEWORK_INC_OS_IOC_H */
