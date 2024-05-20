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

/**
 * SECTION:gstBmallocator
 * @title: GstBmAllocator
 * @short_description: BMbuffer-backed allocator using the default bmlib allocator
 * @see_also: #GstMemory, #GstPhysMemoryAllocator, #GstBmDmaBufferAllocator
 */
#include <string.h>
#include <gst/gst.h>
#include <gst/allocators/allocators.h>
#include "bmlib_runtime.h"
#include "bmvpuapi_common.h"
#include "gstbmallocator.h"
#include <glib-object.h>

GST_DEBUG_CATEGORY_STATIC(bm_allocator_debug);
#define GST_CAT_DEFAULT bm_allocator_debug


#define GST_BM_MEMORY_TYPE "BmDmaMemory"
typedef struct _GstBmDmaMemory GstBmDmaMemory;


struct _GstBmDmaMemory
{
	GstMemory parent;
	bm_device_mem_t dmabuffer;
	gint import_flag;
	GMutex lock;
};


struct _GstBmAllocator
{
	GstAllocator parent;
	bm_handle_t bmdev_allocator;
};


struct _GstBmAllocatorClass
{
	GstAllocatorClass parent_class;
};

#define parent_class gst_bm_allocator_parent_class

static guintptr gst_bm_allocator_get_phys_addr(GstPhysMemoryAllocator *allocator, GstMemory *memory);
static void gst_bm_allocator_phys_mem_allocator_iface_init(gpointer iface, gpointer G_GNUC_UNUSED iface_data);




G_DEFINE_TYPE_WITH_CODE(
	GstBmAllocator, gst_bm_allocator, GST_TYPE_ALLOCATOR,
	G_IMPLEMENT_INTERFACE(GST_TYPE_PHYS_MEMORY_ALLOCATOR,  gst_bm_allocator_phys_mem_allocator_iface_init)
);

static void gst_bm_allocator_dispose(GObject *object);

static GstMemory* gst_bm_allocator_alloc(GstAllocator *allocator, gsize size, GstAllocationParams *params);
static void gst_bm_allocator_free(GstAllocator *allocator, GstMemory *memory);

static gpointer gst_bm_allocator_map(GstMemory *memory, GstMapInfo *info, gsize maxsize);
static void gst_bm_allocator_unmap(GstMemory *memory, GstMapInfo *info);
static GstMemory * gst_bm_allocator_copy(GstMemory *memory, gssize offset, gssize size);
static GstMemory * gst_bm_allocator_share(GstMemory *memory, gssize offset, gssize size);
static gboolean gst_bm_allocator_is_span(GstMemory *memory1, GstMemory *memory2, gsize *offset);


static void gst_bm_allocator_class_init(GstBmAllocatorClass *klass)
{
	GObjectClass *object_class;
	GstAllocatorClass *allocator_class;
	GST_DEBUG_CATEGORY_INIT(bm_allocator_debug, "Bmallocator", 0, "physical memory allocator using the default bm DMA buffer allocator");

	object_class = G_OBJECT_CLASS(klass);
	allocator_class = GST_ALLOCATOR_CLASS(klass);

	allocator_class->alloc = GST_DEBUG_FUNCPTR(gst_bm_allocator_alloc);
	allocator_class->free = GST_DEBUG_FUNCPTR(gst_bm_allocator_free);
	object_class->dispose = GST_DEBUG_FUNCPTR(gst_bm_allocator_dispose);
}


static void gst_bm_allocator_init(GstBmAllocator *bm_allocator)
{
	GstAllocator *allocator = GST_ALLOCATOR(bm_allocator);
	allocator->mem_type       = GST_BM_MEMORY_TYPE;
	allocator->mem_map_full   = GST_DEBUG_FUNCPTR(gst_bm_allocator_map);
	allocator->mem_unmap_full = GST_DEBUG_FUNCPTR(gst_bm_allocator_unmap);
	allocator->mem_copy       = GST_DEBUG_FUNCPTR(gst_bm_allocator_copy);
	allocator->mem_share      = GST_DEBUG_FUNCPTR(gst_bm_allocator_share);
	allocator->mem_is_span    = GST_DEBUG_FUNCPTR(gst_bm_allocator_is_span);
}

static GQuark
gst_bm_framebuffer_quark (void)
{
	static GQuark quark = 0;
	if (quark == 0)
	quark = g_quark_from_string ("bm-framebuf");

	return quark;
}


