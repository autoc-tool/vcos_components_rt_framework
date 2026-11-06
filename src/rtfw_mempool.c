#include <nuttx/config.h>
#include "Os.h"
#include "rtfw_application_internal.h"
#include "rtfw_mempool.h"
#include "application_cfg.h"
#include "core_cfg.h"
#include "syslog.h"
#include "mempool_cfg.h"

static inline void os_mempool_lock(const os_mempool_cfg_t *mempool_module)
{
	switch (mempool_module->lock_mode) {
		case OS_MEMPOOLLOCKMODE_NONE:
			/* Do nothing. */
			break;
		case OS_MEMPOOLLOCKMODE_INTERRUPTS:
			/* Disable interrupts. */
			SuspendOSInterrupts();
			break;
#if (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
		case OS_MEMPOOLLOCKMODE_SPINLOCKS:
			/* Get the spinlock. */
			/* Concurrent lock has to be prevented by the user. */
			(void)GetSpinlock(mempool_module->spinlock_id);
			break;
#endif /*CONFIG_RT_FRAMEWORK_SPINLOCK == 1*/
		default:
			break;
	}
}

static inline void os_mempool_unLock(const os_mempool_cfg_t *mempool_module)
{
	switch (mempool_module->lock_mode) {
		case OS_MEMPOOLLOCKMODE_NONE:
			/* Do nothing. */
			break;
		case OS_MEMPOOLLOCKMODE_INTERRUPTS:
			/* Disable interrupts. */
			ResumeOSInterrupts();
			break;
#if (CONFIG_RT_FRAMEWORK_SPINLOCK == 1)
		case OS_MEMPOOLLOCKMODE_SPINLOCKS:
			/* Get the spinlock. */
			/* Concurrent lock has to be prevented by the user. */
			(void)ReleaseSpinlock(mempool_module->spinlock_id);
			break;
#endif /*CONFIG_RT_FRAMEWORK_SPINLOCK == 1*/
		default:
			break;
	}
}

static inline uint8 *os_mempool_alloc_internal(const os_mempool_module_cfg_t *mempool_module,
					os_mempool_t type,
					uint32 size,
					uint32 alignment,
					boolean use_pool_alignment)
{
	const os_mempool_cfg_t *mempool = NULL_PTR;
	os_mempool_inst_t *mempool_inst = NULL_PTR;
	os_mempool_block_t *mem_block = NULL_PTR;
	uint8 *ret_dddr = NULL_PTR;
	uint32 data_offset = 0U;
	uint32 *mempool_data_offset = NULL_PTR;
	uint32 blockSize = sizeof(os_mempool_block_t);

	for (uint32 index = 0; index < mempool_module->pool_count; index++) {
		mempool = mempool_module->pools[index];
		mempool_inst = mempool->obj_desc;
		if ((mempool->mempool_t == type) &&
			(mempool->block_size >= size) &&
			(use_pool_alignment || (alignment <= mempool->mem_alignment))) {
			os_mempool_lock(mempool);
			mem_block = mempool_inst->head;
			if (LIKELY(mem_block != NULL_PTR)) {
				mempool_inst->head = mem_block->next;
				mempool_inst->available--;
				if (mempool_inst->mininum > mempool_inst->available) {
					mempool_inst->mininum = mempool_inst->available;
				}
				if (use_pool_alignment) {
					data_offset = OS_MEMPOOL_ADDR_ROUNDUP(blockSize, mempool->mem_alignment);
				} else {
					data_offset = OS_MEMPOOL_ADDR_ROUNDUP(blockSize, alignment);
				}
				ret_dddr = (uint8 *)(mem_block) + data_offset;
				mempool_data_offset = (uint32 *)(ret_dddr - sizeof(uint32));
				*mempool_data_offset = data_offset;
				os_mempool_unLock(mempool);
				break;
			} else {
				mempool_inst->failed++;
			}
			os_mempool_unLock(mempool);
		}
	}
	return ret_dddr;
}

