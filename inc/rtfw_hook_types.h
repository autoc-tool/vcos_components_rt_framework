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

#ifndef RT_FRAMEWORK_INC_OS_HOOK_TYPES_H
#define RT_FRAMEWORK_INC_OS_HOOK_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#define OS_ERRORHOOK_HOOKNESTINGMASK                                                                                   \
	((os_context_id_t)((OS_STARTUPHOOK_CONTEXT_ID) | (OS_IOCCALLBACKHOOK_CONTEXT_ID)))

#define OS_PROTECTIONHOOK_HOOKNESTINGMASK                                                                              \
	((os_context_id_t)((OS_ERRORHOOK_CONTEXT_ID) | (OS_STARTUPHOOK_CONTEXT_ID) | (OS_IOCCALLBACKHOOK_CONTEXT_ID)))

#define OS_STARTUPHOOK_HOOKNESTINGMASK ((os_context_id_t)0)

#define OS_SHUTDOWNHOOK_HOOKNESTINGMASK                                                                                \
	((os_context_id_t)((OS_ERRORHOOK_CONTEXT_ID) | (OS_PROTECTIONHOOK_CONTEXT_ID) |                                    \
					 (OS_STARTUPHOOK_CONTEXT_ID) | (OS_IOCCALLBACKHOOK_CONTEXT_ID)))

#define OS_IOCCALLBACKHOOK_HOOKNESTINGMASK                                                                             \
	((os_context_id_t)((OS_ERRORHOOK_CONTEXT_ID) | (OS_PROTECTIONHOOK_CONTEXT_ID) |                                    \
					 (OS_STARTUPHOOK_CONTEXT_ID) | (OS_SHUTDOWNHOOK_CONTEXT_ID)))

struct os_hook_inst_s;
typedef struct os_hook_inst_s os_hook_inst_t;

struct os_hook_cfg_s;
typedef struct os_hook_cfg_s os_hook_cfg_t;

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_HOOK_TYPES_H */
