/* GStreamer
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_OMX_MPEGAUDIO_DEC_H_
#define _GST_OMX_MPEGAUDIO_DEC_H_

#include <gst/gst.h>
#include "gstomxaudiodec.h"

G_BEGIN_DECLS
#define GST_TYPE_OMX_MPEGAUDIO_DEC   (gst_omx_mpegaudio_dec_get_type())
#define GST_OMX_MPEGAUDIO_DEC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_OMX_MPEGAUDIO_DEC,GstOMXMPEGAUDIODec))
#define GST_OMX_MPEGAUDIO_DEC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_OMX_MPEGAUDIO_DEC,GstOMXMPEGAUDIODecClass))
#define GST_IS_OMX_MPEGAUDIO_DEC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_OMX_MPEGAUDIO_DEC))
#define GST_IS_OMX_MPEGAUDIO_DEC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_OMX_MPEGAUDIO_DEC))
typedef struct _GstOMXMPEGAUDIODec GstOMXMPEGAUDIODec;
typedef struct _GstOMXMPEGAUDIODecClass GstOMXMPEGAUDIODecClass;

struct _GstOMXMPEGAUDIODec
{
  GstOMXAudioDec parent;

};

struct _GstOMXMPEGAUDIODecClass
{
  GstOMXAudioDecClass parent_class;
};

GType gst_omx_mpegaudio_dec_get_type (void);

G_END_DECLS
#endif
