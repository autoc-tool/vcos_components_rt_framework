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

/* Including File */
#include <nuttx/config.h>
#include <nuttx/spinlock.h>

#include "string.h"
#include "rtfw_counter.h"
#include "rtfw_core.h"
#include "rtfw_task.h"
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
#include "rtfw_hook.h"
#endif

#include "rtfw_ipicall.h"
#include "rtfw_event.h"

#if defined(CONFIG_RT_FRAMEWORK_MULTIAPP) && (CONFIG_RT_FRAMEWORK_MULTIAPP == 1)
#include "rtfw_application.h"
#include "application_cfg.h"
#endif

#define OS_IPICALL_ACTIVATETASK ((os_ipicall_service_id_t)0)
#define OS_IPICALL_ACTIVATETASKASYN ((os_ipicall_service_id_t)1)
#define OS_IPICALL_GETAPPLICATIONSTATE ((os_ipicall_service_id_t)2)
#define OS_IPICALL_GETEVENT ((os_ipicall_service_id_t)3)
#define OS_IPICALL_SETEVENT ((os_ipicall_service_id_t)4)
#define OS_IPICALL_SETEVENTASYN ((os_ipicall_service_id_t)5)
#define OS_IPICALL_HOOK_CALL_VOID_HOOK ((os_ipicall_service_id_t)6)
#define OS_IPICALL_START_SCHEDULE_TABLE_REL ((os_ipicall_service_id_t)7)
#define OS_IPICALL_START_SCHEDULE_TABLE_ABS ((os_ipicall_service_id_t)8)
#define OS_IPICALL_STOP_SCHEDULE_TABLE ((os_ipicall_service_id_t)9)
#define OS_IPICALL_GET_SCHEDULE_TABLE_STATUS ((os_ipicall_service_id_t)10)
#define OS_IPICALL_NEXT_SCHEDULE_TABLE ((os_ipicall_service_id_t)11)
#define OS_IPICALL_NEXT_SCHEDULE_TABLE_FROM_STOP ((os_ipicall_service_id_t)12)
#define OS_IPICALL_START_SCHEDULE_TABLE_SYN ((os_ipicall_service_id_t)13)
#define OS_IPICALL_SYNC_SCHEDULE_TABLE ((os_ipicall_service_id_t)14)
#define OS_IPICALL_SET_SCHEDULE_TABLE_ASYNC ((os_ipicall_service_id_t)15)
#define OS_IPICALL_DEVIATE_SCHEDULE_TABLE ((os_ipicall_service_id_t)16)
#define OS_IPICALL_SET_SCHEDULE_TABLE_APP_ID ((os_ipicall_service_id_t)17)
#define OS_IPICALL_CLEAR_SCHEDULE_TABLE_APP_ID ((os_ipicall_service_id_t)18)
#define OS_IPICALL_GETTASKSTATE ((os_ipicall_service_id_t)19)
#if defined(CONFIG_RT_FRAMEWORK_IPICALL_TEST) && (CONFIG_RT_FRAMEWORK_IPICALL_TEST == 1)
#define OS_IPICALL_IPC_TEST_FUNCTION ((os_ipicall_service_id_t)20)
#define OS_IPICALL_SHUTDOWN_ALL_CORES ((os_ipicall_service_id_t)21)
#define OS_IPICALL_SERVICE_MAX ((os_ipicall_service_id_t)22)
#else
#define OS_IPICALL_SHUTDOWN_ALL_CORES ((os_ipicall_service_id_t)20)
#define OS_IPICALL_SERVICE_MAX ((os_ipicall_service_id_t)21)
#endif

#define OS_START_SEC_CONST
#include "rtfw_memmap.h"

