/*
 * Copyright (c) 2017-2021, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#include "gstnvcompositor.h"
#include "gstnvcompositorpad.h"

GST_DEBUG_CATEGORY_STATIC (gst_nvcompositor_debug);
#define GST_CAT_DEFAULT gst_nvcompositor_debug

#define GST_TYPE_INTERPOLATION_METHOD (gst_video_interpolation_method_get_type())

static const GEnumValue video_interpolation_methods[] = {
  {GST_INTERPOLATION_NEAREST, "Nearest", "Nearest"},
  {GST_INTERPOLATION_BILINEAR, "Bilinear", "Bilinear"},
  {GST_INTERPOLATION_5_TAP, "5-Tap", "5-Tap"},
  {GST_INTERPOLATION_10_TAP, "10-Tap", "10-Tap"},
  {GST_INTERPOLATION_SMART, "Smart", "Smart"},
  {GST_INTERPOLATION_NICEST, "Nicest", "Nicest"},
  {0, NULL, NULL},
};

static GType
gst_video_interpolation_method_get_type (void)
{
  static GType video_interpolation_method_type = 0;

  if (!video_interpolation_method_type) {
      video_interpolation_method_type = g_enum_register_static ("GstInterpolationMethods",
        video_interpolation_methods);
  }
  return video_interpolation_method_type;
}

/* capabilities of the inputs and outputs */

/* Input capabilities */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_NVMM,
            "{ " "RGBA, I420, NV12 }") ";" GST_VIDEO_CAPS_MAKE ("{ "
            "RGBA, I420, NV12 }")));


/* Output capabilities */
static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_NVMM,
            "{ " "RGBA }") ";" GST_VIDEO_CAPS_MAKE ("{ " "RGBA }")));

#define DEFAULT_NVCOMP_PAD_XPOS   0
#define DEFAULT_NVCOMP_PAD_YPOS   0

#define DEFAULT_NVCOMP_PAD_WIDTH  0
#define DEFAULT_NVCOMP_PAD_HEIGHT 0

#define DEFAULT_NVCOMP_PAD_ALPHA  1.0

enum
{
  PROP_PAD_0,
  PROP_NVCOMP_PAD_XPOS,
  PROP_NVCOMP_PAD_YPOS,
  PROP_NVCOMP_PAD_WIDTH,
  PROP_NVCOMP_PAD_HEIGHT,
  PROP_NVCOMP_PAD_ALPHA,
  PROP_NVCOMP_PAD_FILTER
};

#define NV_COMPOSITOR_MAX_BUF         6
#define GST_NV_COMPOSITOR_MEMORY_TYPE "nvcompositor"
#define GST_OMX_MEMORY_TYPE           "openmax"
#define GST_NV_FILTER_MEMORY_TYPE     "nvfilter"
#define GST_NV_V4L2_MEMORY_TYPE       "V4l2Memory"
#define GST_NVARGUS_MEMORY_TYPE       "nvarguscam"

/* NvCompositor memory allocator Implementation */

typedef struct _GstNvCompositorMemory GstNvCompositorMemory;
typedef struct _GstNvCompositorMemoryAllocator GstNvCompositorMemoryAllocator;
typedef struct _GstNvCompositorMemoryAllocatorClass
    GstNvCompositorMemoryAllocatorClass;

struct _GstNvCompositorMemory
{
  GstMemory mem;
  GstNvCompositorBuffer *buf;
};

struct _GstNvCompositorMemoryAllocator
{
  GstAllocator parent;
  guint width;
  guint height;
  NvBufferColorFormat colorFormat;
};

struct _GstNvCompositorMemoryAllocatorClass
{
  GstAllocatorClass parent_class;
};

/**
  * implementation that releases memory.
  *
  * @param allocator : gst memory allocatot object
  * @param mem       : gst memory
  */
static void
gst_nv_compositor_memory_allocator_free (GstAllocator * allocator,
    GstMemory * mem)
{
  gint ret = 0;
  GstNvCompositorMemory *omem = (GstNvCompositorMemory *) mem;
  GstNvCompositorBuffer *nvbuf = omem->buf;

  ret = NvBufferDestroy (nvbuf->dmabuf_fd);
  if (ret != 0) {
    GST_ERROR ("%s: NvBufferDestroy Failed \n", __func__);
    goto error;
  }

error:
  g_slice_free (GstNvCompositorBuffer, nvbuf);
  g_slice_free (GstNvCompositorMemory, omem);
}

/**
  * memory map function.
  *
  * @param mem     : gst memory
  * @param maxsize : memory max size
  * @param flags   : Flags for wrapped memory
  */
static gpointer
gst_nv_compositor_memory_map (GstMemory * mem, gsize maxsize, GstMapFlags flags)
{
  gint ret = 0;
  GstNvCompositorMemory *omem = (GstNvCompositorMemory *) mem;
  NvBufferParams params = { 0 };

  ret = NvBufferGetParams (omem->buf->dmabuf_fd, &params);
  if (ret != 0) {
    GST_ERROR ("%s: NvBufferGetParams Failed \n", __func__);
    goto error;
  }

  return (gpointer) (params.nv_buffer);
error:
  return NULL;
}

/**
  * memory unmap function.
  *
  * @param mem : gst memory
  */
static void
gst_nv_compositor_memory_unmap (GstMemory * mem)
{
}

/**
  * memory share function.
  *
  * @param mem : gst memory
  */
static GstMemory *
gst_nv_compositor_memory_share (GstMemory * mem, gssize offset, gssize size)
{
  g_assert_not_reached ();
  return NULL;
}

GType gst_nv_compositor_memory_allocator_get_type (void);
G_DEFINE_TYPE (GstNvCompositorMemoryAllocator,
    gst_nv_compositor_memory_allocator, GST_TYPE_ALLOCATOR);

#define GST_TYPE_NV_COMPOSITOR_MEMORY_ALLOCATOR   (gst_nv_compositor_memory_allocator_get_type())
#define GST_NV_COMPOSITOR_MEMORY_ALLOCATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_NV_COMPOSITOR_MEMORY_ALLOCATOR,GstNvCompositorMemoryAllocator))
#define GST_IS_NV_COMPOSITOR_MEMORY_ALLOCATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_NV_COMPOSITOR_MEMORY_ALLOCATOR))

/**
  * custom memory allocation.
  *
  * @param allocator : gst memory allocatot object
  * @param size      : memory size
  * @param params    : allovcation params
  */
