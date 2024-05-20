/*
 * Copyright (C) 2024 Sophgo
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef GST_BM_ALLOCATOR_H
#define GST_BM_ALLOCATOR_H

#include <gst/gst.h>
#include "bm_vpudec_interface.h"

G_BEGIN_DECLS

typedef bm_device_mem_t BmDeviceMem;
typedef  bm_handle_t BmHandle;

#define GST_TYPE_BM_ALLOCATOR             (gst_bm_allocator_get_type())
#define GST_BM_ALLOCATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_BM_ALLOCATOR, GstBmAllocator))
#define GST_BM_ALLOCATOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_BM_ALLOCATOR, GstBmAllocatorClass))
#define GST_BM_ALLOCATOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_BM_ALLOCATOR, GstBmAllocatorClass))
#define GST_BM_ALLOCATOR_CAST(obj)        ((GstBmAllocator *)(obj))
#define GST_IS_BM_ALLOCATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_BM_ALLOCATOR))
#define GST_IS_BM_ALLOCATOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_BM_ALLOCATOR))


typedef struct _GstBmAllocator GstBmAllocator;
typedef struct _GstBmAllocatorClass GstBmAllocatorClass;

GType gst_bm_allocator_get_type(void);

GstAllocator* gst_bm_allocator_new(void);

GstMemory *
gst_bm_allocator_import_bmvidbuf(GstAllocator * allocator, BMVidFrame *bmFrame);

GstMemory *
gst_bm_allocator_import_gst_memory (GstAllocator * allocator, GstMemory *memory);

BmDeviceMem*
gst_bm_allocator_get_bm_buffer(GstMemory *memory);

G_END_DECLS


#endif /* GST_BM_ALLOCATOR_H */
