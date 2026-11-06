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

#ifndef OS_CONTEXT_TYPES_H
#define OS_CONTEXT_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Including File */
#include <nuttx/config.h>
#include "rtfw_types.h"

/* Global Macro Definition */

/* Global Enumeration Definition */

/* Global Variable Declaration */

/* Global Function Declaration */

/* Global Function Implementation */

/***********************************************************************************************************************
 *  GLOBAL CONSTANT MACROS
 **********************************************************************************************************************/

/***********************************************************************************************************************
 *  GLOBAL FUNCTION MACROS
 **********************************************************************************************************************/
#define OS_INVALID_CONTEXT_ID          ((os_context_id_t)0)      /* Invalid ContextID */

/*! Call context ID for Tasks. */
# define OS_TASK_CONTEXT_ID              ((os_context_id_t)1 <<  0)
/*! Call context ID for Category 2 ISRs. */
# define OS_ISR2_CONTEXT_ID              ((os_context_id_t)1 <<  1)
/*! Call context ID for Error Hooks.    */
# define OS_ERRORHOOK_CONTEXT_ID         ((os_context_id_t)1 <<  2)
/*! Call context ID for PreTask Hooks.  */
# define OS_PRETASKHOOK_CONTEXT_ID       ((os_context_id_t)1 <<  3)
/*! Call context ID for PostTask Hooks. */
# define OS_POSTTASKHOOK_CONTEXT_ID      ((os_context_id_t)1 <<  4)
/*! Call context ID for Startup Hooks.  */
# define OS_STARTUPHOOK_CONTEXT_ID       ((os_context_id_t)1 <<  5)
/*! Call context ID for Shutdown Hooks. */
# define OS_SHUTDOWNHOOK_CONTEXT_ID      ((os_context_id_t)1 <<  6)
/*! Call context ID for Callback Hooks. */
# define OS_IOCCALLBACKHOOK_CONTEXT_ID   ((os_context_id_t)1 <<  7)
/*! Call context ID for Alarm Callbacks. */
# define OS_ALARMCALLBACK_CONTEXT_ID     ((os_context_id_t)1 <<  8)
/*! Call context ID for Protection Hooks.*/
# define OS_PROTECTIONHOOK_CONTEXT_ID    ((os_context_id_t)1 <<  9)
/*! Call context ID for Pre-Start Tasks. */
# define OS_PRESTARTTASK_CONTEXT_ID      ((os_context_id_t)1 << 10)
/*! Call context ID for Timing Protection ISR. */
# define OS_TPISR_CONTEXT_ID             ((os_context_id_t)1 << 11)
/*! Call context ID for Category 1 ISRs. */
# define OS_ISR1_CONTEXT_ID              ((os_context_id_t)1 << 12)
/*! Call context ID for Category 0 ISRs. */
# define OS_ISR0_CONTEXT_ID              ((os_context_id_t)1 << 13)

/*! Any Category2 ISR (allows easy check whether the current context is a ISR function) */
# define OS_CONTEXT_ISR               ((os_context_id_t)(OS_ISR2_CONTEXT_ID      | \
								OS_TPISR_CONTEXT_ID    ))

# define OS_CONTEXT_ANYISR               ((os_context_id_t)(OS_ISR2_CONTEXT_ID   | \
								OS_TPISR_CONTEXT_ID    | \
								OS_ISR1_CONTEXT_ID     | \
								OS_ISR0_CONTEXT_ID     ))

# define OS_CONTEXT_ANYTASK               ((os_context_id_t)(OS_TASK_CONTEXT_ID          |   \
								OS_PRESTARTTASK_CONTEXT_ID          ))

/*! Any Hook (allows easy check whether the current context is a hook function) */
# define OS_CONTEXT_ANYHOOK           ((os_context_id_t)(OS_ERRORHOOK_CONTEXT_ID     |   \
								OS_PRETASKHOOK_CONTEXT_ID   |   \
								OS_POSTTASKHOOK_CONTEXT_ID  |   \
								OS_STARTUPHOOK_CONTEXT_ID   |   \
								OS_SHUTDOWNHOOK_CONTEXT_ID  |   \
								OS_IOCCALLBACKHOOK_CONTEXT_ID  |   \
								OS_ALARMCALLBACK_CONTEXT_ID |   \
								OS_PROTECTIONHOOK_CONTEXT_ID))

