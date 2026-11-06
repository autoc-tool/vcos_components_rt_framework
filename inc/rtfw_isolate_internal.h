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
#ifndef RT_FRAMEWORK_INC_ISOLATE_INTERNAL_H
#define RT_FRAMEWORK_INC_ISOLATE_INTERNAL_H

#include <nuttx/config.h>
#include "rtfw_application.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISOLATE_PPC_FLAG_CALL_ENTER       (0u)
#define ISOLATE_PPC_FLAG_CALL_EXIT        (1u)

struct os_isolate_ppc_context_s {
	const struct mpu_access_right_s *access_right;
	const struct mpu_access_right_s *target_access_right;

	const struct os_app_cfg_s *app_cfg;

	boolean is_privilege;
	boolean target_is_privilege;

	uint8 set_num;
	uint8 target_set_num;

};

extern const uint8 *const ppc_func_config[];

#ifdef __cplusplus
}
#endif

#endif /*  RT_FRAMEWORK_INC_ISOLATE_INTERNAL_H */
