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

#ifndef __OS_INTERRUPT_TYPES_H
#define __OS_INTERRUPT_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"

typedef uint16_t os_int_api_cnt_t;

#define INT_DISABLE_ALL_STATE_DISABLED    ((uint8) 1u)
#define INT_DISABLE_ALL_STATE_ENABLED    ((uint8) 0u)

typedef struct {
	irqlevel_t int_level_state_cat1;
	irqlevel_t int_level_state_cat2;

	os_int_api_cnt_t int_suspend_cat1_cnt;
	os_int_api_cnt_t int_suspend_cat2_cnt;

	os_int_api_cnt_t int_suspend_cat1_record;
	os_int_api_cnt_t int_suspend_cat2_record;

	uint8 int_disable_all_state;
} os_int_state_t;

#ifdef __cplusplus
}
#endif

#endif /* __OS_INTERRUPT_TYPES_H */