static void gst_bm_allocator_dispose(GObject *object)
{
	GstBmAllocator *bm_allocator = GST_BM_ALLOCATOR(object);
	GST_DEBUG_OBJECT(bm_allocator, "dispose %s", GST_OBJECT_NAME(bm_allocator));
	if (bm_allocator->bmdev_allocator != NULL)
	{
		bm_dev_free(bm_allocator->bmdev_allocator);
		bm_allocator->bmdev_allocator = NULL;
	}

	G_OBJECT_CLASS(gst_bm_allocator_parent_class)->dispose(object);
}

static guintptr gst_bm_allocator_get_phys_addr(G_GNUC_UNUSED GstPhysMemoryAllocator *allocator, GstMemory *memory)
{
	GstBmDmaMemory *dma_memory = (GstBmDmaMemory *)memory;
	return dma_memory->dmabuffer.u.device.device_addr + memory->offset;
}

static void gst_bm_allocator_phys_mem_allocator_iface_init(gpointer iface, gpointer G_GNUC_UNUSED iface_data)
{
	GstPhysMemoryAllocatorInterface *phys_mem_allocator_iface = (GstPhysMemoryAllocatorInterface *)iface;
	phys_mem_allocator_iface->get_phys_addr = GST_DEBUG_FUNCPTR(gst_bm_allocator_get_phys_addr);
}

BmDeviceMem* gst_bm_allocator_get_bm_buffer(GstMemory *memory)
{
	GstBmDmaMemory *dma_memory = (GstBmDmaMemory *)memory;
	if(!g_type_is_a(G_OBJECT_TYPE(memory->allocator), GST_TYPE_BM_ALLOCATOR)) {
		g_print("get_bm_buffer is not a BM_ALLOCATOR type\n");
		return NULL;
	}
	return &dma_memory->dmabuffer;
}

static void
gst_bm_mem_import_destroy (gpointer G_GNUC_UNUSED ptr) {
	g_print("gst_bm_mem_import_destroy \n");
	//to be done
	//GstMemory *mem;
	//mem = GST_MEMORY_CAST(ptr);
}

GstMemory *
gst_bm_allocator_import_bmvidbuf(GstAllocator * allocator, BMVidFrame *bmFrame)
{
	GstMemory *mem;
	GstBmDmaMemory *bm_dma_memory;
	BmDeviceMem *dev_mem;
	GQuark quark;
	guint size;

	size = bmFrame->size;

	bm_dma_memory = g_slice_alloc0(sizeof(GstBmDmaMemory));

	gst_memory_init(GST_MEMORY_CAST(bm_dma_memory), GST_MEMORY_FLAG_PHYSICALLY_CONTIGUOUS,
                  allocator, NULL, size , 0, 0, size);
	dev_mem = &bm_dma_memory->dmabuffer;
	dev_mem->u.device.dmabuf_fd = 1;
	dev_mem->u.device.device_addr = (unsigned long)(bmFrame->buf[4]);
	dev_mem->flags.u.mem_type = BM_MEM_TYPE_DEVICE;
	dev_mem->size = size;

	mem = GST_MEMORY_CAST(bm_dma_memory);
	///quark = gst_bm_framebuffer_quark();
	bm_dma_memory->import_flag = 1;
	//gst_mini_object_set_qdata (GST_MINI_OBJECT(mem), quark, bm_dma_memory,
	//	gst_bm_mem_import_destroy);

	return mem;
}
GstMemory * gst_bm_allocator_import_gst_memory (GstAllocator G_GNUC_UNUSED *allocator, GstMemory *memory)
{
	if(!g_type_is_a(G_OBJECT_TYPE(memory->allocator), GST_TYPE_BM_ALLOCATOR)) {
		//g_print("get_bm_buffer is not a BM_ALLOCATOR type\n");
		return NULL;
	}
	return gst_memory_ref (memory);
}