/*! All contexts except the timing protection ISR and the InitHook. */
# define OS_CONTEXT_TERMINATEABLE    ((os_context_id_t)( OS_TASK_CONTEXT_ID          |   \
								OS_ISR2_CONTEXT_ID          |   \
								OS_ERRORHOOK_CONTEXT_ID     |   \
								OS_PRETASKHOOK_CONTEXT_ID   |   \
								OS_POSTTASKHOOK_CONTEXT_ID  |   \
								OS_STARTUPHOOK_CONTEXT_ID   |   \
								OS_SHUTDOWNHOOK_CONTEXT_ID  |   \
								OS_IOCCALLBACKHOOK_CONTEXT_ID  |   \
								OS_PRESTARTTASK_CONTEXT_ID  |   \
								OS_ALARMCALLBACK_CONTEXT_ID  ))

/* ----- Allowed call contexts for each API ----- */
/*! Calling Context: ActivateTask() */
#define OS_CONTEXT_ACTIVATETASK                ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: TerminateTask() */
#define OS_CONTEXT_TERMINATETASK               ((os_context_id_t)(OS_TASK_CONTEXT_ID))
/*! Calling Context: ChainTask() */
#define OS_CONTEXT_CHAINTASK                   ((os_context_id_t)(OS_TASK_CONTEXT_ID))
/*! Calling Context: Schedule() */
#define OS_CONTEXT_SCHEDULE                    ((os_context_id_t)(OS_TASK_CONTEXT_ID))
/*! Calling Context: GetTaskID() */
#define OS_CONTEXT_GETTASKID                   ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: GetTaskState() */
#define OS_CONTEXT_GETTASKSTATE                ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID))

/*! Calling Context: GetTickCount() */
#define OS_CONTEXT_GETTICKCOUNT                ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))

/*! Calling Context: task_tickdelay() */
#define OS_CONTEXT_TASK_TICKDELAY              ((os_context_id_t)(OS_TASK_CONTEXT_ID))

/*! Calling Context: Os_BarrierSynchronize() */
#define OS_CONTEXT_BARRIERSYNCHRONIZE          ((os_context_id_t)(OS_TASK_CONTEXT_ID))

/*! Calling Context: DisableAllInterrupts() */
#define OS_CONTEXT_DISABLEALLINTERRUPTS        ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_PRESTARTTASK_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_ALARMCALLBACK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: EnableAllInterrupts() */
#define OS_CONTEXT_ENABLEALLINTERRUPTS         ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_PRESTARTTASK_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_ALARMCALLBACK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: SuspendAllInterrupts() */
#define OS_CONTEXT_SUSPENDALLINTERRUPTS        ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_PRESTARTTASK_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_ALARMCALLBACK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: ResumeAllInterrupts() */
#define OS_CONTEXT_RESUMEALLINTERRUPTS         ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_PRESTARTTASK_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_ALARMCALLBACK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: SuspendOSInterrupts() */
#define OS_CONTEXT_SUSPENDOSINTERRUPTS         ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_PRESTARTTASK_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_ALARMCALLBACK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: ResumeOSInterrupts() */
#define OS_CONTEXT_RESUMEOSINTERRUPTS          ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_PRESTARTTASK_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_ALARMCALLBACK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: GetResource() */
#define OS_CONTEXT_GETRESOURCE                 ((os_context_id_t)(OS_TASK_CONTEXT_ID))
/*! Calling Context: ReleaseResource() */
#define OS_CONTEXT_RELEASERESOURCE             ((os_context_id_t)(OS_TASK_CONTEXT_ID))
/*! Calling Context: SetEvent() */
#define OS_CONTEXT_SETEVENT                    ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: ClearEvent() */
#define OS_CONTEXT_CLEAREVENT                  ((os_context_id_t)(OS_TASK_CONTEXT_ID))
/*! Calling Context: GetEvent() */
#define OS_CONTEXT_GETEVENT                    ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID))
/*! Calling Context: WaitEvent() */
#define OS_CONTEXT_WAITEVENT                   ((os_context_id_t)(OS_TASK_CONTEXT_ID))
/*! Calling Context: event_tickwait() */
#define OS_CONTEXT_EVENT_TICKWAIT              ((os_context_id_t)(OS_TASK_CONTEXT_ID))
/*! Calling Context: GetAlarmBase() */
#define OS_CONTEXT_GETALARMBASE                ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID))
/*! Calling Context: GetAlarm() */
#define OS_CONTEXT_GETALARM                    ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID))
/*! Calling Context: SetRelAlarm() */
#define OS_CONTEXT_SETRELALARM                 ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: SetAbsAlarm() */
#define OS_CONTEXT_SETABSALARM                 ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: CancelAlarm() */
#define OS_CONTEXT_CANCELALARM                 ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: GetActiveApplicationMode() */
#define OS_CONTEXT_GETACTIVEAPPLICATIONMODE    ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID))
/*! Calling Context: StartOS() */
#define OS_CONTEXT_STARTOS                     ((os_context_id_t)(0))
/*! Calling Context: ShutdownOS() */
#define OS_CONTEXT_SHUTDOWNOS                  ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID))
/*! Calling Context: GetApplicationID() */
#define OS_CONTEXT_GETAPPLICATIONID            ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: GetISRID() */
#define OS_CONTEXT_GETISRID                    ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: os_disable_interrupt_source() */
#define OS_CONTEXT_ISRDISABLESOURCE            ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))