CONST(os_ipicall_service_t, OS_CONST)
os_ipicall_service_table[OS_IPICALL_SERVICE_MAX] = {
	[OS_IPICALL_ACTIVATETASK] = &os_ipicall_recv_activate_task,
	[OS_IPICALL_ACTIVATETASKASYN] = &os_ipicall_recv_activate_task_asyn,
	[OS_IPICALL_GETAPPLICATIONSTATE] = &os_ipicall_recv_get_application_state,
	[OS_IPICALL_GETEVENT] = &os_ipicall_recv_get_event,
	[OS_IPICALL_SETEVENT] = &os_ipicall_recv_set_event,
	[OS_IPICALL_SETEVENTASYN] = &os_ipicall_recv_set_event_asyn,
#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
	[OS_IPICALL_HOOK_CALL_VOID_HOOK] = &os_ipicall_recv_hook_call_void_hook,
#endif
#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
	[OS_IPICALL_START_SCHEDULE_TABLE_REL] = &os_ipicall_recv_start_scheduletable_rel,
	[OS_IPICALL_START_SCHEDULE_TABLE_ABS] = &os_ipicall_recv_start_scheduletable_abs,
	[OS_IPICALL_STOP_SCHEDULE_TABLE] = &os_ipicall_recv_stop_scheduletable,
	[OS_IPICALL_GET_SCHEDULE_TABLE_STATUS] = &os_ipicall_recv_get_scheduletable_status,
	[OS_IPICALL_NEXT_SCHEDULE_TABLE] = &os_ipicall_recv_next_scheduletable,
	[OS_IPICALL_NEXT_SCHEDULE_TABLE_FROM_STOP] = &os_ipicall_recv_next_scheduletable_from_stop,
	[OS_IPICALL_START_SCHEDULE_TABLE_SYN] = &os_ipicall_recv_start_scheduletable_synchron,
	[OS_IPICALL_SYNC_SCHEDULE_TABLE] = &os_ipicall_recv_sync_scheduletable,
	[OS_IPICALL_SET_SCHEDULE_TABLE_ASYNC] = &os_ipicall_recv_set_scheduletable_async,
	[OS_IPICALL_DEVIATE_SCHEDULE_TABLE] = &os_ipicall_recv_deviate_schedule_table,
	[OS_IPICALL_SET_SCHEDULE_TABLE_APP_ID] = &os_ipicall_recv_set_schedule_table_app_id,
	[OS_IPICALL_CLEAR_SCHEDULE_TABLE_APP_ID] = &os_ipicall_recv_clear_schedule_table_app_id,
#endif
	[OS_IPICALL_GETTASKSTATE] = &os_ipicall_recv_get_task_state,
#if defined(CONFIG_RT_FRAMEWORK_IPICALL_TEST) && (CONFIG_RT_FRAMEWORK_IPICALL_TEST == 1)
	[OS_IPICALL_IPC_TEST_FUNCTION] = &os_ipicall_recv_ipc_test_function,
#endif
	[OS_IPICALL_SHUTDOWN_ALL_CORES] = &os_ipicall_recv_shutdown_all_cores,
};

#define OS_STOP_SEC_CONST
#include "rtfw_memmap.h"

static inline os_ipicall_index_t next_buffer_index(os_ipicall_index_t current_index)
{
	os_ipicall_index_t next_index = current_index + 1U;

	if (next_index >= (os_ipicall_index_t)QUEUE_DEPTH) {
		next_index = 0U;
	}
	return next_index;
}

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

static FUNC(void, OS_CODE) os_ipicall_isr_handler(void);

/* Local Function Implementation */

/* global Function Declaration */

/* 核间调用发送接口 */
static inline os_status_t os_ipicall_send(uint32 target_core, os_ipicall_data_t *data)
{
	os_status_t ipicall_send_status;
#if defined(CONFIG_RT_FRAMEWORK_IPICALL_SYNC) && (CONFIG_RT_FRAMEWORK_IPICALL_SYNC == 1)
	ipicall_send_status = os_ipicall_send_sync(target_core, data);
#else
	ipicall_send_status = os_ipicall_send_async(target_core, data);
#endif /* CONFIG_RT_FRAMEWORK_IPICALL_SYNC */

	return ipicall_send_status;
}

static inline os_status_t os_ipicall_get_send_cfg(uint32 target_core, const os_ipicall_cfg_t **ipicall_cfg)
{
	os_status_t status = OS_STATUS_NOT_OK;
	const os_core_cfg_t *core_cfg = OS_CORE_CFG_GET(os_get_core_id());

	if (target_core < (uint32)CONFIG_RT_FRAMEWORK_NCORES) {
		*ipicall_cfg = core_cfg->ipicall_refs[target_core];
		if (*ipicall_cfg == NULL_PTR) {
			status = OS_STATUS_IPICALL_UNAVAILABLE;
		} else {
			status = OS_STATUS_OK;
		}
	} else {
		status = OS_STATUS_IPICALL_UNAVAILABLE;
	}
	return status;
}

static inline os_status_t os_ipicall_get_send_buffer(const volatile os_ipicall_cfg_t *ipicall_cfg,
						       os_ipicall_data_t *data)
{
	os_status_t status = OS_STATUS_NOT_OK;
	os_ipicall_index_t current_index;
	os_ipicall_index_t next_index;

	current_index = ipicall_cfg->instance->send_idx;
	while ((next_index = next_buffer_index(current_index)) != ipicall_cfg->instance->receive_idx) {
		if (ipicall_cfg->instance->send_buffer[current_index].ack == OS_IPICALL_ACK) {
			data->index = current_index;
			status = OS_STATUS_OK;
			break;
		}
		current_index = next_index;
	}

	return status;
}

static inline boolean os_ipicall_new_request_available(const volatile os_ipicall_cfg_t *ipicall_cfg)
{
	return (ipicall_cfg->instance->receive_idx != ipicall_cfg->instance->send_idx);
}

