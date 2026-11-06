# ##############################################################################
# Copyright (c) 2025 Li Auto Inc. and its affiliates
# Licensed under the Apache License, Version 2.0(the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ##############################################################################

if(CONFIG_RT_FRAMEWORK)
  set(CSRCS src/rtfw.c
            src/rtfw_task.c
            src/rtfw_thread.c
            src/rtfw_isr.c
            src/rtfw_interrupt.c
            src/rtfw_event.c
            src/rtfw_core.c
            src/rtfw_wrapper.c
            ${GENDATA_DIR}/Os/core_cfg.c
            ${GENDATA_DIR}/Os/task_cfg.c
            ${GENDATA_DIR}/Os/isr_cfg.c)

  if (CONFIG_RT_FRAMEWORK_MULTIAPP)
    list(APPEND CSRCS src/rtfw_application.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/application_cfg.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_SPINLOCK)
    list(APPEND CSRCS src/rtfw_spinlock.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/spinlock_cfg.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_COUNTER)
    list(APPEND CSRCS src/rtfw_counter.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/counter_cfg.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_SYSCALL)
    list(APPEND CSRCS src/rtfw_syscall.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_IPICALL)
    list(APPEND CSRCS src/rtfw_ipicall.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/ipicall_cfg.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_IOC)
    list(APPEND CSRCS src/rtfw_ioc.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/ioc.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/ioc_cfg.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_SCHTBL)
    list(APPEND CSRCS src/rtfw_schedtable.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/schedtable_cfg.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_RESOURCE)
    list(APPEND CSRCS src/rtfw_resource.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/resource_cfg.c)
  endif()
  if (CONFIG_RT_FRAMEWORK_MEMPOOL)
    list(APPEND CSRCS src/rtfw_mempool.c)
    list(APPEND CSRCS ${GENDATA_DIR}/MemPool/mempool_cfg.c)
  endif ()

  if (CONFIG_RT_FRAMEWORK_HOOK)
    list(APPEND CSRCS src/rtfw_hook.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/hook_cfg.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_ERROR)
    list(APPEND CSRCS src/rtfw_error.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_ISOLATE)
    list(APPEND CSRCS src/rtfw_isolate.c)
    list(APPEND CSRCS ${GENDATA_DIR}/Isolate/isolate_ppc_cfg.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_MEMORY_PROTECTION)
    list(APPEND CSRCS ${GENDATA_DIR}/Os/memory_protection_cfg.c)
  endif()

  if (CONFIG_RT_FRAMEWORK_HAVE_TRAP_HANDLER)
    list(APPEND CSRCS src/rtfw_trap.c)
  endif()

  set(INCLUDE_DIRS ${NUTTX_DIR}/sched
                   ${GENDATA_DIR}/Rte
                   ${CMAKE_CURRENT_LIST_DIR}/src
                   ${CMAKE_CURRENT_LIST_DIR}/inc
                   ${NUTTX_APPS_DIR}/components/include
                   ${NUTTX_APPS_DIR}/components/include/autosar)

  list(APPEND INCLUDE_DIRS ${NUTTX_APPS_DIR}/${APPS_DIR}/apps)

list(APPEND INCLUDE_DIRS ${GENDATA_DIR}/Os
                         ${GENDATA_DIR}/Memmap
                         ${GENDATA_DIR}/Isolate)

if (CONFIG_RT_FRAMEWORK_MEMPOOL)
  list(APPEND INCLUDE_DIRS ${GENDATA_DIR}/MemPool)
endif ()

  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    if(CONFIG_TRICORE_TOOLCHAIN_HIGHTEC)
      message("Test info rt_framework compiler option!")
      add_compile_options(-Wextra -pedantic -Wstrict-prototypes)
    else()
	    add_compile_options(-Werror -Wextra -pedantic -Wstrict-prototypes)
    endif()
  endif()
  nuttx_add_library(rt_framework STATIC)
  target_sources(rt_framework PRIVATE ${CSRCS})
  target_include_directories(rt_framework PRIVATE ${INCLUDE_DIRS} ${VENDOR_THIRD_PARTY_DRIVER_INCDIR})

  if (CONFIG_IRQ_USE_ARCH_DISPATCH)
    if(CONFIG_ARCH_TRICORE)
      target_sources(nuttx PRIVATE ${GENDATA_DIR}/Os/tricore_irq_dispatch.c)
      target_include_directories(nuttx PRIVATE ${INCLUDE_DIRS} )
    endif()
  endif()
endif()
