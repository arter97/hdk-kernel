/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 */


#ifndef _MSM_CVP_CLOCKS_H_
#define _MSM_CVP_CLOCKS_H_
#include "msm_cvp_internal.h"
#include "cvp_core_hfi.h"

int cvp_msm_cvp_set_clocks(struct msm_cvp_core *core);
int cvp_msm_cvp_mmrm_register(struct iris_hfi_device *device);
int cvp_msm_cvp_mmrm_set_value_in_range(struct iris_hfi_device *device,
		u32 freq_min, u32 freq_cur);
int cvp_msm_cvp_set_clocks_impl(struct iris_hfi_device *device, u32 freq);
int cvp_msm_cvp_scale_clocks(struct iris_hfi_device *device);
int cvp_msm_cvp_prepare_enable_clk(struct iris_hfi_device *device,
		const char *name);
int cvp_msm_cvp_disable_unprepare_clk(struct iris_hfi_device *device,
		const char *name);
int cvp_msm_cvp_init_clocks(struct iris_hfi_device *device);
void cvp_msm_cvp_deinit_clocks(struct iris_hfi_device *device);
#endif