static inline boolean os_ipicall_request_handled(const volatile os_ipicall_cfg_t *ipicall_cfg,
						 const os_ipicall_data_t *data)
{
	return (ipicall_cfg->instance->send_buffer[data->index].ack == OS_IPICALL_SYNC_PENDING);
}

/* global Function Declaration */
FUNC(void, OS_CODE) os_ipicall_init(const os_ipicall_cfg_t *const os_ipicall_cfg_table[], OsObjIndexType count)
{
	if ((os_ipicall_cfg_table == NULL_PTR) || (count == (OsObjIndexType)0U)) {
		return;
	}
	const os_ipicall_cfg_t *ipicall_cfg;
	os_ipicall_inst_t *ipicall_inst;
	OsObjIndexType cfg_idx, buffer_idx;
	CoreIdType core_id = os_get_core_id();

	for (cfg_idx = 0U; cfg_idx < count; cfg_idx++) {
		ipicall_cfg = os_ipicall_cfg_table[cfg_idx];
		if (ipicall_cfg == NULL_PTR) {
			continue;
		} else {
			ipicall_inst = ipicall_cfg->instance;
			if (ipicall_inst == NULL_PTR) {
				continue;
			} else {
				if (ipicall_cfg->src_core_id == core_id) {
					ipicall_inst->send_idx = 0U;
					for (buffer_idx = 0U; buffer_idx < QUEUE_DEPTH; buffer_idx++) {
						ipicall_inst->send_buffer[buffer_idx].ack = OS_IPICALL_ACK;
					}
				}
				if (ipicall_cfg->target_core_id == core_id) {
					ipicall_inst->receive_idx = 0U;
				}
			}
		}
	}
	/* attach ipi callback */
	up_ipicall_attach((void *)(uintptr_t)os_ipicall_isr_handler);
}

/* Internal implementation of inter-core call sending interface */
static FUNC(os_status_t, OS_CODE) os_ipicall_send_internal(const volatile os_ipicall_cfg_t *ipicall_cfg,
							     os_ipicall_data_t *data, os_ipicall_cast_mode broad_cast)
{
	os_status_t status = OS_STATUS_NOT_OK;
	irqlevel_t int_state;

	os_int_suspend_kernel(&int_state);
	{
		/* Obtain an available buffer slot. */
		status = os_ipicall_get_send_buffer(ipicall_cfg, data);
		if (status != OS_STATUS_OK) {
			status = OS_STATUS_IPICALL_OVERFLOW;
		} else {
			ipicall_cfg->instance->send_buffer[data->index].service = data->send_element.service;
			ipicall_cfg->instance->send_buffer[data->index].in_param = data->send_element.in_param;
			ipicall_cfg->instance->send_buffer[data->index].send_app_id = data->send_element.send_app_id;
			ipicall_cfg->instance->send_buffer[data->index].ack = data->send_element.ack;
			SP_DSB();

			ipicall_cfg->instance->send_idx = next_buffer_index(data->index);
			SP_DSB();

			/* trigger target core interrupt */
			if (broad_cast == OS_IPICALL_UNICAST) {
				status = (os_status_t)up_trigger_ipicall((CoreIdType)(ipicall_cfg->target_core_id));
			}
		}
	}
	os_int_resume_kernel(&int_state);
	return status;
}

/* synchronous inter-processor function call */
FUNC(os_status_t, OS_CODE) os_ipicall_send_sync(uint32 target_core, os_ipicall_data_t *data)
{
	os_status_t status = OS_STATUS_NOT_OK;
	const os_ipicall_cfg_t *send_cfg = NULL_PTR;
	ApplicationIDType app_id = 0;

	if (data == NULL_PTR) {
		status = OS_STATUS_PARAM_POINTER_2;
	} else {
		if (OS_STATUS_OK != get_task_app_id(&app_id)) {
			status = OS_STATUS_ID_1;
		} else {
			if (app_id > (ApplicationIDType)OS_APPLICATION_CNT) {
				status = OS_STATUS_ID_1;
			} else {
				data->send_element.send_app_id = app_id;
				data->send_element.ack = OS_IPICALL_SYNC_NACK;

				/* get communication channel from static configs */
				status = os_ipicall_get_send_cfg(target_core, &send_cfg);
				if (status != OS_STATUS_OK) {
					status = OS_STATUS_IPICALL_UNAVAILABLE;
				} else {
					/* send(copy) the info to the target core */
					status = os_ipicall_send_internal(send_cfg, data, OS_IPICALL_UNICAST);
					if (status == OS_STATUS_OK) {
						/* Polling the results */
						while (os_ipicall_request_handled(send_cfg, data) != TRUE) {
							/* busy loop. can be preemptive by high priority task */
						}
						/* copy results from sharebuffer to private buffer */
						(void)memcpy((uint8 *)&data->ret_element,
							     &send_cfg->instance->result_buffer[data->index],
							     sizeof(os_ipicall_return_t));
						send_cfg->instance->send_buffer[data->index].ack = OS_IPICALL_ACK;
						status = data->ret_element.return_status;
					} else {
						/*! do nothing */
					}
				}
			}
		}
	}

	return status;
}

