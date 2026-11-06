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

#ifndef RT_FRAMEWORK_INC_OS_ISR_INTERNAL_H
#define RT_FRAMEWORK_INC_OS_ISR_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_isr_types.h"
#include "rtfw_context_internal.h"
#include "rtfw_counter_internal.h"
#include "rtfw_list.h"
#include "Compiler.h"

/* Global Macro Definition */

/* 通过 os_isr_cfg_t(isr_cfg) 获取各种 object ID */
#define OS_ISR_GET_OWNER_APPID(isr_cfg) (OS_APPLICATION_GET_APPID(OS_ISR_GET_APPONRWECFG_REF(isr_cfg)))
#define OS_ISR_GET_OWNER_COREID(isr_cfg) (OS_CORE_GET_COREID(OS_ISR_GET_CORE_REF(isr_cfg)))

/* 通过 os_isr_cfg_t(isr_cfg) 获取 os_arch_isr_cfg_t(arch_isr_cfg) 的内部信息 */
#define OS_ISR_GET_LEVEL(isr_cfg) ((isr_cfg)->arch_isr_cfg.level)
#define OS_ISR_GET_VECTORNUM(isr_cfg) ((isr_cfg)->arch_isr_cfg.vector_num)
#define OS_ISR_GET_HWHANDLER(isr_cfg) ((isr_cfg)->arch_isr_cfg.hw_handler)

/* 通过 os_isr_cfg_t(isr_cfg) 获取 (isr_cfg) 的内部信息 */
#define OS_ISR_GET_CONTEXT_REF(isr_cfg) (&((isr_cfg)->context))
#define OS_ISR_GET_ARCHISRCFG_REF(isr_cfg) (&((isr_cfg)->arch_isr_cfg))
#define OS_ISR_GET_ISR_TYPE(isr_cfg) ((isr_cfg)->type)
#define OS_ISR_GET_ISRID(isr_cfg) ((isr_cfg)->isr_id)
#define OS_ISR_GET_ISENABLEDONINIT(isr_cfg) ((isr_cfg)->is_enabled_on_init)

/* 通过 os_isr_cfg_t(isr_cfg) 获取 os_isr_inst_t(IsrInst) 的内部信息 */
#define OS_ISR_GET_TOPOFSTACK(isr_cfg) ((OS_ISR_GET_INSTANCE_REF(isr_cfg))->top_of_stack)

/* 通过 os_isr_inst_t(IsrInst) 获取 (IsrInst) 的内部信息 */
#define OS_ISRINST_GET_TOPOFSTACK(IsrInst) ((IsrInst)->top_of_stack)

/* 通过 os_isr_cfg_t(isr_cfg) 获取 (isr_cfg)->context 的内部信息 */
#define OS_ISR_GET_INSTANCE_REF(isr_cfg) ((os_isr_inst_t *)((isr_cfg)->context.instance))
#define OS_ISR_GET_OBJNAME(isr_cfg) ((isr_cfg)->context.instance_name)
#define OS_ISR_GET_CONTEXTINST_REF(isr_cfg) ((isr_cfg)->context.ctx_instance)
#define OS_ISR_GET_CORE_REF(isr_cfg) ((isr_cfg)->context.core)
#define OS_ISR_GET_ARCHCONTEXTCFG_REF(isr_cfg) (&((isr_cfg)->context.arch_ctx_cfg))
#define OS_ISR_GET_APPONRWECFG_REF(isr_cfg) ((isr_cfg)->context.app_owner_cfg)
#define OS_ISR_GET_ACCESSINGAPPLMASK(isr_cfg) ((isr_cfg)->context.access_app_mask)

#define OS_ISR_GET_INITCONTEXTID(isr_cfg) ((isr_cfg)->context.initial_ctx_id)
#define OS_ISR_GET_USEFPU(isr_cfg) ((isr_cfg)->context.use_fpu)

/* 通过 os_isr_cfg_t(isr_cfg) 获取 (isr_cfg)->context.ctx_instance 的内部信息 */
#define OS_ISR_GET_CURRENT_CONTEXTID(isr_cfg) (OS_ISR_GET_CONTEXTINST_REF(isr_cfg)->current_ctx_id)

