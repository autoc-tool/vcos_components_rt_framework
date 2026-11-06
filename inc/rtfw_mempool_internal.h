#ifndef RT_FRAMEOWRK_INC_OS_MEMPOOL_INTERNAL_H
#define RT_FRAMEOWRK_INC_OS_MEMPOOL_INTERNAL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <nuttx/config.h>
#include "rtfw_types.h"
#include "rtfw_mempool_types.h"
#include "mempool_lcfg.h"

#define OS_MEMPOOL_ADDR_ROUNDUP(memaddr, alignment)   (((uint32)(memaddr) + ((alignment) -1)) & (~((alignment) -1)))

typedef enum
{
	/*! Lock nothing. */
	OS_MEMPOOLLOCKMODE_NONE,
	/*! Lock interrupts. */
	OS_MEMPOOLLOCKMODE_INTERRUPTS,
#if (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
	/*! Lock spinlocks. */
	OS_MEMPOOLLOCKMODE_SPINLOCKS,
#endif /*CONFIG_RT_FRAMEWORK_SPINLOCK == 1*/
}os_mempool_lock_mode_t;

struct os_mempool_block {
	os_mempool_block_t *next;
	const os_mempool_cfg_t *mempool;
	uint32_t owner_app_id;
	uint32_t data_offset;
};

struct os_mempool_inst{
	os_mempool_block_t *head;
	uint32 available;
	uint32 mininum;
	uint32 failed;
};

struct os_mempool_cfg{
	os_mempool_inst_t * const obj_desc;
	os_mempool_lock_mode_t const lock_mode;
#if (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
	SpinlockIdType const spinlock_id;
#endif
	const uint32 block_count;
	const uint32 block_size;
	const uint32 mem_alignment;
	const char * const mempool_name;
	const os_mempool_t mempool_t;
	const CoreIdType core_id;
	void * const block_start;
};

struct os_mempool_module_cfg{
	const char * const name;
	const uint32 pool_count;
	const os_mempool_cfg_t * const *pools;
};

struct os_mempool_module_entry{
	const char *name;
	const os_mempool_module_cfg_t *ptr;
};

extern const os_mempool_cfg_t ** const os_private_mempools[];

extern const os_mempool_cfg_t * const os_global_mempools[];

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"
FUNC(void, OS_CODE) os_mempool_init(const os_mempool_cfg_t *mempool);
FUNC(void, OS_CODE) os_private_mempools_init(ApplicationIDType app_id);
FUNC(void, OS_CODE) os_global_mempools_init(void);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEOWRK_INC_OS_MEMPOOL_INTERNAL_H */