/*! broad_cast to non-local cores  */
FUNC(os_status_t, OS_CODE) os_ipicall_broad_cast(uint32 target_core_mask, os_ipicall_data_t *data)
{
	os_status_t ipicall_status = OS_STATUS_NOT_OK;
	const os_ipicall_cfg_t *send_cfg = NULL_PTR;
	ApplicationIDType app_id = 0;

	if (data == NULL_PTR) {
		ipicall_status = OS_STATUS_PARAM_POINTER_2;
	} else {
		if (OS_STATUS_OK != get_task_app_id(&app_id)) {
			ipicall_status = OS_STATUS_ID_1;
		} else {
			if (app_id > (ApplicationIDType)OS_APPLICATION_CNT) {
				ipicall_status = OS_STATUS_ID_1;
			} else {
				data->send_element.send_app_id = app_id;
				data->send_element.ack = OS_IPICALL_ASYNC_NACK;
				uint32 CoreIdx = 0U;

				for (; CoreIdx < (uint32)(CONFIG_RT_FRAMEWORK_NCORES); ++CoreIdx) {
					if (target_core_mask & ((0x00000001U << CoreIdx))) {
						/* get communication channel from static configs */
						ipicall_status = os_ipicall_get_send_cfg(CoreIdx, &send_cfg);
						if (ipicall_status != OS_STATUS_OK) {
							ipicall_status = OS_STATUS_IPICALL_UNAVAILABLE;
						} else {
							/* send(copy) the info to the target core */
							ipicall_status = os_ipicall_send_internal(send_cfg, data,
												  OS_IPICALL_BROADCAST);
						}
						if (ipicall_status != OS_STATUS_OK)
							break;
					}
				}
				up_trigger_ipicalls(target_core_mask);
			}
		}
	}
	return ipicall_status;
}

/* asynchronous inter-processor function call */
FUNC(os_status_t, OS_CODE) os_ipicall_send_async(uint32 target_core, os_ipicall_data_t *data)
{
	os_status_t status = OS_STATUS_NOT_OK;
	const os_ipicall_cfg_t *send_cfg = NULL_PTR;
	ApplicationIDType app_id = 0;

	if (data == NULL_PTR) {
		status = OS_STATUS_PARAM_POINTER_2;
	} else {
		if (OS_STATUS_OK != get_task_app_id(&app_id)) {
			status = OS_STATUS_ID_1;
		} else {
			if (app_id > (uint8)OS_APPLICATION_CNT) {
				status = OS_STATUS_ID_1;
			} else {
				data->send_element.send_app_id = app_id;
				data->send_element.ack = OS_IPICALL_ASYNC_NACK;

				/* get communication channel from static configs */
				status = os_ipicall_get_send_cfg(target_core, &send_cfg);
				if (status != OS_STATUS_OK) {
					status = OS_STATUS_IPICALL_UNAVAILABLE;
				} else {
					/* send(copy) the info to the target core */
					status = os_ipicall_send_internal(send_cfg, data, OS_IPICALL_UNICAST);
				}
			}
		}
	}

	return status;
}