static GstMemory* gst_bm_allocator_alloc(GstAllocator *allocator, gsize size, GstAllocationParams *params)
{
	int error;
	BmDeviceMem *dmabuffer;
	GstBmDmaMemory *bm_dma_memory;
	GstBmAllocator *bm_allocator = GST_BM_ALLOCATOR(allocator);
	g_assert(bm_allocator != NULL);

	bm_dma_memory = g_slice_alloc0(sizeof(GstBmDmaMemory));

	gst_memory_init(GST_MEMORY_CAST(bm_dma_memory), params->flags | GST_MEMORY_FLAG_PHYSICALLY_CONTIGUOUS,
                  allocator, NULL, size + params->padding, params->align, 0, size);

	dmabuffer = &bm_dma_memory->dmabuffer;
	bm_dma_memory->import_flag = 0;
	error = bmvpu_malloc_device_byte_heap(bm_allocator->bmdev_allocator, dmabuffer, size + params->padding, 0x06, 0);
	if (error)
	{
		g_slice_free1(sizeof(GstBmDmaMemory), bm_dma_memory);
		GST_ERROR_OBJECT(bm_allocator, "could not allocate memory with bmdevmem allocator: %s (%d)", strerror(error), error);
		return NULL;
	}
	return GST_MEMORY_CAST(bm_dma_memory);
}


static void gst_bm_allocator_free(G_GNUC_UNUSED GstAllocator *allocator, GstMemory *memory)
{
	GstBmDmaMemory *bm_dma_memory = (GstBmDmaMemory *)memory;
	GstBmAllocator *bm_allocator = GST_BM_ALLOCATOR(allocator);
	if(!g_type_is_a(G_OBJECT_TYPE(memory->allocator), GST_TYPE_BM_ALLOCATOR)) {
		GST_ERROR_OBJECT(bm_allocator, "free memory TYPE_BM_ALLOCATOR \n");
	}
	g_assert(bm_dma_memory != NULL);
	if (!bm_dma_memory->import_flag) {
		bm_free_device(bm_allocator->bmdev_allocator, bm_dma_memory->dmabuffer);
	}
	g_mutex_clear(&(bm_dma_memory->lock));

	g_slice_free1(sizeof(GstBmDmaMemory), bm_dma_memory);
}


static gpointer gst_bm_allocator_map(GstMemory *memory, GstMapInfo *info, gsize maxsize)
{
	GstBmDmaMemory *bm_dma_memory = (GstBmDmaMemory *)memory;
	GstBmAllocator *bm_allocator = GST_BM_ALLOCATOR(memory->allocator);
	int error;
	uint8_t *mapped_virtual_address;

	if (memory->parent != NULL)
		return gst_bm_allocator_map(memory->parent, info, maxsize);

#if 0
	unsigned int flags = 0;
	if (info->flags & GST_MAP_READ)
		flags |= BM_DMA_BUFFER_MAPPING_FLAG_READ;
	if (info->flags & GST_MAP_WRITE)
		flags |= BM_DMA_BUFFER_MAPPING_FLAG_WRITE;
	if (info->flags & GST_MAP_FLAG_BM_MANUAL_SYNC)
		flags |= BM_DMA_BUFFER_MAPPING_FLAG_MANUAL_SYNC;
#endif

	g_mutex_lock(&(bm_dma_memory->lock));
	error = bm_mem_mmap_device_mem_no_cache(bm_allocator->bmdev_allocator, &bm_dma_memory->dmabuffer, (unsigned long long *)&mapped_virtual_address);
	g_mutex_unlock(&(bm_dma_memory->lock));

	if (mapped_virtual_address == NULL)
		GST_ERROR_OBJECT(memory->allocator, "could not map memory: %s (%d)", strerror(error), error);

	info->data = mapped_virtual_address;
	info->size = bm_dma_memory->dmabuffer.size;
	return mapped_virtual_address;
}


static void gst_bm_allocator_unmap(GstMemory *memory, G_GNUC_UNUSED GstMapInfo *info)
{
	GstBmDmaMemory *bm_dma_memory = (GstBmDmaMemory *)memory;
	GstBmAllocator *bm_allocator = GST_BM_ALLOCATOR(memory->allocator);
	g_mutex_lock(&(bm_dma_memory->lock));
	bm_mem_unmap_device_mem(bm_allocator->bmdev_allocator, info->data, info->size);
	g_mutex_unlock(&(bm_dma_memory->lock));
}


