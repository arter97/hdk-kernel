/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 */

#ifndef _MSM_V4L2_PRIVATE_H_
#define _MSM_V4L2_PRIVATE_H_

#include <media/msm_eva_private.h>
#include "msm_cvp_debug.h"

long eva_cvp_unblocked_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg);

long eva_cvp_compat_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg);

#endif
