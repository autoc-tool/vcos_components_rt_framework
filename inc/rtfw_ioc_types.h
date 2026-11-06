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

#ifndef RT_FRAMEWORK_INC_OS_IOC_TYPES_H
#define RT_FRAMEWORK_INC_OS_IOC_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>

/* Global Type & Structure Definition */
struct os_ioc_callbacks_config_s;
typedef struct os_ioc_callbacks_config_s os_ioc_callbacks_config_t;

struct os_ioc_inst_s;
typedef struct os_ioc_inst_s os_ioc_inst_t;

struct os_ioc_cfg_s;
typedef struct os_ioc_cfg_s os_ioc_cfg_t;

struct os_ioc_send_inst_s;
typedef struct os_ioc_send_inst_s os_ioc_send_inst_t;

struct os_ioc_send_cfg_s;
typedef struct os_ioc_send_cfg_s os_ioc_send_cfg_t;

struct os_ioc_recv_inst_s;
typedef struct os_ioc_recv_inst_s os_ioc_recv_inst_t;

struct os_ioc_recv_cfg_s;
typedef struct os_ioc_recv_cfg_s os_ioc_recv_cfg_t;

struct os_ioc_write_inst_s;
typedef struct os_ioc_write_inst_s os_ioc_write_inst_t;

struct os_ioc_write_cfg_s;
typedef struct os_ioc_write_cfg_s os_ioc_write_cfg_t;

struct os_ioc_read_inst_s;
typedef struct os_ioc_read_inst_s os_ioc_read_inst_t;

struct os_ioc_read_cfg_s;
typedef struct os_ioc_read_cfg_s os_ioc_read_cfg_t;

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_IOC_TYPES_H */
