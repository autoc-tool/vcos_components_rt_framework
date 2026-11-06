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

#ifndef RT_FRAMEWORK_LIST_H_
#define RT_FRAMEWORK_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "nuttx/config.h"
#include "rtfw_types.h"

/*
 *define the type of os list node
 */
typedef struct os_list_node_s {
	struct os_list_node_s *next;
	struct os_list_node_s *prev;
} os_list_node_t;

/*
 * Define the type of os list
 */
typedef struct {
	os_list_node_t *last;
} os_list_t;

/*
 * Get the last node of the os list
 */
#define OS_LIST_LAST(plist) ((plist)->last)

/*
 * Check the os list is empty or not.
 */
#define OS_LIST_IS_EMPTY(plist) ((plist)->last == NULL)

/*
 * Get the object associated by node of the os list
 */
#define OS_LIST_ENTRY(ptr, type, member) ((type *)(((u8_t *)ptr) - offsetof(type, member)))

/*
 * Initialize list
 */
static inline void os_list_init_empty(os_list_t *plist)
{
	plist->last = NULL;
}

/*
 * Insert a os list node into os list and put the pnode after after_node specified
 */
static inline void os_list_insert(os_list_node_t *after_node, os_list_node_t *pnode)
{
	os_list_node_t *before_node;

	pnode->prev = after_node;
	before_node = after_node->next;
	after_node->next = pnode;
	pnode->next = before_node;
	before_node->prev = pnode;
}

/*
 * Append a os list node into the tail of the os list
 */
static inline void os_list_append(os_list_t *plist, os_list_node_t *pnode)
{
	os_list_node_t *old_last_node;

	pnode->next = NULL;
	old_last_node = plist->last;
	plist->last = pnode;
	if (old_last_node != NULL) {
		old_last_node->next = pnode;
	}
	pnode->prev = old_last_node;
}

/*
 * Remove a os list node from the os list
 */
static inline void os_list_remove(os_list_t *plist, const os_list_node_t *pnode)
{
	os_list_node_t *next;
	os_list_node_t *previous;

	next = pnode->next;
	previous = pnode->prev;
	if (next != NULL) {
		next->prev = previous;
	} else {
		plist->last = previous;
	}
	if (previous != NULL) {
		previous->next = next;
	}
}

#ifdef __cplusplus
}
#endif

#endif /* RT_FRAMEWORK_LIST_H_ */