/*! ipicall services handler */
static FUNC(void, OS_CODE) os_ipicall_isr_handler(void)
{
	os_status_t ipicall_status;
	const os_ipicall_cfg_t *ipicall_cfg;
	os_ipicall_inst_t *ipicall_inst;
	os_ipicall_data_t com_data;
	const os_core_cfg_t *core_cfg = OS_CORE_CFG_GET(os_get_core_id());
	os_ipicall_index_t recv_idx;
	irqlevel_t int_state;
	os_ipicall_service_t ipicall_service;
	os_ipicall_send_t *ipicall_send_ptr;
	os_ipicall_return_t *ipicall_ret_ptr;
	uint32 service_idx;

	os_int_suspend_kernel(&int_state);
	for (service_idx = (uint32)CONFIG_RT_FRAMEWORK_NCORES;
	     service_idx < (((uint32)CONFIG_RT_FRAMEWORK_NCORES) * 2U); service_idx++) {
		ipicall_cfg = core_cfg->ipicall_refs[service_idx];
		if (ipicall_cfg == NULL_PTR) {
			continue;
		}
		ipicall_inst = ipicall_cfg->instance;
		if (ipicall_inst == NULL_PTR) {
			continue;
		}
		while (os_ipicall_new_request_available(ipicall_cfg) == TRUE) {
			recv_idx = ipicall_inst->receive_idx;
			ipicall_send_ptr = &(ipicall_inst->send_buffer[recv_idx]);
			if ((ipicall_send_ptr->ack == OS_IPICALL_ASYNC_NACK) ||
				(ipicall_send_ptr->ack == OS_IPICALL_SYNC_NACK)) {
				com_data.send_element.service = ipicall_send_ptr->service;
				com_data.send_element.in_param = ipicall_send_ptr->in_param;
				com_data.send_element.send_app_id = ipicall_send_ptr->send_app_id;
				ipicall_service = ipicall_cfg->service_ref[com_data.send_element.service];

				if (com_data.send_element.service < OS_IPICALL_SERVICE_MAX) {
					ipicall_service = ipicall_cfg->service_ref[com_data.send_element.service];
					if (ipicall_service != NULL_PTR) {
						ipicall_status = ipicall_service(&com_data);
					} else {
						ipicall_status = OS_STATUS_IPICALL_UNAVAILABLE;
					}
				} else {
					ipicall_status = OS_STATUS_IPICALL_UNAVAILABLE;
				}

				if (ipicall_send_ptr->ack == OS_IPICALL_ASYNC_NACK) {
					ipicall_send_ptr->ack = OS_IPICALL_ACK;
				}

				if (ipicall_send_ptr->ack == OS_IPICALL_SYNC_NACK) {
					ipicall_ret_ptr = &(ipicall_inst->result_buffer[recv_idx]);
					ipicall_ret_ptr->service = com_data.send_element.service;
					ipicall_ret_ptr->out_param = com_data.ret_element.out_param;
					ipicall_ret_ptr->return_status = ipicall_status;
					ipicall_send_ptr->ack = OS_IPICALL_SYNC_PENDING;
				}
			}
			ipicall_inst->receive_idx = next_buffer_index(ipicall_inst->receive_idx);
		}
	}
	os_int_resume_kernel(&int_state);
}

/*! activate task belongs to other core */
FUNC(os_status_t, OS_CODE) os_ipicall_recv_activate_task(os_ipicall_data_t *param)
{
	TaskType task_id = param->send_element.in_param.avtivate_task_param.task_id;
	const os_task_cfg_t *task_cfg;
	os_task_inst_t *task_inst;
	int ret;

	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	task_cfg = os_task_cfg_ref_table[task_id];

	task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);

#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	const os_ctx_cfg_t *task_context = OS_TASK_GET_CONTEXT_REF(task_cfg);

	if (UNLIKELY(os_app_check_is_accessible(OS_CONTEXT_GET_APPONRWECFG_REF(task_context)) != OS_STATUS_OK)) {
		return OS_STATUS_NOT_ACCESSIBLE;
	}
#endif
	ret = nxsem_post(&(task_inst->thread.act_sem));
	if (ret == -EOVERFLOW) {
		return OS_STATUS_LIMIT;
	} else {
		return (ret == OK ? OS_STATUS_OK : OS_STATUS_NOT_OK);
	}
}

/*! activate task belongs to other core */
FUNC(os_status_t, OS_CODE) os_ipicall_recv_activate_task_asyn(os_ipicall_data_t *param)
{
	TaskType task_id = param->send_element.in_param.avtivate_task_param.task_id;
	const os_task_cfg_t *task_cfg;
	os_task_inst_t *task_inst;
	int ret;

	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	task_cfg = os_task_cfg_ref_table[task_id];

	task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);

#ifdef CONFIG_RT_FRAMEWORK_ACCESSCHECK
	const os_ctx_cfg_t *task_context = OS_TASK_GET_CONTEXT_REF(task_cfg);

	if (UNLIKELY(os_app_check_is_accessible(OS_CONTEXT_GET_APPONRWECFG_REF(task_context)) != OS_STATUS_OK)) {
		return OS_STATUS_NOT_ACCESSIBLE;
	}
#endif

	ret = nxsem_post(&(task_inst->thread.act_sem));
	if (ret == -EOVERFLOW) {
		return OS_STATUS_LIMIT;
	} else {
		return (ret == OK ? OS_STATUS_OK : OS_STATUS_NOT_OK);
	}
}

/*! get task state */
FUNC(os_status_t, OS_CODE) os_ipicall_recv_get_task_state(os_ipicall_data_t *param)
{
	TaskType task_id = param->send_element.in_param.get_task_state_param.task_id;
	const os_task_cfg_t *task_cfg;
	os_task_inst_t *task_inst;

	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	task_cfg = os_task_cfg_ref_table[task_id];

	task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);

	param->ret_element.out_param.get_task_state_param.task_state = os_get_task_state_internal(task_inst);

	return OS_STATUS_OK;
}