#define OS_ISR_GET_LOCKLIST_REF(isr_cfg) (&(OS_ISR_GET_CONTEXTINST_REF(isr_cfg)->lock_list))

/* 通过 os_isr_cfg_t(isr_cfg) 获取 (isr_cfg)  的内部信息 */
#define OS_ISR_GET_STACKADDR(isr_cfg) ((OS_ISR_GET_ARCHCONTEXTCFG_REF(isr_cfg))->stack_addr)
#define OS_ISR_GET_STACKSIZE(isr_cfg) ((OS_ISR_GET_ARCHCONTEXTCFG_REF(isr_cfg))->stack_size)
#define OS_ISR_GET_ENTRYADDR(isr_cfg) ((OS_ISR_GET_ARCHCONTEXTCFG_REF(isr_cfg))->entry_addr)
#define OS_ISR_GET_EXITADDR(isr_cfg) ((OS_ISR_GET_ARCHCONTEXTCFG_REF(isr_cfg))->exit_addr)
#define OS_ISR_GET_ENTRYPARAM(isr_cfg) ((OS_ISR_GET_ARCHCONTEXTCFG_REF(isr_cfg))->entry_param)

/* 通过 isr_id_t(isr_id) 获取 os_isr_cfg_t(isr_cfg) */
#define OS_ISR_GET_ISRREF_BY_ID(isr_id) (os_isr_cfg_ref_table[isr_id])
#define GET_APP_CFG_BY_ISRREF_ID(isr_id) ((os_isr_cfg_ref_table[isr_id])->context.app_owner_cfg)

/* Global Enumeration Definition */
typedef enum {
	ISR_CAT0 = 0, /* category 1*/
	ISR_CAT1, /* category 1*/
	ISR_CAT2, /* category 2*/
	ISR_TP /* category 1*/
} isr_type_t;

/* Global Type & Structure Definition */
/*! ISR实例描述结构体 os_isr_inst_t */
struct os_isr_inst {
	os_ctx_inst_t ctx_instance; /*! Context instance. Must be the first member !!! */
	/* 32 bit */
	volatile StackType_t *top_of_stack; /*! this must come first!!*/

	/* enum */

	/* 16 bit */

	/* 8 bit */
};

/*! ISR用户配置结构体 os_isr_cfg_t */
struct os_isr_cfg_s {
	/* 32 bit */
	os_ctx_cfg_t context; /*! Context configuration. Must be the first member !!! */
	os_arch_isr_cfg_t arch_isr_cfg; /*! Architecture ISR configuration */

	/* enum */
	isr_type_t type; /*! INTERRUPT TYPE */

	/* 16 bit */
	isr_id_t isr_id; /*! ISR标识, 逻辑 ID */

	/* 8 bit */
	boolean is_enabled_on_init; /*! Defines whether or not the ISR shall be enabled on initialization */
};

struct os_counter_isr_cfg_s {
	os_isr_cfg_t isr_cfg;
	os_counter_cfg_t *counter;
};

/* Global Variable Definition */
DeclareRtfwRefTable_H(isr)

/* Global Function Declaration */

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

/*!  \brief         ISR初始化
 *  \details        将ISR描述结构体中的state初始化为0.
 *
 *  \param[in]      os_isr_cfg_table   ISR参数结构体指针的存放首地址
 *  \param[in]      size            ISR个数
 *  \param[out]     None
 *
 *  \return         None
 *
 *  \context        ANY
 *
 *  \reentrant      TRUE
 *
 *  \synchronous    FALSE
 */
FUNC(void, OS_CODE) rtfw_isr_init(const os_isr_cfg_t *const os_isr_cfg_table[], OsObjIndexType size);
#ifdef CONFIG_IRQ_USE_ARCH_DISPATCH
FUNC(uintptr_t, OS_CODE) rtfw_irq_handler(int irq, void *regs, void *arg);
#else
FUNC(int, OS_CODE) rtfw_irq_handler(int irq, void *regs, void *arg);
#endif
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_INC_OS_ISR_INTERNAL_H */