static GstMemory * gst_bm_allocator_copy(GstMemory *memory, gssize offset, gssize size)
{
	GstBmDmaMemory *bm_dma_memory = (GstBmDmaMemory *)memory;
	GstBmAllocator *bm_allocator = GST_BM_ALLOCATOR(memory->allocator);
	GstBmDmaMemory *new_bm_dma_memory = NULL;
	uint8_t *mapped_src_data = NULL, *mapped_dest_data = NULL;
	int error;

	g_mutex_lock(&(bm_dma_memory->lock));

	if (size == -1)
	{
		size = bm_dma_memory->dmabuffer.size;
		size = (size > offset) ? (size - offset) : 0;
	}

	new_bm_dma_memory = g_slice_alloc0(sizeof(GstBmDmaMemory));
	if (G_UNLIKELY(new_bm_dma_memory == NULL))
	{
		GST_ERROR_OBJECT(bm_allocator, "could not allocate slice for GstBmDmaMemory copy");
		goto cleanup;
	}

	gst_memory_init(GST_MEMORY_CAST(new_bm_dma_memory), GST_MEMORY_FLAG_PHYSICALLY_CONTIGUOUS, memory->allocator, NULL, size, memory->align, 0, size);

	error = bmvpu_malloc_device_byte_heap(bm_allocator->bmdev_allocator, &new_bm_dma_memory->dmabuffer, size, 0x6, 1);
	if (error)
	{
		GST_ERROR_OBJECT(bm_allocator, "could not allocate bmdevmem buffer for copy: %s (%d)", strerror(error), error);
		goto cleanup;
	}

	error = bm_mem_mmap_device_mem_no_cache(bm_allocator->bmdev_allocator, &bm_dma_memory->dmabuffer, (unsigned long long *)&mapped_src_data);
	if (mapped_src_data == NULL)
	{
		GST_ERROR_OBJECT(bm_allocator, "could not map source bmdevmem buffer for copy: %s (%d)", strerror(error), error);
		goto cleanup;
	}

	error  = bm_mem_mmap_device_mem_no_cache(bm_allocator->bmdev_allocator, &new_bm_dma_memory->dmabuffer, (unsigned long long *)&mapped_dest_data);
	if (mapped_dest_data == NULL)
	{
		GST_ERROR_OBJECT(bm_allocator, "could not map source bmdevmem buffer for copy: %s (%d)", strerror(error), error);
		goto cleanup;
	}

	/* TODO: Is it perhaps possible to copy over vpss instead of by using the CPU? */
	memcpy(mapped_dest_data, mapped_src_data + offset, size);

finish:
	if (mapped_src_data != NULL)
		bm_mem_unmap_device_mem(bm_allocator->bmdev_allocator, mapped_src_data, size);
	if (mapped_dest_data != NULL)
		bm_mem_unmap_device_mem(bm_allocator->bmdev_allocator, mapped_dest_data, size);
	g_mutex_unlock(&(bm_dma_memory->lock));

	return GST_MEMORY_CAST(new_bm_dma_memory);

cleanup:
	if (new_bm_dma_memory != NULL)
	{
		g_slice_free1(sizeof(GstBmDmaMemory), new_bm_dma_memory);
		new_bm_dma_memory = NULL;
	}

	goto finish;
}


static GstMemory * gst_bm_allocator_share(GstMemory *memory, gssize offset, gssize size)
{
	GstBmDmaMemory *bm_dma_memory = (GstBmDmaMemory *)memory;
	GstBmAllocator *bm_allocator = GST_BM_ALLOCATOR(memory->allocator);
	GstBmDmaMemory *new_bm_dma_memory = NULL;
	GstMemory *parent;

	g_mutex_lock(&(bm_dma_memory->lock));

	if (size == -1)
	{
		size = bm_dma_memory->dmabuffer.size;
		size = (size > offset) ? (size - offset) : 0;
	}

	if ((parent = memory->parent) == NULL)
		parent = memory;

	new_bm_dma_memory = g_slice_alloc0(sizeof(GstBmDmaMemory));
	if (G_UNLIKELY(new_bm_dma_memory == NULL))
	{
		GST_ERROR_OBJECT(bm_allocator, "could not allocate slice for GstBmDmaMemory share");
		goto cleanup;
	}

	gst_memory_init(GST_MEMORY_CAST(new_bm_dma_memory), GST_MINI_OBJECT_FLAGS(parent) | GST_MINI_OBJECT_FLAG_LOCK_READONLY | GST_MEMORY_FLAG_PHYSICALLY_CONTIGUOUS, memory->allocator, parent, memory->maxsize, memory->align, memory->offset + offset, size);

	new_bm_dma_memory->dmabuffer = bm_dma_memory->dmabuffer;

finish:
	g_mutex_unlock(&(bm_dma_memory->lock));
	return GST_MEMORY_CAST(new_bm_dma_memory);

cleanup:
	if (new_bm_dma_memory != NULL)
	{
		g_slice_free1(sizeof(GstBmDmaMemory), new_bm_dma_memory);
		new_bm_dma_memory = NULL;
	}

	goto finish;
}


