#ifndef RT_FRAMEOWRK_INC_OS_MEMPOOL_H
#define RT_FRAMEOWRK_INC_OS_MEMPOOL_H

#include "rtfw_mempool_internal.h"

#define OS_START_SEC_CODE
#include "rtfw_memmap.h"

FUNC(void, OS_CODE) *os_mempool_alloc(const os_mempool_module_cfg_t *mempool_module,
									  os_mempool_t type,
									  uint32 size,
									  uint32 alignment);
FUNC(void, OS_CODE) os_mempool_free(void *mem, uint32 alignment);

FUNC(void, OS_CODE) *os_mempool_auto_alloc(const os_mempool_module_cfg_t *mempool_module,
	os_mempool_t type,
	uint32 size);
FUNC(void, OS_CODE) os_mempool_auto_free(void *mem);
#define OS_STOP_SEC_CODE
#include "rtfw_memmap.h"

#endif /* RT_FRAMEOWRK_INC_OS_MEMPOOL_H */