static GstMemory *
gst_nv_compositor_memory_allocator_alloc (GstAllocator * allocator,
    gsize size, GstAllocationParams * params)
{
  gint ret = 0;
  GstNvCompositorMemory *mem = NULL;
  GstNvCompositorBuffer *nvbuf = NULL;
  NvBufferParams par = { 0 };
  NvBufferCreateParams input_params = {0};
  GstNvCompositorMemoryAllocator *compalloc = GST_NV_COMPOSITOR_MEMORY_ALLOCATOR (allocator);

  mem = g_slice_new0 (GstNvCompositorMemory);
  nvbuf = g_slice_new0 (GstNvCompositorBuffer);

  input_params.width = compalloc->width;
  input_params.height = compalloc->height;
  input_params.layout = NvBufferLayout_Pitch;
  input_params.colorFormat = compalloc->colorFormat;
  input_params.payloadType = NvBufferPayload_SurfArray;
  input_params.nvbuf_tag = NvBufferTag_VIDEO_CONVERT;

  ret = NvBufferCreateEx (&nvbuf->dmabuf_fd, &input_params);
  if (ret != 0) {
    GST_ERROR ("%s: NvBufferCreateEx Failed \n", __func__);
    goto error;
  }

  ret = NvBufferGetParams (nvbuf->dmabuf_fd, &par);
  if (ret != 0) {
    GST_ERROR ("%s: NvBufferGetParams Failed \n", __func__);
    goto error;
  }

  gst_memory_init (GST_MEMORY_CAST (mem), GST_MEMORY_FLAG_NO_SHARE, allocator, NULL, par.nv_buffer_size, 1,/* Alignment */
      0, par.nv_buffer_size);
  mem->buf = nvbuf;

  return GST_MEMORY_CAST (mem);

error:
  g_slice_free (GstNvCompositorBuffer, nvbuf);
  g_slice_free (GstNvCompositorMemory, mem);

  return NULL;
}

/**
  * initialize the nvcompositor allocator's class.
  *
  * @param klass : nvcompositor memory allocator objectclass
  */
static void
    gst_nv_compositor_memory_allocator_class_init
    (GstNvCompositorMemoryAllocatorClass * klass)
{
  GstAllocatorClass *allocator_class;

  allocator_class = GST_ALLOCATOR_CLASS (klass);

  allocator_class->alloc = gst_nv_compositor_memory_allocator_alloc;
  allocator_class->free = gst_nv_compositor_memory_allocator_free;
}

/**
  * nvcompositor allocator init function.
  *
  * @param allocator : nvcompositor allocator object instance
  */
static void
gst_nv_compositor_memory_allocator_init (GstNvCompositorMemoryAllocator *
    allocator)
{
  GstAllocator *alloc = GST_ALLOCATOR_CAST (allocator);

  alloc->mem_type = GST_NV_COMPOSITOR_MEMORY_TYPE;
  alloc->mem_map = gst_nv_compositor_memory_map;
  alloc->mem_unmap = gst_nv_compositor_memory_unmap;
  alloc->mem_share = gst_nv_compositor_memory_share;

  GST_OBJECT_FLAG_SET (allocator, GST_ALLOCATOR_FLAG_CUSTOM_ALLOC);
}

/* Create a new allocator of type GST_TYPE_NV_COMPOSITOR_MEMORY_ALLOCATOR and initialize
 * members. */
static GstAllocator *
gst_nv_compositor_allocator_new (guint width, guint height,
    NvBufferColorFormat out_pix_fmt)
{
  GstNvCompositorMemoryAllocator *allocator = (GstNvCompositorMemoryAllocator *)
      g_object_new (GST_TYPE_NV_COMPOSITOR_MEMORY_ALLOCATOR, NULL);

  allocator->width = width;
  allocator->height = height;
  allocator->colorFormat = out_pix_fmt;

  return (GstAllocator *) allocator;
}

G_DEFINE_TYPE (GstNvCompositorPad, gst_nvcompositor_pad,
    GST_TYPE_VIDEO_AGGREGATOR_PAD);

/**
  * nvcompositor pad set property function.
  *
  * @param object : GstNvCompositorPad object instance
  * @param prop_id : Property ID
  * @param value : Property value
  */
