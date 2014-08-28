/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "allocator.h"

typedef struct allocation_tracker_t allocation_tracker_t;

// Initialize the allocation tracker. If you do not call this function,
// the allocation tracker functions do nothing but are still safe to call.
void allocation_tracker_init(bool use_canaries);

// Reset the allocation tracker. Don't call this in the normal course of
// operations. Useful mostly for testing.
void allocation_tracker_reset(void);

// Expects that there are no allocations at the time of this call. Dumps
// information about unfreed allocations to the log. Returns the amount of
// unallocated memory.
size_t allocation_tracker_expect_no_allocations(void);

// Notify the tracker of a new allocation. If |ptr| is NULL, this function
// does nothing. |requested_size| is the size of the allocation without any
// canaries. |add_canary| indicates if the caller has appropriately sized
// the allocation using |allocation_tracker_resize_for_canary|, and if the
// canaries should be filled now and then checked upon free. |add_canary|
// has no effect if the tracker was initialized with |use_canaries| as false.
// Returns |ptr| offset to the the beginning of the uncanaried region.
void *allocation_tracker_notify_alloc(void *ptr, size_t requested_size, bool add_canary);

// Notify the tracker of an allocation that is being freed. |ptr| must be a
// pointer returned by a call to |allocation_tracker_notify_alloc|. If |ptr| is
// NULL, this function does nothing. Returns |ptr| offset to the real beginning
// of the allocation including any canary space.
void *allocation_tracker_notify_free(void *ptr);

// Get the full size for an allocation, taking into account whether canaries
// are turned on or not. If you call this for an allocation, pass true to the
// |add_canary| parameter of |allocation_tracker_notify_alloc|.
size_t allocation_tracker_resize_for_canary(size_t size);