/*! Calling Context: os_enable_interrupt_source() */
#define OS_CONTEXT_ISRENABLESOURCE             ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))

/*! Calling Context: Os_InitialEnableInterruptSources() */
#define OS_CONTEXT_ISRINITENABLESOURCE         ((os_context_id_t)(OS_TASK_CONTEXT_ID))

/*! Calling Context: os_clear_pending_interrupt() */
#define OS_CONTEXT_ISRCLEARPENDING             ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))

/*! Calling Context: Os_IsInterruptSourceEnabled() */
#define OS_CONTEXT_ISINTERRUPTSOURCEENABLED    ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))

/*! Calling Context: Os_IsInterruptPending() */
#define OS_CONTEXT_ISINTERRUPTPENDING          ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: CallTrustedFunction() */
#define OS_CONTEXT_CALLTRUSTEDFUNCTION         ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_PRESTARTTASK_CONTEXT_ID))
/*! Calling Context: CallNonTrustedFunction() */
#define OS_CONTEXT_CALLNONTRUSTEDFUNCTION      ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_PRESTARTTASK_CONTEXT_ID))

/*! Calling Context: CheckISRMemoryAccess() */
#define OS_CONTEXT_CHECKISRMEMORYACCESS        ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: CheckTaskMemoryAccess() */
#define OS_CONTEXT_CHECKTASKMEMORYACCESS       ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: CheckObjectAccess() */
#define OS_CONTEXT_CHECKOBJECTACCESS           ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: CheckObjectOwnership() */
#define OS_CONTEXT_CHECKOBJECTOWNERSHIP        ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: StartScheduleTableRel() */
#define OS_CONTEXT_STARTSCHEDULETABLEREL       ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: StartScheduleTableAbs() */
#define OS_CONTEXT_STARTSCHEDULETABLEABS       ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: StopScheduleTable() */
#define OS_CONTEXT_STOPSCHEDULETABLE           ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))

/*! Calling Context: SetScheduleTableAppId() */
#define OS_CONTEXT_SETSCHEDULETABLEAPPID       ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))

/*! Calling Context: ClearScheduleTableAppId() */
#define OS_CONTEXT_CLEARSCHEDULETABLEAPPID     ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))

/*! Calling Context: NextScheduleTable() */
#define OS_CONTEXT_NEXTSCHEDULETABLE           ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: NextScheduleTableFromStop() */
#define OS_CONTEXT_NEXTSCHEDULETABLEFROMSTOP   ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: StartScheduleTableSynchron() */
#define OS_CONTEXT_STARTSCHEDULETABLESYNCHRON  ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: SyncScheduleTable() */
#define OS_CONTEXT_SYNCSCHEDULETABLE           ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))

