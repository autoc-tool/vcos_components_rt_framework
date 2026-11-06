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
#ifndef RT_FRAMEWORK_INC_ISOLATE_H
#define RT_FRAMEWORK_INC_ISOLATE_H

#include "rtfw_isolate_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(boolean, OS_CODE) isolate_ppc_is_needed(const uint32_t func_id);

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /*  RT_FRAMEWORK_INC_ISOLATE_H */