static gboolean gst_bm_allocator_is_span(G_GNUC_UNUSED GstMemory *memory1, G_GNUC_UNUSED GstMemory *memory2, G_GNUC_UNUSED gsize *offset)
{

	return FALSE;
}


/**
 * gst_bm_allocator_new:
 *
 * Creates a new #GstAllocator using the bmbuffer allocator.
 *
 * Returns: (transfer full) (nullable): Newly created allocator, or NULL in case of failure.
 */
GstAllocator* gst_bm_allocator_new(void)
{
	GstBmAllocator *bm_allocator;
	int error = 0;
	bm_allocator = GST_BM_ALLOCATOR_CAST(g_object_new(gst_bm_allocator_get_type(), NULL));
	error = bm_dev_request(&bm_allocator->bmdev_allocator, 0);
	if (bm_allocator->bmdev_allocator == NULL)
	{
		GST_ERROR_OBJECT(bm_allocator, "could not create BM dev allocator: %s (%d)", strerror(error), error);
		gst_object_unref(GST_OBJECT(bm_allocator));
		return NULL;
	}
	GST_DEBUG_OBJECT(bm_allocator, "created new BM allocator %s", GST_OBJECT_NAME(bm_allocator));

	gst_object_ref_sink(GST_OBJECT(bm_allocator));

	return GST_ALLOCATOR_CAST(bm_allocator);
}
#if 0
int main(int argc, char *argv[]) {
	gst_init(&argc, &argv);

	g_print("bm allocator test in \n");
	// Register the custom allocator
	#if 0
	if (!bm_allocator_register()) {
		g_error("Failed to register the custom allocator");
		return -1;
	}
		g_print("bm allocator test 1 \n");

	GstAllocator *allocator = g_object_new(gst_bm_allocator_get_type(), NULL);
	// Now you can use your custom allocator
	#else
	GstAllocator *allocator = gst_bm_allocator_new();
	#endif
	g_assert_nonnull(allocator);

	GstAllocationParams params = {0,};
	gst_allocation_params_init(&params);
	// Use allocator as needed...
	GstMemory *memory = gst_allocator_alloc(allocator, 1920 * 1080 * 3 / 2, &params);
	g_assert_nonnull(memory);

	if (memory) {
		g_print("Memory allocated successfully\n");
	} else {
		g_print("Memory allocation failed\n");
	}

	guintptr physical_address;

	if (G_UNLIKELY(!gst_is_phys_memory(memory)))
	{
		GST_ERROR_OBJECT(allocator, "supplied gstbuffer's memory block is not backed by physical memory");
		goto end;
	}

	physical_address = gst_phys_memory_get_phys_addr(memory);

	g_print("physical_address  %lx \n", physical_address);


	GstMapInfo map;
	gboolean map_result = gst_memory_map(memory, &map, GST_MAP_WRITE);
	g_assert_true(map_result);

	if (map_result) {
		const char *data = "This is a test";
		memcpy(map.data, data, strlen(data) + 1); // +1 for null-terminator
		gst_memory_unmap(memory, &map);
	}

	BMVidFrame bmFrame;
	bmFrame.size = 1920 * 1080 * 3 / 2;
	bmFrame.buf[4] = physical_address;

	GstMemory *mem_import = gst_bm_allocator_import_bmvidbuf(allocator, &bmFrame);
	map_result = gst_memory_map(mem_import, &map, GST_MAP_WRITE);
	g_assert_true(map_result);

	if (map_result) {
		const char *data = map.data;
		g_print("mem_import %s \n", data); // +1 for null-terminator
		gst_memory_unmap(mem_import, &map);
	}

	if (mem_import)
		gst_memory_unref(mem_import);

end:
	if (memory)
		gst_memory_unref(memory);
	// Decrease the reference count when done
	if (allocator) {
		gst_object_unref(allocator);
	}
	return 0;
}
#endif