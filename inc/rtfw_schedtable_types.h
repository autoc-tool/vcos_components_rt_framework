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

#ifndef RT_FRAMEWORK_INC_OS_SCHED_TABLE_TYPES_H
#define RT_FRAMEWORK_INC_OS_SCHED_TABLE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif
/* Including File */
#include <nuttx/config.h>

/* Global Type & Structure Definition */
struct os_schedtable_inst;
typedef struct os_schedtable_inst os_schedtable_inst_t;

struct os_schedtable_set_event;
typedef struct os_schedtable_set_event os_schedtable_set_event_t;

struct os_schedtable_expiry_point;
typedef struct os_schedtable_expiry_point os_schedtable_expiry_point_t;

struct os_schedtable_sync;
typedef struct os_schedtable_sync os_schedtable_sync_t;

struct os_schedtable_adjust_expiry_point;
typedef struct os_schedtable_adjust_expiry_point os_schedtable_adjust_expiry_point_t;

struct os_schedtable_cfg;
typedef struct os_schedtable_cfg os_schedtable_cfg_t;

struct os_schedtable_autostart;
typedef struct os_schedtable_autostart os_schedtable_autostart_t;

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_SCHED_TABLE_TYPES_H */