static void
gst_nvcompositor_pad_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstNvCompositorPad *pad = GST_NVCOMPOSITOR_PAD (object);

  switch (prop_id) {
    case PROP_NVCOMP_PAD_XPOS:
      pad->xpos = g_value_get_int (value);
      break;
    case PROP_NVCOMP_PAD_YPOS:
      pad->ypos = g_value_get_int (value);
      break;
    case PROP_NVCOMP_PAD_WIDTH:
      pad->width = g_value_get_int (value);
      break;
    case PROP_NVCOMP_PAD_HEIGHT:
      pad->height = g_value_get_int (value);
      break;
    case PROP_NVCOMP_PAD_ALPHA:
      pad->alpha = g_value_get_double (value);
      break;
    case PROP_NVCOMP_PAD_FILTER:
      pad->interpolation_method = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/**
  * nvcompositor pad get property function.
  *
  * @param object : GstNvCompositorPad object instance
  * @param prop_id : Property ID
  * @param value : Property value
  */
static void
gst_nvcompositor_pad_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstNvCompositorPad *pad = GST_NVCOMPOSITOR_PAD (object);

  switch (prop_id) {
    case PROP_NVCOMP_PAD_XPOS:
      g_value_set_int (value, pad->xpos);
      break;
    case PROP_NVCOMP_PAD_YPOS:
      g_value_set_int (value, pad->ypos);
      break;
    case PROP_NVCOMP_PAD_WIDTH:
      g_value_set_int (value, pad->width);
      break;
    case PROP_NVCOMP_PAD_HEIGHT:
      g_value_set_int (value, pad->height);
      break;
    case PROP_NVCOMP_PAD_ALPHA:
      g_value_set_double (value, pad->alpha);
      break;
    case PROP_NVCOMP_PAD_FILTER:
      g_value_set_enum (value, pad->interpolation_method);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/**
  * get Nvbuffer color format.
  *
  * @param info : video frame info
  * @param pix_fmt : Nvbuffer color format
  */
static gboolean
get_nvcolorformat (GstVideoInfo * info, NvBufferColorFormat * pix_fmt)
{
  gboolean ret = TRUE;

  switch (GST_VIDEO_INFO_FORMAT (info)) {
    case GST_VIDEO_FORMAT_I420:
      *pix_fmt = NvBufferColorFormat_YUV420;
      break;
    case GST_VIDEO_FORMAT_NV12:
      *pix_fmt = NvBufferColorFormat_NV12;
      break;
    case GST_VIDEO_FORMAT_RGBA:
      *pix_fmt = NvBufferColorFormat_ABGR32;
      break;
    default:
      GST_ERROR ("buffer type not supported");
      *pix_fmt = NvBufferColorFormat_Invalid;
      ret = FALSE;
      break;
  }

  return ret;
}

/**
  * get width & height for output buffer.
  *
  * @param nvcomp : GstNvCompositor object instance
  * @param nvcomp_pad : GstNvCompositorPad object instance
  * @param out_par_numerator : output pixel aspect ratio numerator
  * @param out_par_denominator : output pixel aspect ratio denominator
  * @param output_width : width for output frame
  * @param output_height : height for output frame
  */
static void
gst_nvcompositor_mpad_output_size (GstNvCompositor * nvcomp,
    GstNvCompositorPad * nvcomp_pad, gint out_par_numerator,
    gint out_par_denominator, gint * output_width, gint * output_height)
{
  GstVideoAggregatorPad *vagg_pad = GST_VIDEO_AGGREGATOR_PAD (nvcomp_pad);
  gint pad_w, pad_h;
  guint dar_numerator, dar_denominator;

  if (!vagg_pad->info.finfo
      || vagg_pad->info.finfo->format == GST_VIDEO_FORMAT_UNKNOWN) {
    GST_DEBUG_OBJECT (nvcomp_pad, "Do not have caps yet");
    *output_width = 0;
    *output_height = 0;
    return;
  }

  pad_w =
      nvcomp_pad->width <=
      0 ? GST_VIDEO_INFO_WIDTH (&vagg_pad->info) : nvcomp_pad->width;
  pad_h =
      nvcomp_pad->height <=
      0 ? GST_VIDEO_INFO_HEIGHT (&vagg_pad->info) : nvcomp_pad->height;

  if (!gst_video_calculate_display_ratio (&dar_numerator, &dar_denominator, pad_w,
      pad_h, GST_VIDEO_INFO_PAR_N (&vagg_pad->info),
      GST_VIDEO_INFO_PAR_D (&vagg_pad->info), out_par_numerator,
      out_par_denominator)) {
    GST_WARNING_OBJECT (nvcomp_pad, "Display aspect ratio can not be calculated");
    *output_width = 0;
    *output_height = 0;
    return;
  }
  GST_LOG_OBJECT (nvcomp_pad, "scaling %ux%u by %u/%u (%u/%u / %u/%u)", pad_w,
      pad_h, dar_numerator, dar_denominator,
      GST_VIDEO_INFO_PAR_N (&vagg_pad->info),
      GST_VIDEO_INFO_PAR_D (&vagg_pad->info), out_par_numerator,
      out_par_denominator);

  if (pad_h % dar_numerator == 0) {
    pad_w = gst_util_uint64_scale_int (pad_h, dar_numerator, dar_denominator);
  } else if (pad_w % dar_denominator == 0) {
    pad_h = gst_util_uint64_scale_int (pad_w, dar_denominator, dar_numerator);
  } else {
    pad_w = gst_util_uint64_scale_int (pad_h, dar_numerator, dar_denominator);
  }

  if (output_width)
    *output_width = pad_w;
  if (output_height)
    *output_height = pad_h;
}

/**
  * NvCompositorPad set info function.
  *
  * @param pad: GstVideoAggregatorPad object instance
  * @param vagg: GstVideoAggregator object instance
  * @param current_info: input pad video info
  * @param wanted_info: input pad needed info
  */
static gboolean
gst_nvcompositor_pad_set_info (GstVideoAggregatorPad * pad,
    GstVideoAggregator * vagg G_GNUC_UNUSED,
    GstVideoInfo * current_info, GstVideoInfo * wanted_info)
{
  GstNvCompositorPad *cpad = GST_NVCOMPOSITOR_PAD (pad);

  if (!current_info->finfo)
    return TRUE;

  if (GST_VIDEO_INFO_FORMAT (current_info) == GST_VIDEO_FORMAT_UNKNOWN)
    return TRUE;

  cpad->conversion_info = *current_info;
  cpad->input_width = GST_VIDEO_INFO_WIDTH (&cpad->conversion_info);
  cpad->input_height = GST_VIDEO_INFO_HEIGHT (&cpad->conversion_info);

  if (!get_nvcolorformat (&cpad->conversion_info, &cpad->comppad_pix_fmt)) {
    GST_ERROR_OBJECT (vagg,
        "Failed to get nvcompositorpad input NvColorFormat");
    return FALSE;
  }

  return TRUE;
}

/**
  * Prepare the frame from the pad buffer.
  *
  * @param pad: GstVideoAggregatorPad object instance
  * @param vagg: GstVideoAggregator object instance
  */
static gboolean
gst_nvcompositor_pad_prepare_frame (GstVideoAggregatorPad * pad,
    GstVideoAggregator * vagg)
{
  guint i = 0;
  gint ret = 0;
  guint SrcWidth[3] = { 0 };
  guint SrcHeight[3] = { 0 };
  guint Bufsize = 0;
  GstMemory *inmem = NULL;
  GstMapInfo inmap = GST_MAP_INFO_INIT;
  NvBufferParams params = { 0 };
  NvBufferCreateParams input_params = {0};
  GstNvCompositorPad *cpad = GST_NVCOMPOSITOR_PAD (pad);

  if (!pad->buffer)
    return TRUE;

  inmem = gst_buffer_peek_memory (pad->buffer, 0);
  if (!inmem)
    goto no_memory;

  if (!g_strcmp0 (inmem->allocator->mem_type, GST_ALLOCATOR_SYSMEM)) {
    if (!gst_buffer_map (pad->buffer, &inmap, GST_MAP_READ))
      goto invalid_inbuf;

    if (cpad->comppad_buf_flag == TRUE) {
      input_params.width = GST_VIDEO_INFO_WIDTH (&cpad->conversion_info);
      input_params.height = GST_VIDEO_INFO_HEIGHT (&cpad->conversion_info);
      input_params.layout = NvBufferLayout_Pitch;
      input_params.colorFormat = cpad->comppad_pix_fmt;
      input_params.payloadType = NvBufferPayload_SurfArray;
      input_params.nvbuf_tag = NvBufferTag_VIDEO_CONVERT;

      ret = NvBufferCreateEx (&cpad->comppad_buf.pad_dmabuf_fd, &input_params);
      if (ret != 0) {
        GST_ERROR ("%s: NvBufferCreateEx Failed \n", __func__);
        goto error;
      }
      cpad->comppad_buf_flag = FALSE;
    }

    ret = NvBufferGetParams (cpad->comppad_buf.pad_dmabuf_fd, &params);
    if (ret != 0) {
      GST_ERROR ("NvBufferGetParams failed for out_dmabuf_fd\n");
      goto error;
    }

    switch (params.pixel_format) {
      case NvBufferColorFormat_ABGR32:
        SrcWidth[0] = GST_VIDEO_INFO_WIDTH (&cpad->conversion_info);
        SrcHeight[0] = GST_VIDEO_INFO_HEIGHT (&cpad->conversion_info);
        break;
      case NvBufferColorFormat_NV12:
        SrcWidth[0] =
            GST_ROUND_UP_4 (GST_VIDEO_INFO_WIDTH (&cpad->conversion_info));
        SrcHeight[0] =
            GST_ROUND_UP_2 (GST_VIDEO_INFO_HEIGHT (&cpad->conversion_info));
        SrcWidth[1] = GST_ROUND_UP_2 (SrcWidth[0] / 2);
        SrcHeight[1] = SrcHeight[0] / 2;
        break;
      case NvBufferColorFormat_YUV420:
        SrcWidth[0] =
            GST_ROUND_UP_4 (GST_VIDEO_INFO_WIDTH (&cpad->conversion_info));
        SrcHeight[0] =
            GST_ROUND_UP_2 (GST_VIDEO_INFO_HEIGHT (&cpad->conversion_info));
        SrcWidth[1] =
            GST_ROUND_UP_4 (GST_VIDEO_INFO_WIDTH (&cpad->conversion_info) / 2);
        SrcHeight[1] = SrcHeight[0] / 2;
        SrcWidth[2] = SrcWidth[1];
        SrcHeight[2] = SrcHeight[1];
        break;
      default:
        GST_ERROR ("%s: Not supoprted in_pix_fmt\n", __func__);
        goto error;
        break;
    }

    for (i = 0; i < params.num_planes; i++) {
      ret =
          Raw2NvBuffer (inmap.data + Bufsize, i, SrcWidth[i], SrcHeight[i],
          cpad->comppad_buf.pad_dmabuf_fd);
      if (ret != 0) {
        GST_ERROR ("Raw2NvBuffer Failed\n");
        goto error;
      }
      Bufsize += SrcWidth[i] * SrcHeight[i];
    }

    gst_buffer_unmap (pad->buffer, &inmap);
  }

  return TRUE;

/* ERRORS */
no_memory:
  {
    GST_ERROR ("no memory block");
    return FALSE;
  }
invalid_inbuf:
  {
    GST_ERROR ("input buffer mapinfo failed");
    return FALSE;
  }
error:
  {
    gst_buffer_unmap (pad->buffer, &inmap);
    return FALSE;
  }
}

/**
  * clean the frame previously prepared(if any).
  *
  * @param pad: GstVideoAggregatorPad object instance
  * @param vagg: GstVideoAggregator object instance
  */
static void
gst_nvcompositor_pad_clean_frame (GstVideoAggregatorPad * pad,
    GstVideoAggregator * vagg)
{
}

/**
  * nvcompositor pad finalize function.
  *
  * @param object : GstVideoAggregatorPad object instanc
  */
static void
gst_nvcompositor_pad_finalize (GObject * object)
{
  gint ret = 0;
  GstNvCompositorPad *cpad = GST_NVCOMPOSITOR_PAD (object);

  if (cpad->comppad_buf.pad_dmabuf_fd != -1) {
    ret = NvBufferDestroy (cpad->comppad_buf.pad_dmabuf_fd);
    if (ret != 0) {
      GST_ERROR ("%s: NvBufferDestroy Failed \n", __func__);
    }
  }

  G_OBJECT_CLASS (gst_nvcompositor_pad_parent_class)->finalize (object);
}

/**
  * initialize the GstNvCompositorPad's class.
  *
  * @param klass : GstNvCompositorPad objectclass
  */
static void
gst_nvcompositor_pad_class_init (GstNvCompositorPadClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstVideoAggregatorPadClass *vaggpadclass =
      (GstVideoAggregatorPadClass *) klass;

  gobject_class->set_property = gst_nvcompositor_pad_set_property;
  gobject_class->get_property = gst_nvcompositor_pad_get_property;
  gobject_class->finalize = gst_nvcompositor_pad_finalize;

  g_object_class_install_property (gobject_class, PROP_NVCOMP_PAD_XPOS,
      g_param_spec_int ("xpos", "X Position", "X Position of the frame",
          G_MININT, G_MAXINT, DEFAULT_NVCOMP_PAD_XPOS,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_NVCOMP_PAD_YPOS,
      g_param_spec_int ("ypos", "Y Position", "Y Position of the frame",
          G_MININT, G_MAXINT, DEFAULT_NVCOMP_PAD_YPOS,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_NVCOMP_PAD_WIDTH,
      g_param_spec_int ("width", "Width", "Width of the frame",
          G_MININT, G_MAXINT, DEFAULT_NVCOMP_PAD_WIDTH,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_NVCOMP_PAD_HEIGHT,
      g_param_spec_int ("height", "Height", "Height of the frame",
          G_MININT, G_MAXINT, DEFAULT_NVCOMP_PAD_HEIGHT,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_NVCOMP_PAD_ALPHA,
      g_param_spec_double ("alpha", "Alpha", "Alpha of the frame", 0.0, 1.0,
          DEFAULT_NVCOMP_PAD_ALPHA,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NVCOMP_PAD_FILTER,
      g_param_spec_enum ("interpolation-method", "Interpolation-method", "Set interpolation methods",
          GST_TYPE_INTERPOLATION_METHOD, GST_INTERPOLATION_NEAREST,
          GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
          G_PARAM_STATIC_STRINGS));

  vaggpadclass->set_info = GST_DEBUG_FUNCPTR (gst_nvcompositor_pad_set_info);
  vaggpadclass->prepare_frame =
      GST_DEBUG_FUNCPTR (gst_nvcompositor_pad_prepare_frame);
  vaggpadclass->clean_frame =
      GST_DEBUG_FUNCPTR (gst_nvcompositor_pad_clean_frame);
}

/**
  * initialize nvcompositor pad instance structure.
  *
  * @param nvcompo_pad : GstNvCompositorPad instance
  */
static void
gst_nvcompositor_pad_init (GstNvCompositorPad * nvcompo_pad)
{
  nvcompo_pad->xpos = DEFAULT_NVCOMP_PAD_XPOS;
  nvcompo_pad->ypos = DEFAULT_NVCOMP_PAD_YPOS;
  nvcompo_pad->alpha = DEFAULT_NVCOMP_PAD_ALPHA;
  nvcompo_pad->comppad_buf_flag = TRUE;
  nvcompo_pad->comppad_buf.pad_dmabuf_fd = -1;
}

/* GstNvCompositor */

#define DEFAULT_BACKGROUND NVCOMPOSITOR_BACKGROUND_BLACK

enum
{
  PROP_0,
  PROP_BACKGROUND
};

#define GST_TYPE_NVCOMPOSITOR_BACKGROUND (gst_nvcompositor_background_get_type())
static GType
gst_nvcompositor_background_get_type (void)
{
  static GType nvcompositor_bg_type = 0;

  static const GEnumValue nvcompositor_bg[] = {
    {NVCOMPOSITOR_BACKGROUND_BLACK, "Black", "black"},
    {NVCOMPOSITOR_BACKGROUND_RED, "Red", "red"},
    {NVCOMPOSITOR_BACKGROUND_GREEN, "Green", "green"},
    {NVCOMPOSITOR_BACKGROUND_BLUE, "Blue", "blue"},
    {NVCOMPOSITOR_BACKGROUND_WHITE, "White", "white"},
    {0, NULL, NULL},
  };

  if (!nvcompositor_bg_type) {
    nvcompositor_bg_type =
        g_enum_register_static ("GstNvCompositorBackground", nvcompositor_bg);
  }
  return nvcompositor_bg_type;
}

/* TODO: Replace background property static enum with rgb values */
#if 0
static void
gst_nvcompositor_parse_bgcolor (const GValue * value, GstNvCompositor * nvcomp)
{
  GArray *array;

  array = (GArray *) g_value_get_boxed (value);
  if (!array || array->len != 3)
    return;

  nvcomp->bg.r = g_array_index (array, gfloat, 0);
  nvcomp->bg.g = g_array_index (array, gfloat, 1);
  nvcomp->bg.b = g_array_index (array, gfloat, 2);

  GST_DEBUG ("Background color values: R:%f, G:%f, B:%f", nvcomp->bg.r,
      nvcomp->bg.g, nvcomp->bg.b);
}

static void
gst_nvcompositor_get_bgcolor (GValue * value, GstNvCompositor * nvcomp)
{
  GArray *array;

  array = g_array_new (FALSE, TRUE, sizeof (gfloat));

  g_array_append_val (array, nvcomp->bg.r);
  g_array_append_val (array, nvcomp->bg.g);
  g_array_append_val (array, nvcomp->bg.b);
  g_value_set_boxed (value, array);
}
#endif

/**
  * nvcompositor set property function.
  *
  * @param object : GstNvCompositor object instance
  * @param prop_id : Property ID
  * @param value : Property value
  */
static void
gst_nvcompositor_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstNvCompositor *nvcomp = GST_NVCOMPOSITOR (object);

  switch (prop_id) {
    case PROP_BACKGROUND:
//    gst_nvcompositor_parse_bgcolor (value, nvcomp);
      nvcomp->background = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/**
  * nvcompositor get property function.
  *
  * @param object : GstNvCompositor object instance
  * @param prop_id : Property ID
  * @param value : Property value
  */
static void
gst_nvcompositor_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstNvCompositor *nvcomp = GST_NVCOMPOSITOR (object);

  switch (prop_id) {
    case PROP_BACKGROUND:
//    gst_nvcompositor_get_bgcolor (value, nvcomp);
      g_value_set_enum (value, nvcomp->background);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstElementClass *gparent_class = NULL;

#define gst_nvcompositor_parent_class parent_class
G_DEFINE_TYPE (GstNvCompositor, gst_nvcompositor, GST_TYPE_VIDEO_AGGREGATOR);

/**
  * Fixate and return the src pad caps provided.
  *
  * @param vagg : GstVideoAggregator object instance
  * @param caps : source pad caps
  * @param :
  */
static GstCaps *
gst_nvcompositor_fixate_caps (GstAggregator * agg, GstCaps * caps)
{
  GstCaps *ret = NULL;
  GList *l;
  GstStructure *str;
  gint par_numerator, par_denominator;
  gdouble suitable_fps = 0.;
  gint suitable_fps_n = -1, suitable_fps_d = -1;
  gint suitable_width = -1, suitable_height = -1;
  GstVideoAggregator *vagg = GST_VIDEO_AGGREGATOR (agg);

  ret = gst_caps_make_writable (caps);

  /* calculate how big to make the output frame */
  str = gst_caps_get_structure (ret, 0);
  if (gst_structure_has_field (str, "pixel-aspect-ratio")) {
    gst_structure_fixate_field_nearest_fraction (str, "pixel-aspect-ratio", 1,
        1);
    gst_structure_get_fraction (str, "pixel-aspect-ratio", &par_numerator,
        &par_denominator);
  } else {
    par_numerator = par_denominator = 1;
  }

  GST_OBJECT_LOCK (vagg);
  for (l = GST_ELEMENT (vagg)->sinkpads; l; l = l->next) {
    GstVideoAggregatorPad *vaggpad = l->data;
    GstNvCompositorPad *nvcompositor_pad = GST_NVCOMPOSITOR_PAD (vaggpad);
    gdouble current_fps;
    gint fps_numerator, fps_denominator;
    gint mpad_output_width, mpad_output_height;
    gint cur_width, cur_height;

    fps_numerator = GST_VIDEO_INFO_FPS_N (&vaggpad->info);
    fps_denominator = GST_VIDEO_INFO_FPS_D (&vaggpad->info);

    gst_nvcompositor_mpad_output_size (GST_NVCOMPOSITOR (vagg),
        nvcompositor_pad, par_numerator, par_denominator, &mpad_output_width,
        &mpad_output_height);

    if (mpad_output_width == 0 || mpad_output_height == 0)
      continue;

    cur_width = mpad_output_width + MAX (nvcompositor_pad->xpos, 0);
    cur_height = mpad_output_height + MAX (nvcompositor_pad->ypos, 0);

    if (suitable_width < cur_width)
      suitable_width = cur_width;
    if (suitable_height < cur_height)
      suitable_height = cur_height;

    if (fps_denominator == 0)
      current_fps = 0.0;
    else
      gst_util_fraction_to_double (fps_numerator, fps_denominator,
          &current_fps);

    if (suitable_fps < current_fps) {
      suitable_fps = current_fps;
      suitable_fps_n = fps_numerator;
      suitable_fps_d = fps_denominator;
    }
  }
  GST_OBJECT_UNLOCK (vagg);

  if (suitable_fps_n <= 0 || suitable_fps_d <= 0 || suitable_fps == 0.0) {
    suitable_fps = 30.0;
    suitable_fps_n = 30;
    suitable_fps_d = 1;
  }

  gst_structure_fixate_field_nearest_fraction (str, "framerate", suitable_fps_n,
      suitable_fps_d);
  gst_structure_fixate_field_nearest_int (str, "width", suitable_width);
  gst_structure_fixate_field_nearest_int (str, "height", suitable_height);
  ret = gst_caps_fixate (ret);

  return ret;
}

/**
  * notifies negotiated caps format
  *
  * @param vagg : GstVideoAggregator object instance
  * @param caps : source pad caps
  */
static gboolean
gst_nvcompositor_negotiated_caps (GstAggregator * agg, GstCaps * caps)
{
  GstVideoAggregator *vagg = GST_VIDEO_AGGREGATOR (agg);
  GstNvCompositor *nvcomp = GST_NVCOMPOSITOR (vagg);
  GstVideoInfo v_info;
  GstCapsFeatures *ift = NULL;

  GST_DEBUG_OBJECT (agg, "Negotiated caps %" GST_PTR_FORMAT, caps);

  if (!gst_video_info_from_caps (&v_info, caps))
    return FALSE;

  nvcomp->out_width = GST_VIDEO_INFO_WIDTH (&v_info);
  nvcomp->out_height = GST_VIDEO_INFO_HEIGHT (&v_info);

  if (!get_nvcolorformat (&v_info, &nvcomp->out_pix_fmt)) {
    GST_ERROR_OBJECT (vagg, "Failed to get nvcompositor output NvColorFormat");
    return FALSE;
  }

  ift = gst_caps_features_new (GST_CAPS_FEATURE_MEMORY_NVMM, NULL);
  if (gst_caps_features_is_equal (gst_caps_get_features (caps, 0), ift)) {
    nvcomp->nvcomppool = TRUE;
    if (nvcomp->pool) {
      GST_WARNING_OBJECT (vagg, "Release old pool");
      gst_object_unref (nvcomp->pool);
      nvcomp->pool = NULL;
    }
  }
  gst_caps_features_free (ift);

  return GST_AGGREGATOR_CLASS (parent_class)->negotiated_src_caps (agg, caps);
}

static gboolean
gst_nvcompositor_decide_allocation (GstAggregator * agg, GstQuery * query)
{
  guint j, metas_no;
  GstCaps *outcaps = NULL;
  GstCaps *myoutcaps = NULL;
  GstBufferPool *pool = NULL;
  guint size, minimum, maximum;
  GstAllocator *allocator = NULL;
  GstAllocationParams params = { 0, 0, 0, 0 };
  GstStructure *config = NULL;
  GstVideoInfo info;
  GstVideoAggregator *vagg = GST_VIDEO_AGGREGATOR (agg);
  GstNvCompositor *nvcomp = GST_NVCOMPOSITOR (vagg);

  metas_no = gst_query_get_n_allocation_metas (query);
  for (j = 0; j < metas_no; j++) {
    gboolean remove_meta;
    GType meta_api;
    const GstStructure *param_str = NULL;

    meta_api = gst_query_parse_nth_allocation_meta (query, j, &param_str);

    if (gst_meta_api_type_has_tag (meta_api, GST_META_TAG_MEMORY)) {
      /* Different memory will get allocated for input and output.
         remove all memory dependent metadata */
      GST_DEBUG_OBJECT (nvcomp, "remove memory specific metadata %s",
          g_type_name (meta_api));
      remove_meta = TRUE;
    } else {
      /* Default remove all metadata */
      GST_DEBUG_OBJECT (nvcomp, "remove metadata %s", g_type_name (meta_api));
      remove_meta = TRUE;
    }

    if (remove_meta) {
      gst_query_remove_nth_allocation_meta (query, j);
      j--;
      metas_no--;
    }
  }

  gst_query_parse_allocation (query, &outcaps, NULL);
  if (outcaps == NULL)
    goto no_caps;

  if (nvcomp->nvcomppool) {
    pool = nvcomp->pool;
    if (pool)
      gst_object_ref (pool);

    if (pool != NULL) {
      config = gst_buffer_pool_get_config (pool);
      gst_buffer_pool_config_get_params (config, &myoutcaps, &size, NULL, NULL);

      GST_DEBUG_OBJECT (nvcomp, "we have a pool with caps %" GST_PTR_FORMAT,
          myoutcaps);

      if (!gst_caps_is_equal (outcaps, myoutcaps)) {
        /* different caps, we can't use current pool */
        GST_DEBUG_OBJECT (nvcomp, "pool has different caps");
        gst_object_unref (pool);
        pool = NULL;
      }
      gst_structure_free (config);
    }

    if (pool == NULL) {
      if (!gst_video_info_from_caps (&info, outcaps))
        goto invalid_caps;

      size = info.size;
      minimum = NV_COMPOSITOR_MAX_BUF;

      GST_DEBUG_OBJECT (nvcomp, "create new pool");

      pool = gst_buffer_pool_new ();

      config = gst_buffer_pool_get_config (pool);
      gst_buffer_pool_config_set_params (config, outcaps, NvBufferGetSize(), minimum, minimum);
      allocator = gst_nv_compositor_allocator_new (nvcomp->out_width, nvcomp->out_height, nvcomp->out_pix_fmt);

      gst_buffer_pool_config_set_allocator (config, allocator, &params);
      if (!gst_buffer_pool_set_config (pool, config))
        goto config_failed;

      nvcomp->pool = gst_object_ref (pool);
      gst_object_unref (allocator);
    }

    if (pool) {
      config = gst_buffer_pool_get_config (pool);
      gst_buffer_pool_config_get_allocator (config, &allocator, &params);
      gst_buffer_pool_config_get_params (config, &myoutcaps, &size, &minimum, &maximum);

      /* Add check, params may be empty e.g. fakesink */
      if (gst_query_get_n_allocation_params (query) > 0)
        gst_query_set_nth_allocation_param (query, 0, allocator, &params);
      else
        gst_query_add_allocation_param (query, allocator, &params);

      if (gst_query_get_n_allocation_pools (query) > 0)
        gst_query_set_nth_allocation_pool (query, 0, pool, size, minimum, maximum);
      else
        gst_query_add_allocation_pool (query, pool, size, minimum, maximum);

      gst_structure_free (config);
      gst_object_unref (pool);
    }
  } else {
    goto not_supported_outcaps;
  }

  return TRUE;
/* ERROR */
no_caps:
  {
    GST_ERROR ("no caps specified");
    return FALSE;
  }
not_supported_outcaps:
  {
    GST_ERROR ("not supported out caps");
    return FALSE;
  }
invalid_caps:
  {
    GST_ERROR ("invalid caps specified");
    return FALSE;
  }
config_failed:
  {
    GST_ERROR ("failed to set config on bufferpool");
    return FALSE;
  }
}

static gboolean
gst_nvcompositor_stop (GstAggregator * agg)
{
  GstVideoAggregator *vagg = GST_VIDEO_AGGREGATOR (agg);
  GstNvCompositor *nvcomp = GST_NVCOMPOSITOR (vagg);

  if (nvcomp->pool) {
    gst_object_unref (nvcomp->pool);
    nvcomp->pool = NULL;
  }

  return TRUE;
}

/**
  * get rgb color for background.
  *
  * @param nvcomp : GstNvCompositor object instance
  */
static void
get_bg_color (GstNvCompositor * nvcomp)
{
  switch (nvcomp->background) {
    case NVCOMPOSITOR_BACKGROUND_BLACK:
      nvcomp->bg.r = 0.0;
      nvcomp->bg.g = 0.0;
      nvcomp->bg.b = 0.0;
      break;
    case NVCOMPOSITOR_BACKGROUND_RED:
      nvcomp->bg.r = 1.0;
      nvcomp->bg.g = 0.0;
      nvcomp->bg.b = 0.0;
      break;
    case NVCOMPOSITOR_BACKGROUND_GREEN:
      nvcomp->bg.r = 0.0;
      nvcomp->bg.g = 1.0;
      nvcomp->bg.b = 0.0;
      break;
    case NVCOMPOSITOR_BACKGROUND_BLUE:
      nvcomp->bg.r = 0.0;
      nvcomp->bg.g = 0.0;
      nvcomp->bg.b = 1.0;
      break;
    case NVCOMPOSITOR_BACKGROUND_WHITE:
      nvcomp->bg.r = 1.0;
      nvcomp->bg.g = 1.0;
      nvcomp->bg.b = 1.0;
      break;
    default:
      nvcomp->bg.r = 0.0;
      nvcomp->bg.g = 0.0;
      nvcomp->bg.b = 0.0;
      break;
  }
}

/**
  * composite NvBuffers.
  *
  * @param vagg : GstVideoAggregator object instance
  * @param out_dmabuf_fd : output Nvbuffer dmabuf fd
  */
static gboolean
do_nvcomposite (GstVideoAggregator * vagg, gint out_dmabuf_fd)
{
  gint i = 0;
  guint all_yuv = 0;
  GList *l;
  gint ret = 0;
  gint input_dmabuf_fds[MAX_INPUT_FRAME] = {-1, -1, -1, -1, -1, -1};
  gint input_dmabuf_count = 0;
  gint releasefd_index[MAX_INPUT_FRAME] = { 0 };
  GstMemory *inmem = NULL;
  GstMapInfo inmap = GST_MAP_INFO_INIT;

  GstNvCompositor *nvcomp = GST_NVCOMPOSITOR (vagg);

  for (l = GST_ELEMENT (vagg)->sinkpads; l; l = l->next) {
    GstVideoAggregatorPad *pad = l->data;
    if (!pad->buffer)
      continue;

    GstNvCompositorPad *compo_pad = GST_NVCOMPOSITOR_PAD (pad);

    inmem = gst_buffer_peek_memory (pad->buffer, 0);
    if (!inmem) {
      GST_ERROR ("no input memory block");
      return FALSE;
    }

    if (!g_strcmp0 (inmem->allocator->mem_type, GST_OMX_MEMORY_TYPE) ||
        !g_strcmp0 (inmem->allocator->mem_type, GST_NV_FILTER_MEMORY_TYPE) ||
        !g_strcmp0 (inmem->allocator->mem_type, GST_NVARGUS_MEMORY_TYPE) ||
        !g_strcmp0 (inmem->allocator->mem_type, GST_NV_V4L2_MEMORY_TYPE)) {
      if (!gst_buffer_map (pad->buffer, &inmap, GST_MAP_READ)) {
        GST_ERROR ("input buffer mapinfo failed");
        return FALSE;
      }

      ret = ExtractFdFromNvBuffer (inmap.data, &input_dmabuf_fds[i]);
      if (ret != 0) {
        GST_ERROR ("ExtractFdFromNvBuffer failed");
        return FALSE;
      }
      if (!g_strcmp0 (inmem->allocator->mem_type,
                      GST_OMX_MEMORY_TYPE)) {
          releasefd_index[i] = 1;
      }
    } else if (!g_strcmp0 (inmem->allocator->mem_type, GST_ALLOCATOR_SYSMEM)) {
      input_dmabuf_fds[i] = compo_pad->comppad_buf.pad_dmabuf_fd;
    } else {
      GST_ERROR ("input buffer not supported");
      return FALSE;
    }

    if (input_dmabuf_fds[i] == -1) {
      GST_ERROR ("input buffer invalid");
      return FALSE;
    }

    nvcomp->comp_params.src_comp_rect[i].left = 0;
    nvcomp->comp_params.src_comp_rect[i].top = 0;
    nvcomp->comp_params.src_comp_rect[i].width = compo_pad->input_width;
    nvcomp->comp_params.src_comp_rect[i].height = compo_pad->input_height;

    nvcomp->comp_params.dst_comp_rect[i].left = compo_pad->xpos;
    nvcomp->comp_params.dst_comp_rect[i].top = compo_pad->ypos;
    if (compo_pad->width) {
      nvcomp->comp_params.dst_comp_rect[i].width = compo_pad->width;
    } else {
      nvcomp->comp_params.dst_comp_rect[i].width = compo_pad->input_width;
    }

    if (compo_pad->height) {
      nvcomp->comp_params.dst_comp_rect[i].height = compo_pad->height;
    } else {
      nvcomp->comp_params.dst_comp_rect[i].height = compo_pad->input_height;
    }

    nvcomp->comp_params.dst_comp_rect_alpha[i] = (gfloat) compo_pad->alpha;
    if (compo_pad->comppad_pix_fmt != NvBufferColorFormat_ABGR32) {
      all_yuv = 1;
    }

    switch(compo_pad->interpolation_method)
    {
      case GST_INTERPOLATION_NEAREST:
          nvcomp->comp_params.composite_filter[i] = NvBufferTransform_Filter_Nearest;
        break;
      case GST_INTERPOLATION_BILINEAR:
          nvcomp->comp_params.composite_filter[i] = NvBufferTransform_Filter_Bilinear;
        break;
      case GST_INTERPOLATION_5_TAP:
          nvcomp->comp_params.composite_filter[i] = NvBufferTransform_Filter_5_Tap;
        break;
      case GST_INTERPOLATION_10_TAP:
          nvcomp->comp_params.composite_filter[i] = NvBufferTransform_Filter_10_Tap;
        break;
      case GST_INTERPOLATION_SMART:
          nvcomp->comp_params.composite_filter[i] = NvBufferTransform_Filter_Smart;
        break;
      case GST_INTERPOLATION_NICEST:
          nvcomp->comp_params.composite_filter[i] = NvBufferTransform_Filter_Nicest;
        break;
      default:
          nvcomp->comp_params.composite_filter[i] = NvBufferTransform_Filter_Smart;
        break;
    }

    if (inmap.data) {
      gst_buffer_unmap (pad->buffer, &inmap);
    }

    input_dmabuf_count += 1;
    i++;
  }
  nvcomp->comp_params.input_buf_count = input_dmabuf_count;

  if (!all_yuv && (nvcomp->out_pix_fmt == NvBufferColorFormat_ABGR32)) {
    nvcomp->comp_params.composite_flag |= NVBUFFER_BLEND;
  }

  nvcomp->comp_params.composite_flag |= NVBUFFER_COMPOSITE;
  nvcomp->comp_params.composite_flag |= NVBUFFER_COMPOSITE_FILTER;

  if (!(nvcomp->comp_params.composite_flag & NVBUFFER_BLEND)) {
    get_bg_color (nvcomp);
    nvcomp->comp_params.composite_bgcolor.r = nvcomp->bg.r;
    nvcomp->comp_params.composite_bgcolor.g = nvcomp->bg.g;
    nvcomp->comp_params.composite_bgcolor.b = nvcomp->bg.b;
  }

  ret = NvBufferComposite (input_dmabuf_fds, out_dmabuf_fd, &nvcomp->comp_params);
  if (ret != 0) {
    GST_ERROR ("NvBufferComposite failed");
    return FALSE;
  }

  for (i = 0; i < MAX_INPUT_FRAME; i++) {
    if (releasefd_index[i] == 1) {
      ret = NvReleaseFd (input_dmabuf_fds[i]);
      if (ret != 0) {
        GST_ERROR ("NvReleaseFd failed");
        return FALSE;
      }
    }
  }

  return TRUE;
}

/**
  * aggregate frames that are ready.
  *
  * @param vagg : GstVideoAggregator object instance
  * @param outbuf : output buffer
  */
static GstFlowReturn
gst_nvcompositor_aggregate_frames (GstVideoAggregator * vagg,
    GstBuffer * outbuf)
{
  GstFlowReturn flow_ret = GST_FLOW_OK;
  GstMemory *outmem = NULL;
  GstNvCompositorMemory *omem = NULL;
  GstMapInfo outmap = GST_MAP_INFO_INIT;

  outmem = gst_buffer_peek_memory (outbuf, 0);
  if (!outmem) {
    GST_ERROR_OBJECT (vagg, "output buffer peek memory failed");
    flow_ret = GST_FLOW_ERROR;
    goto no_memory;
  }
  omem = (GstNvCompositorMemory *) outmem;

  if (g_strcmp0 (outmem->allocator->mem_type, GST_NV_COMPOSITOR_MEMORY_TYPE)) {
    GST_ERROR_OBJECT (vagg,
        "outmem_type is not of type GST_NV_COMPOSITOR_MEMORY_TYPE");
    flow_ret = GST_FLOW_ERROR;
    goto invalid_outmem;
  }

  if (!gst_buffer_map (outbuf, &outmap, GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (vagg, "output buffer map failed");
    flow_ret = GST_FLOW_ERROR;
    goto invalid_outbuf;
  }

  GST_OBJECT_LOCK (vagg);

  /* Nv Composition function */
  if (!do_nvcomposite (vagg, omem->buf->dmabuf_fd)) {
    GST_ERROR_OBJECT (vagg, "Failed to composit frames");
    flow_ret = GST_FLOW_ERROR;
    goto done;
  }

done:
  GST_OBJECT_UNLOCK (vagg);
  gst_buffer_unmap (outbuf, &outmap);

  return flow_ret;

  /* ERRORS */
no_memory:
  {
    GST_ERROR ("no memory block");
    return flow_ret;
  }
invalid_outmem:
  {
    GST_ERROR ("outmem type invalid");
    return GST_FLOW_ERROR;
  }
invalid_outbuf:
  {
    GST_ERROR ("output buffer invalid");
    return GST_FLOW_ERROR;
  }
}

/**
  * query functon for sink pad.
  *
  * @param agg : GstAggregator object instance
  * @param bpad : GstAggregatorPad object instance
  * @param query : query to handle
  */
static gboolean
gst_nvcompositor_sink_query (GstAggregator * agg, GstAggregatorPad * bpad,
    GstQuery * query)
{
  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_ALLOCATION:{
      GstCaps *caps;
      GstVideoInfo info;
      GstBufferPool *pool;
      guint size;
      GstStructure *structure;

      gst_query_parse_allocation (query, &caps, NULL);

      if (caps == NULL)
        return FALSE;

      if (!gst_video_info_from_caps (&info, caps))
        return FALSE;

      size = GST_VIDEO_INFO_SIZE (&info);

      pool = gst_video_buffer_pool_new ();

      structure = gst_buffer_pool_get_config (pool);
      gst_buffer_pool_config_set_params (structure, caps, size, 0, 0);

      if (!gst_buffer_pool_set_config (pool, structure)) {
        gst_object_unref (pool);
        return FALSE;
      }

      gst_query_add_allocation_pool (query, pool, size, 0, 0);
      gst_object_unref (pool);
      gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

      return TRUE;
    }
    default:
      return GST_AGGREGATOR_CLASS (parent_class)->sink_query (agg, bpad, query);
  }
}

/**
  * initialize the nvcompositor's class.
  *
  * @param klass : GstNvCompositorClass objectclass
  */
static void
gst_nvcompositor_class_init (GstNvCompositorClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstVideoAggregatorClass *videoaggregator_class =
      (GstVideoAggregatorClass *) klass;
  GstAggregatorClass *agg_class = (GstAggregatorClass *) klass;

  gparent_class = g_type_class_peek_parent (agg_class);

  gobject_class->get_property = gst_nvcompositor_get_property;
  gobject_class->set_property = gst_nvcompositor_set_property;

  agg_class->sink_query = gst_nvcompositor_sink_query;
  agg_class->fixate_src_caps = gst_nvcompositor_fixate_caps;
  agg_class->negotiated_src_caps = gst_nvcompositor_negotiated_caps;
  agg_class->decide_allocation = gst_nvcompositor_decide_allocation;
  agg_class->stop = gst_nvcompositor_stop;

  videoaggregator_class->aggregate_frames = gst_nvcompositor_aggregate_frames;

  g_object_class_install_property (gobject_class, PROP_BACKGROUND,
      g_param_spec_enum ("background", "Background", "Background type",
          GST_TYPE_NVCOMPOSITOR_BACKGROUND,
          DEFAULT_BACKGROUND, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

/* TODO: Replace background property static enum with rgb boxed array */
#if 0
  g_object_class_install_property (gobject_class, PROP_BACKGROUND,
      g_param_spec_boxed ("background", "background color",
          "Property to set background color for composition.\n"
          "\t\t\t Use GArray, with values of background color (R, G, B)\n"
          "\t\t\t in that order, to set the property.\n"
          "\t\t\t This will have effect only when compositing without blending.",
          G_TYPE_ARRAY,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
#endif

  gst_element_class_add_static_pad_template_with_gtype (gstelement_class,
      &src_factory, GST_TYPE_AGGREGATOR_PAD);
  gst_element_class_add_static_pad_template_with_gtype (gstelement_class,
      &sink_factory, GST_TYPE_NVCOMPOSITOR_PAD);

  gst_element_class_set_static_metadata (gstelement_class,
      "NvCompositor",
      "Filter/Video/Compositor",
      "Composite multiple video frames", "Amit Pandya <apandya@nvidia.com>");
}

/**
  * initialize nvcompositor instance structure..
  *
  * @param nvcomp : GstNvCompositor object instance
  */
static void
gst_nvcompositor_init (GstNvCompositor * nvcomp)
{
  nvcomp->background = NVCOMPOSITOR_BACKGROUND_BLACK;
  nvcomp->bg.r = 0;
  nvcomp->bg.g = 0;
  nvcomp->bg.b = 0;
  nvcomp->pool = NULL;
  memset(&nvcomp->comp_params, 0, sizeof(NvBufferCompositeParams));
}

/* NvCompositor Element registration */
static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_nvcompositor_debug, "nvcompositor", 0,
      "nvcompositor");

  return gst_element_register (plugin, "nvcompositor", GST_RANK_PRIMARY + 1,
      GST_TYPE_NVCOMPOSITOR);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    nvcompositor,
    PACKAGE_DESCRIPTION, plugin_init, PACKAGE_VERSION, PACKAGE_LICENSE,
    PACKAGE_NAME, PACKAGE_ORIGIN)
