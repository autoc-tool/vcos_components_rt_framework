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

#ifndef RT_FRAMEWORK_INC_OS_CPUSET_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_CPUSET_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_atomic.h"

/* Global Macro Definition */
#define OS_CPU_ENABLE_GET() (OSCpuEnabled)
#define OS_CPU_START_GET() (OSCpuStarted)

#define CPUSET_SET(cpuset, n) (void)Os_AtomicOr((Atomic *)&(cpuset), (1U << (n)))
#define CPUSET_CLR(cpuset, n) (void)Os_AtomicAnd((Atomic *)&(cpuset), (~(1U << (n))))
#define CPUSET_ZERO(cpuset) (void)Os_AtomicAnd((Atomic *)&(cpuset), 0)

#define CPUSET_ISSET(cpuset, n) ((((cpuset) & (1U << (n))) != 0U) ? TRUE : FALSE)
#define CPUSET_ISZERO(cpuset) (((cpuset) == 0) ? TRUE : FALSE)

#define OS_IS_CPU_ENABLED(index) (CPUSET_ISSET(OS_CPU_ENABLE_GET(), (index)))
#define OS_IS_CPU_STARTED(index) (CPUSET_ISSET(OS_CPU_START_GET(), (index)))

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_CPUSET_INTERNAL_H */