/*! Calling Context: DeviateScheduleTable() */
#define OS_CONTEXT_DEVIATESCHEDULETABLE        ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))

/*! Calling Context: GetScheduleTableStatus() */
#define OS_CONTEXT_GETSCHEDULETABLESTATUS      ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: SetScheduleTableAsync() */
#define OS_CONTEXT_SETSCHEDULETABLEASYNC       ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: IncrementCounter() */
#define OS_CONTEXT_INCREMENTCOUNTER            ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: GetCounterValue() */
#define OS_CONTEXT_GETCOUNTERVALUE             ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: GetElapsedValue() */
#define OS_CONTEXT_GETELAPSEDVALUE             ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: TerminateApplication() */
#define OS_CONTEXT_TERMINATEAPPLICATION        ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID))
/*! Calling Context: AllowAccess() */
#define OS_CONTEXT_ALLOWACCESS                 ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: GetApplicationState() */
#define OS_CONTEXT_GETAPPLICATIONSTATE         ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: ControlIdle() */
#define OS_CONTEXT_CONTROLIDLE                 ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: GetCurrentApplicationID() */
#define OS_CONTEXT_GETCURRENTAPPLICATIONID     ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID))
/*! Calling Context: GetNumberOfActivatedCores() */
#define OS_CONTEXT_GETNUMBEROFACTIVATEDCORES   ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: GetCoreID() */
#define OS_CONTEXT_GETCOREID                   ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_PRETASKHOOK_CONTEXT_ID | \
									OS_POSTTASKHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID | \
									OS_ALARMCALLBACK_CONTEXT_ID | \
									OS_PROTECTIONHOOK_CONTEXT_ID) )
/*! Calling Context: StartCore() */
#define OS_CONTEXT_STARTCORE                   ((os_context_id_t)(0))
/*! Calling Context: StartNonAutosarCore() */
#define OS_CONTEXT_STARTNONAUTOSARCORE         ((os_context_id_t)(0))
/*! Calling Context: get_spinlock() */
#define OS_CONTEXT_GETSPINLOCK                 ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: release_spinlock() */
#define OS_CONTEXT_RELEASESPINLOCK             ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: try_to_get_spinlock() */
#define OS_CONTEXT_TRYTOGETSPINLOCK            ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))
/*! Calling Context: ShutdownAllCores() */
#define OS_CONTEXT_SHUTDOWNALLCORES            ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID))
/*! Calling Context: Os_ErrorGetErrorDetail() */
#define OS_CONTEXT_GETERRORDETAIL              ((os_context_id_t)(OS_ERRORHOOK_CONTEXT_ID))

/*! Calling Context peripheral access API */
#define OS_CONTEXT_PERIPHERAL                  ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID | \
									OS_PRESTARTTASK_CONTEXT_ID | \
									OS_STARTUPHOOK_CONTEXT_ID | \
									OS_ERRORHOOK_CONTEXT_ID | \
									OS_SHUTDOWNHOOK_CONTEXT_ID))
#define OS_CONTEXT_IPICALLSYNC                ((os_context_id_t)(OS_TASK_CONTEXT_ID | \
									OS_ISR2_CONTEXT_ID))

/*! Calling Context: Os_GetExceptionContext() */
#define OS_CONTEXT_GETEXCEPTIONCONTEXT         ((os_context_id_t)(OS_PROTECTIONHOOK_CONTEXT_ID))

/*! Calling Context: Os_SetExceptionContext() */
#define OS_CONTEXT_SETEXCEPTIONCONTEXT         ((os_context_id_t)(OS_PROTECTIONHOOK_CONTEXT_ID))

/* Global Type & Structure Definition */
struct os_context_inst;
typedef struct os_context_inst    os_ctx_inst_t;

struct os_context_cfg;
typedef struct os_context_cfg         os_ctx_cfg_t;

struct os_context_nest;
typedef struct os_context_nest        os_ctx_nest_t;

struct os_arch_isr_cfg;
typedef struct os_arch_isr_cfg         os_arch_isr_cfg_t;

struct os_arch_context_cfg;
typedef struct os_arch_context_cfg     os_arch_ctx_cfg_t;

#ifdef __cplusplus
}
#endif

#endif	 /* OS_CONTEXT_TYPES_H */
