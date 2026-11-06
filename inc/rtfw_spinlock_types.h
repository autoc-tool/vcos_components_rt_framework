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

#ifndef RT_FRAMEWORK_INC_OS_SPINLOCK_TYPES_H
#define RT_FRAMEWORK_INC_OS_SPINLOCK_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */

/* Global Type & Structure Definition */
struct os_spin_inst_s;
typedef struct os_spin_inst_s os_spin_inst_t;

struct os_spin_cfg_s;
typedef struct os_spin_cfg_s os_spinlock_cfg_t; /* TODO: change cfg type with other modules */

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_SPINLOCK_TYPES_H */