/*! get application state */
FUNC(os_status_t, OS_CODE) os_ipicall_recv_get_application_state(os_ipicall_data_t *param)
{
	ApplicationIDType app_id = param->send_element.in_param.get_app_state_param.app_id;
	const os_app_cfg_t *app_cfg;

	if (UNLIKELY(app_id >= os_app_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	app_cfg = os_app_cfg_ref_table[app_id];

	param->ret_element.out_param.get_app_state_param.app_state = app_cfg->instance->state;
	return OS_STATUS_OK;
}

/*! get event values */
FUNC(os_status_t, OS_CODE) os_ipicall_recv_get_event(os_ipicall_data_t *param)
{
	os_event_mask_t mask = 0;
	TaskType task_id = param->send_element.in_param.get_event_param.task_id;
	const os_task_cfg_t *task_cfg;
	os_task_inst_t *task_inst;

	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	task_cfg = os_task_cfg_ref_table[task_id];

	task_inst = OS_TASK_GET_INSTANCE_REF(task_cfg);

	if (UNLIKELY(task_inst->thread.tcb.flags & TCB_FLAG_THREAD_IS_SUSPENDED)) {
		return OS_STATUS_STOPPED;
	} else {
		mask = nxevent_getmask(&(task_cfg->event_hdl->event));
	}
	param->ret_element.out_param.get_event_param.mask = mask;
	return OS_STATUS_OK;
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_set_event(os_ipicall_data_t *param)
{
	TaskType task_id = param->send_element.in_param.set_event_param.task_id;
	os_event_mask_t mask = param->send_element.in_param.set_event_param.mask;

	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	return os_set_event(task_id, mask);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_set_event_asyn(os_ipicall_data_t *param)
{
	TaskType task_id = param->send_element.in_param.set_event_param.task_id;
	os_event_mask_t mask = param->send_element.in_param.set_event_param.mask;

	if (UNLIKELY(task_id >= os_task_cfg_ref_table_size)) {
		return OS_STATUS_ID_1;
	}

	return os_set_event_asyn(task_id, mask);
}

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
FUNC(os_status_t, OS_CODE) os_ipicall_recv_hook_call_void_hook(os_ipicall_data_t *param)
{
	return os_hook_call_void_hook(param->send_element.in_param.hook_call_void_hook_param.hook_id);
}
#endif

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
FUNC(os_status_t, OS_CODE) os_ipicall_recv_start_scheduletable_rel(os_ipicall_data_t *param)
{
	return os_start_schedule_table_rel(param->send_element.in_param.start_schtbl_rel_param.schedtable_id,
					   param->send_element.in_param.start_schtbl_rel_param.offset);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_start_scheduletable_abs(os_ipicall_data_t *param)
{
	return os_start_schedule_table_abs(param->send_element.in_param.start_schtbl_abs_param.schedtable_id,
					   param->send_element.in_param.start_schtbl_abs_param.start);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_stop_scheduletable(os_ipicall_data_t *param)
{
	return os_stop_schedule_table(param->send_element.in_param.stop_schtbl_param.schedtable_id);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_get_scheduletable_status(os_ipicall_data_t *param)
{
	ScheduleTableStatusType schedtable_status;
	os_status_t RecvGetScheduleTableStatus;
	ScheduleTableType schedtable_id;

	schedtable_id = param->send_element.in_param.get_schtbl_status_param.schedtable_id;
	RecvGetScheduleTableStatus = os_get_schedule_table_status(schedtable_id, &schedtable_status);
	param->ret_element.out_param.get_schtbl_status_param.schedtable_status = schedtable_status;
	(void)schedtable_status;
	return RecvGetScheduleTableStatus;
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_next_scheduletable(os_ipicall_data_t *param)
{
	return os_next_schedule_table(param->send_element.in_param.next_schtbl_param.curr_schedtable_id,
				      param->send_element.in_param.next_schtbl_param.next_schedtable_id);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_next_scheduletable_from_stop(os_ipicall_data_t *param)
{
	return os_next_schedule_table_from_stop(
		param->send_element.in_param.next_schtbl_fromstop_param.curr_schedtable_id,
		param->send_element.in_param.next_schtbl_fromstop_param.next_schedtable_id);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_start_scheduletable_synchron(os_ipicall_data_t *param)
{
	return os_start_schedule_table_synchron(param->send_element.in_param.start_schtbl_synchron_param.schedtable_id);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_sync_scheduletable(os_ipicall_data_t *param)
{
	return os_sync_schedule_table(param->send_element.in_param.sync_schtbl_param.schedtable_id,
				      param->send_element.in_param.sync_schtbl_param.tick_value);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_set_scheduletable_async(os_ipicall_data_t *param)
{
	return os_set_schedule_table_async(param->send_element.in_param.set_schtbl_async_param.schedtable_id);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_deviate_schedule_table(os_ipicall_data_t *param)
{
	return os_deviate_schedule_table(param->send_element.in_param.deviate_schtbl_param.schedtable_id,
					 param->send_element.in_param.deviate_schtbl_param.deviate_value);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_set_schedule_table_app_id(os_ipicall_data_t *param)
{
	return os_set_schedule_table_app_id(param->send_element.in_param.set_schtbl_appid_param.schedtable_id,
					    param->send_element.in_param.set_schtbl_appid_param.app_id);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_clear_schedule_table_app_id(os_ipicall_data_t *param)
{
	return os_clear_schedule_table_app_id(param->send_element.in_param.clear_schtbl_appid_param.schedtable_id,
					      param->send_element.in_param.clear_schtbl_appid_param.app_id);
}

#endif

/*! ActivateTask */
FUNC(os_status_t, OS_CODE) os_ipicall_activate_task(uint32 target_core, TaskType task_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_ACTIVATETASK;
	param.send_element.in_param.avtivate_task_param.task_id = task_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

/*! ActivateTaskAsync */
FUNC(os_status_t, OS_CODE) os_ipicall_activate_task_asyn(uint32 target_core, TaskType task_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_ACTIVATETASKASYN;
	param.send_element.in_param.avtivate_task_param.task_id = task_id;

	return os_ipicall_send_async(target_core, &param);
}

FUNC(os_status_t, OS_CODE) os_ipicall_get_task_state(uint32 target_core, TaskType task_id, TaskStateRefType state_ref)
{
	os_status_t status;

	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_GETTASKSTATE;
	param.send_element.in_param.get_task_state_param.task_id = task_id;

	status = os_ipicall_send_sync(target_core, &param);

	*state_ref = param.ret_element.out_param.get_task_state_param.task_state;

	return status;
}

#if defined(CONFIG_RT_FRAMEWORK_SCHTBL) && (CONFIG_RT_FRAMEWORK_SCHTBL == 1)
FUNC(os_status_t, OS_CODE)
os_ipicall_start_schedule_table_rel(uint32 target_core, ScheduleTableType schedtable_id, TickType offset)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_START_SCHEDULE_TABLE_REL;
	param.send_element.in_param.start_schtbl_rel_param.schedtable_id = schedtable_id;
	param.send_element.in_param.start_schtbl_rel_param.offset = offset;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(os_status_t, OS_CODE)
os_ipicall_start_schedule_table_abs(uint32 target_core, ScheduleTableType schedtable_id, TickType start)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_START_SCHEDULE_TABLE_ABS;
	param.send_element.in_param.start_schtbl_abs_param.schedtable_id = schedtable_id;
	param.send_element.in_param.start_schtbl_abs_param.start = start;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(os_status_t, OS_CODE) os_ipicall_stop_schedule_table(uint32 target_core, ScheduleTableType schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_STOP_SCHEDULE_TABLE;
	param.send_element.in_param.stop_schtbl_param.schedtable_id = schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(os_status_t, OS_CODE)
os_ipicall_deviate_schedule_table(uint32 target_core, ScheduleTableType schedtable_id, TickType deviate_value)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_DEVIATE_SCHEDULE_TABLE;
	param.send_element.in_param.deviate_schtbl_param.schedtable_id = schedtable_id;
	param.send_element.in_param.deviate_schtbl_param.deviate_value = deviate_value;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(os_status_t, OS_CODE)
os_ipicall_set_schedule_table_app_id(uint32 target_core, ScheduleTableType schedtable_id, ApplicationIDType app_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SET_SCHEDULE_TABLE_APP_ID;
	param.send_element.in_param.set_schtbl_appid_param.schedtable_id = schedtable_id;
	param.send_element.in_param.set_schtbl_appid_param.app_id = app_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(os_status_t, OS_CODE)
os_ipicall_clear_schedule_table_app_id(uint32 target_core, ScheduleTableType schedtable_id, ApplicationIDType app_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_CLEAR_SCHEDULE_TABLE_APP_ID;
	param.send_element.in_param.clear_schtbl_appid_param.schedtable_id = schedtable_id;
	param.send_element.in_param.clear_schtbl_appid_param.app_id = app_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(os_status_t, OS_CODE)
os_ipicall_get_schedule_table_status(uint32 target_core, ScheduleTableType schedtable_id,
				     ScheduleTableStatusType *schedtable_status)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_GET_SCHEDULE_TABLE_STATUS;
	param.send_element.in_param.get_schtbl_status_param.schedtable_id = schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;
	os_status_t status = os_ipicall_send(target_core, &param);
	*schedtable_status = param.ret_element.out_param.get_schtbl_status_param.schedtable_status;

	return status;
}

FUNC(os_status_t, OS_CODE)
os_ipicall_next_schedule_table(uint32 target_core, ScheduleTableType curr_schedtable_id,
			       ScheduleTableType next_schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_NEXT_SCHEDULE_TABLE;
	param.send_element.in_param.next_schtbl_param.curr_schedtable_id = curr_schedtable_id;
	param.send_element.in_param.next_schtbl_param.next_schedtable_id = next_schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(os_status_t, OS_CODE)
os_ipicall_next_schedule_table_from_stop(uint32 target_core, ScheduleTableType curr_schedtable_id,
					 ScheduleTableType next_schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_NEXT_SCHEDULE_TABLE_FROM_STOP;
	param.send_element.in_param.next_schtbl_fromstop_param.curr_schedtable_id = curr_schedtable_id;
	param.send_element.in_param.next_schtbl_fromstop_param.next_schedtable_id = next_schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

FUNC(os_status_t, OS_CODE)
os_ipicall_start_schedule_table_synchron(uint32 target_core, ScheduleTableType schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_START_SCHEDULE_TABLE_SYN;
	param.send_element.in_param.start_schtbl_synchron_param.schedtable_id = schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send_sync(target_core, &param);
}

FUNC(os_status_t, OS_CODE)
os_ipicall_sync_schedule_table(uint32 target_core, ScheduleTableType schedtable_id, TickType tick_value)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SYNC_SCHEDULE_TABLE;
	param.send_element.in_param.sync_schtbl_param.schedtable_id = schedtable_id;
	param.send_element.in_param.sync_schtbl_param.tick_value = tick_value;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send_sync(target_core, &param);
}

FUNC(os_status_t, OS_CODE) os_ipicall_set_schedule_table_async(uint32 target_core, ScheduleTableType schedtable_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SET_SCHEDULE_TABLE_ASYNC;
	param.send_element.in_param.set_schtbl_async_param.schedtable_id = schedtable_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send_async(target_core, &param);
}

#endif

/*! GetApplicationState*/
FUNC(os_status_t, OS_CODE)
os_ipicall_get_app_state(uint32 target_core, ApplicationType app_id, ApplicationStateRefType state)
{
	os_status_t status;
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_GETAPPLICATIONSTATE;
	param.send_element.in_param.get_app_state_param.app_id = app_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;
	status = os_ipicall_send_sync(target_core, &param);
	*state = (ApplicationStateType)param.ret_element.out_param.get_app_state_param.app_state;

	return status;
}

/*! GetEvent */
FUNC(os_status_t, OS_CODE) os_ipicall_get_event(uint32 target_core, TaskType task_id, os_event_mask_t *mask)
{
	os_status_t status;
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_GETEVENT;
	param.send_element.in_param.get_event_param.task_id = task_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;
	status = os_ipicall_send_sync(target_core, &param);
	*mask = param.ret_element.out_param.get_event_param.mask;

	return status;
}

/*! SetEvent */
FUNC(os_status_t, OS_CODE) os_ipicall_set_event(uint32 target_core, TaskType task_id, os_event_mask_t mask)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SETEVENT;
	param.send_element.in_param.set_event_param.task_id = task_id;
	param.send_element.in_param.set_event_param.mask = mask;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}

/*! SetEventAsync */
os_status_t os_ipicall_set_event_asyn(uint32 target_core, TaskType task_id, os_event_mask_t mask)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SETEVENTASYN;
	param.send_element.in_param.set_event_param.task_id = task_id;
	param.send_element.in_param.set_event_param.mask = mask;

	return os_ipicall_send_async(target_core, &param);
}

#if defined(CONFIG_RT_FRAMEWORK_IPICALL_TEST) && (CONFIG_RT_FRAMEWORK_IPICALL_TEST == 1)
FUNC(os_status_t, OS_CODE) os_ipicall_ipc_test_function(uint32 target_core, uint32 test_value)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_IPC_TEST_FUNCTION;
	param.send_element.in_param.ipc_test_param.test_value = test_value;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send(target_core, &param);
}
#endif

#if defined(CONFIG_RT_FRAMEWORK_HOOK) && (CONFIG_RT_FRAMEWORK_HOOK == 1)
/*! HookCallCallback */
FUNC(os_status_t, OS_CODE) os_ipicall_hook_call_void_hook(uint32 target_core, HookType hook_id)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_HOOK_CALL_VOID_HOOK;
	param.send_element.in_param.hook_call_void_hook_param.hook_id = hook_id;
	param.ret_element.return_status = OS_STATUS_NOT_OK;

	return os_ipicall_send_async(target_core, &param);
}
#endif

FUNC(os_status_t, OS_CODE) os_ipicall_shutdown_all_cores(uint32 target_core_mask, StatusType error_status)
{
	os_ipicall_data_t param;

	param.send_element.service = OS_IPICALL_SHUTDOWN_ALL_CORES;
	param.send_element.in_param.shutdown_all_cores_param.error_status = error_status;

	return os_ipicall_broad_cast(target_core_mask, &param);
}

FUNC(os_status_t, OS_CODE) os_ipicall_recv_shutdown_all_cores(os_ipicall_data_t *param)
{
	os_shutdown_core(param->send_element.in_param.shutdown_all_cores_param.error_status);

	return OS_STATUS_OK;
}

#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
