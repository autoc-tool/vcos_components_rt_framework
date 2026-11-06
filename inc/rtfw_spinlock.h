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

#ifndef RT_FRAMEWORK_INC_OS_SPINLOCK_H
#define RT_FRAMEWORK_INC_OS_SPINLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include "rtfw_spinlock_internal.h"

/*! Spinlock declare*/
#define DeclareSpinlock(section_const, section_var, spin_id)    \
	RTFW_LINK_ENTRY_DECLARE(section_var, os_obj, os_spin_inst_t, spin_id);    \
	RTFW_LINK_ENTRY_DECLARE(section_const, os_spinlock, const os_spinlock_cfg_t, spin_id)

#define SPINLOCKCFGVAR(spin_id) RTFW_LINK_ENTRY_NAME(os_spinlock, spin_id)
#define SPINLOCKOBJVAR(spin_id) RTFW_LINK_ENTRY_NAME(os_obj, spin_id)

#ifdef __cplusplus
}
#endif

#endif /*  RT_FRAMEWORK_INC_OS_SPINLOCK_H */
