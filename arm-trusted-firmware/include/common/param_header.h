/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PARAM_HEADER_H__
#define __PARAM_HEADER_H__

/* Param header types */
#define PARAM_EP		0x01U
#define PARAM_IMAGE_BINARY	0x02U
#define PARAM_BL31		0x03U
#define PARAM_BL_LOAD_INFO	0x04U
#define PARAM_BL_PARAMS		0x05U
#define PARAM_PSCI_LIB_ARGS	0x06U

/* Param header version */
#define VERSION_1	0x01U
#define VERSION_2	0x02U

#define SET_PARAM_HEAD(_p, _type, _ver, _attr) do { \
	(_p)->h.type = (uint8_t)(_type); \
	(_p)->h.version = (uint8_t)(_ver); \
	(_p)->h.size = (uint16_t)sizeof(*_p); \
	(_p)->h.attr = (uint32_t)(_attr) ; \
	} while (0)

/* Following is used for populating structure members statically. */
#define SET_STATIC_PARAM_HEAD(_p, _type, _ver, _p_type, _attr)	\
	._p.h.type = (uint8_t)(_type), \
	._p.h.version = (uint8_t)(_ver), \
	._p.h.size = (uint16_t)sizeof(_p_type), \
	._p.h.attr = (uint32_t)(_attr)

#ifndef __ASSEMBLY__

#include <types.h>

/***************************************************************************
 * This structure provides version information and the size of the
 * structure, attributes for the structure it represents
 ***************************************************************************/
typedef struct param_header {
	uint8_t type;		/* type of the structure */
	uint8_t version;    /* version of this structure */
	uint16_t size;      /* size of this structure in bytes */
	uint32_t attr;      /* attributes: unused bits SBZ */
} param_header_t;

#endif /*__ASSEMBLY__*/

#endif /* __PARAM_HEADER_H__ */