static inline void os_mempool_free_internal(void *mem)
{
	const os_mempool_cfg_t *mempool = NULL_PTR;
	os_mempool_inst_t *mempool_inst = NULL_PTR;
	os_mempool_block_t *mem_block = NULL_PTR;
	uint32 data_offset = 0U;

	if (LIKELY(mem != NULL_PTR)) {
		data_offset = *(uint32 *)((uint8 *)mem - sizeof(uint32));
		mem_block = (os_mempool_block_t *)((uint8 *)mem - data_offset);
		mempool = mem_block->mempool;
		mempool_inst = mempool->obj_desc;
		os_mempool_lock(mempool);
		mem_block->next = mempool_inst->head;
		mempool_inst->head = mem_block;
		mempool_inst->available++;
		os_mempool_unLock(mempool);
	}
}

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) os_mempool_init(const os_mempool_cfg_t *mempool)
{
	os_mempool_inst_t *mempool_inst = mempool->obj_desc;
	os_mempool_block_t *mem_block = NULL_PTR;
	uint32 mem_alignment = mempool->mem_alignment;
	uint32 block_start_addr = (uint32)OS_MEMPOOL_ADDR_ROUNDUP(mempool->block_start, mem_alignment);
	uint32 data_offset = (uint32)OS_MEMPOOL_ADDR_ROUNDUP(sizeof(os_mempool_block_t), mem_alignment);
	uint32 block_size = (uint32)OS_MEMPOOL_ADDR_ROUNDUP(mempool->block_size + data_offset, mem_alignment);

	mempool_inst->head = NULL_PTR;
	mempool_inst->available = mempool->block_count;
	mempool_inst->mininum = mempool->block_count;
	mempool_inst->failed = 0;
	for (uint32 index = 0; index < mempool->block_count; index++) {
		uint32 block_addr = (block_start_addr + block_size * index);

		mem_block = (os_mempool_block_t *)block_addr;
		mem_block->next = mempool_inst->head;
		mem_block->mempool = mempool;
		mem_block->owner_app_id = OS_APPLICATION_CNT;
		mempool_inst->head = mem_block;
	}
}

FUNC(void, OS_CODE) os_private_mempools_init(ApplicationIDType app_id)
{
	if (app_id < OS_APPLICATION_CNT) {
		const os_mempool_cfg_t **mempools = os_private_mempools[app_id];

		if (mempools != NULL_PTR) {
			for (uint32 index = 0; mempools[index] != NULL_PTR; index ++) {
				os_mempool_init(mempools[index]);
			}
		}
	}
}

FUNC(void, OS_CODE) os_global_mempools_init(void)
{
	for (uint32 index = 0; os_global_mempools[index] != NULL_PTR; index++) {
		os_mempool_init(os_global_mempools[index]);
	}
}

FUNC(void, OS_CODE) *os_mempool_alloc(const os_mempool_module_cfg_t *mempool_module,
					os_mempool_t type,
					uint32 size,
					uint32 alignment)
{
	return os_mempool_alloc_internal(mempool_module, type, size, alignment, FALSE);
}

FUNC(void, OS_CODE) *os_mempool_auto_alloc(const os_mempool_module_cfg_t *mempool_module,
					os_mempool_t type,
					uint32 size)
{
	return os_mempool_alloc_internal(mempool_module, type, size, 0U, TRUE);
}

FUNC(void, OS_CODE) os_mempool_free(void *mem, uint32 alignment)
{
	(void)alignment;
	os_mempool_free_internal(mem);
}

FUNC(void, OS_CODE) os_mempool_auto_free(void *mem)
{
	os_mempool_free_internal(mem);
}

FUNC(void, OS_CODE) os_print_mempool_info_by_name(const char *name)
{
	const os_mempool_module_cfg_t *module_cfg = NULL;

	for (uint32 i = 0U; mempool_module_map[i].name != NULL; ++i) {
		if (strcmp(mempool_module_map[i].name, name) == 0U) {
			module_cfg = mempool_module_map[i].ptr;
			break;
		}
	}
	if (module_cfg == NULL) {
		syslog(LOG_DEBUG, "\r\n%sModule not found.\r\n", name);
	} else {
		for (uint32 i = 0U; i < module_cfg->pool_count; ++i) {
			const os_mempool_cfg_t *pool = module_cfg->pools[i];

			if ((pool != NULL) && (pool->obj_desc != NULL)) {
				syslog(LOG_DEBUG, "\n\rModuleName: %s PoolName: %s", name, pool->mempool_name);
				syslog(LOG_DEBUG, "\n\rCount: %lu Size: %lu Type: %u CoreID: %lu Avail: %lu Mininum: %lu Fail: %lu\n\r",
					(unsigned long)pool->block_count,
					(unsigned long)pool->block_size,
					(unsigned int)pool->mempool_t,
					(unsigned long)pool->core_id,
					(unsigned long)pool->obj_desc->available,
					(unsigned long)pool->obj_desc->mininum,
					(unsigned long)pool->obj_desc->failed);
			}
		}
	}
	return;
}
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"
