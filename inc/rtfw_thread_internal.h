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
#ifndef RT_FRAMEWORK_INC_THREAD_H
#define RT_FRAMEWORK_INC_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <nuttx/irq.h>
#include <setjmp.h>
#include <nuttx/semaphore.h>
#include <nuttx/event.h>
#include <nuttx/sched.h>
#include "rtfw_types.h"
#include "rtfw_thread_types.h"

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
#include "rtfw_syscall_internal.h"
#endif

struct os_task_thread_inst_s {
	jmp_buf jmpbuf;
	struct tcb_s tcb;
	sem_t act_sem;
};

struct os_hook_thread_inst_s {
	jmp_buf jmpbuf;
	struct tcb_s tcb;
};

#if defined(CONFIG_RT_FRAMEWORK_SYSCALL) && (CONFIG_RT_FRAMEWORK_SYSCALL == 1)
static inline_function void os_set_thread_mode_to_privilege(void)
{
	syscall_data_t syscall;

	if (TRUE != up_is_privilege_mode()) {
		syscall.syscall_id = OS_SYSCALLID_SET_USER_TO_PRIVILEGE;
		up_syscall(&syscall);
	}
}
static inline_function void os_set_thread_mode_from_privilege(bool is_privilege)
{
	if (TRUE != is_privilege) {
		up_set_privilege_to_user();
	}
}
#endif

#ifdef __cplusplus
}
#endif

#endif /*  RT_FRAMEWORK_INC_THREAD_H */
