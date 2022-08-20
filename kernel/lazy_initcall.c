// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Juhyung Park <qkrwngud825@gmail.com>
 *
 * Partially based on kernel/module.c.
 */

#ifdef CONFIG_LAZY_INITCALL_DEBUG
#define DEBUG
#define __fatal pr_err
#else
#define __fatal panic
#endif

#define pr_fmt(fmt) "lazy_initcall: " fmt

#include <linux/syscalls.h>
#include <linux/kmemleak.h>
#include <uapi/linux/module.h>

#include "module-internal.h"

#ifdef CONFIG_LAZY_INITCALL_DEBUG
static void __init show_unused_modules(struct work_struct *unused);
static __initdata DECLARE_DELAYED_WORK(show_unused_work, show_unused_modules);
#endif
static DEFINE_MUTEX(lazy_initcall_mutex);
static bool completed;

/*
 * Why is this here, instead of defconfig?
 *
 * Data used in defconfig isn't freed in free_initmem() and putting a list this
 * big into the defconfig isn't really ideal anyways.
 *
 * Since lazy_initcall isn't meant to be generic, set this here.
 *
 * This list is generatable by putting .ko modules from vendor, vendor_boot and
 * vendor_dlkm to a directory and running the following:
 *
 * MODDIR=/path/to/modules
 * find "$MODDIR" -name '*.ko' -exec modinfo {} + | grep '^name:' | awk '{print $2}' | sort | uniq | while read f; do printf '\t"'$f'",\n'; done
 * find "$MODDIR" -name '*.ko' | while read f; do if ! modinfo $f | grep -q "^name:"; then n=$(basename $f); n="${n%.*}"; printf '\t"'$n'",\n'; fi; done | sort | uniq
 */

static const __initconst char * const targets_list[] = {
	"adsp_loader_dlkm",
	"adsp_sleepmon",
	"altmode_glink",
	"arm_smmu",
	"atmel_mxt_ts",
	"audio_pkt_dlkm",
	"audio_prm_dlkm",
	"audpkt_ion_dlkm",
	"bam_dma",
	"bcl_pmic5",
	"bcl_soc",
	"boot_stats",
	"bt_fm_slim",
	"btpower",
	"bwmon",
	"c1dcvs_scmi",
	"c1dcvs_vendor",
	"camcc_diwali",
	"camcc_waipio",
	"camera",
	"cdsp_loader",
	"cdsprm",
	"cfg80211",
	"charger_ulog_glink",
	"clk_dummy",
	"clk_qcom",
	"clk_rpmh",
	"cmd_db",
	"cnss2",
	"cnss_nl",
	"cnss_plat_ipc_qmi_svc",
	"cnss_prealloc",
	"cnss_utils",
	"core_hang_detect",
	"coresight",
	"coresight_csr",
	"coresight_cti",
	"coresight_dummy",
	"coresight_funnel",
	"coresight_hwevent",
	"coresight_remote_etm",
	"coresight_replicator",
	"coresight_stm",
	"coresight_tgu",
	"coresight_tmc",
	"coresight_tpda",
	"coresight_tpdm",
	"cpu_hotplug",
	"cpu_voltage_cooling",
	"cqhci",
	"crypto_qti_common",
	"crypto_qti_hwkm",
	"dcc_v2",
	"dcvs_fp",
	"ddr_cdev",
	"debugcc_diwali",
	"debugcc_waipio",
	"debug_regulator",
	"dispcc_diwali",
	"dispcc_waipio",
	"dwc3_msm",
	"ehset",
	"ep_pcie_drv",
	"eud",
	"f_fs_ipc_log",
	"focaltech_fts",
	"frpc_adsprpc",
	"fsa4480_i2c",
	"gcc_diwali",
	"gcc_waipio",
	"gdsc_regulator",
	"gh_arm_drv",
	"gh_ctrl",
	"gh_dbl",
	"gh_irq_lend",
	"gh_mem_notifier",
	"gh_msgq",
	"gh_rm_drv",
	"gh_virtio_backend",
	"gh_virt_wdt",
	"glink_pkt",
	"glink_probe",
	"goodix_core",
	"gpi",
	"gplaf_scmi",
	"gplaf_vendor",
	"gpr_dlkm",
	"gpucc_diwali",
	"gpucc_waipio",
	"gsim",
	"guestvm_loader",
	"hdcp_qseecom",
	"hdmi_dlkm",
	"heap_mem_ext_v01",
	"hung_task_enh",
	"hvc_gunyah",
	"hwkm",
	"hyp_core_ctl",
	"i2c_msm_geni",
	"i3c_master_msm_geni",
	"icc_bcm_voter",
	"icc_debug",
	"icc_rpmh",
	"icc_test",
	"icnss2",
	"iommu_logger",
	"ipa_clientsm",
	"ipa_fmwk",
	"ipam",
	"ipanetm",
	"kiwi",
	"kryo_arm64_edac",
	"leds_qpnp_vibrator_ldo",
	"leds_qti_flash",
	"leds_qti_tri_led",
	"llcc_perfmon",
	"llcc_qcom",
	"lpass_cdc_dlkm",
	"lpass_cdc_rx_macro_dlkm",
	"lpass_cdc_tx_macro_dlkm",
	"lpass_cdc_va_macro_dlkm",
	"lpass_cdc_wsa2_macro_dlkm",
	"lpass_cdc_wsa_macro_dlkm",
	"lt9611uxc",
	"lvstest",
	"mac80211",
	"machine_dlkm",
	"mbhc_dlkm",
	"mdt_loader",
	"mem_buf",
	"mem_buf_dev",
	"mem_hooks",
	"memlat",
	"mem_offline",
	"memory_dump_v2",
	"mhi",
	"mhi_cntrl_qcom",
	"mhi_dev_drv",
	"mhi_dev_dtr",
	"mhi_dev_net",
	"mhi_dev_netdev",
	"mhi_dev_uci",
	"microdump_collector",
	"minidump",
	"mmrm_test_module",
	"msm_cvp",
	"msm_dma_iommu_mapping",
	"msm_drm",
	"msm_eva",
	"msm_ext_display",
	"msm_geni_se",
	"msm_geni_serial",
	"msm_kgsl",
	"msm_lmh_dcvs",
	"msm_memshare",
	"msm_mmrm",
	"msm_performance",
	"msm_qmp",
	"msm_rng",
	"msm_sharedmem",
	"msm_show_epoch",
	"msm_show_resume_irq",
	"msm_sysstats",
	"msm_video",
	"nfc_i2c",
	"ns",
	"nt36xxx_i2c",
	"nt36xxx_spi",
	"nvmem_qcom_spmi_sdam",
	"nvmem_qfprom",
	"panel_event_notifier",
	"pci_edma",
	"pci_msm_drv",
	"pdr_interface",
	"phy_generic",
	"phy_msm_snps_eusb2",
	"phy_msm_snps_hs",
	"phy_msm_ssusb_qmp",
	"phy_qcom_emu",
	"phy_qcom_ufs",
	"phy_qcom_ufs_qmp_14nm",
	"phy_qcom_ufs_qmp_v3",
	"phy_qcom_ufs_qmp_v4_cape",
	"phy_qcom_ufs_qmp_v4_diwali",
	"phy_qcom_ufs_qmp_v4_lahaina",
	"phy_qcom_ufs_qmp_v4_parrot",
	"phy_qcom_ufs_qmp_v4_waipio",
	"pinctrl_cape",
	"pinctrl_diwali",
	"pinctrl_lpi_dlkm",
	"pinctrl_msm",
	"pinctrl_spmi_gpio",
	"pinctrl_spmi_mpp",
	"pinctrl_waipio",
	"plh_scmi",
	"plh_vendor",
	"pm8941_pwrkey",
	"pmic_glink",
	"pmic_pon_log",
	"pmu_scmi",
	"pmu_vendor",
	"policy_engine",
	"proxy_consumer",
	"pwm_qti_lpg",
	"q6_dlkm",
	"q6_notifier_dlkm",
	"q6_pdr_dlkm",
	"qbt_handler",
	"qca6490",
	"qca6750",
	"qce50",
	"qcedev_mod",
	"qcom_aoss",
	"qcom_cpufreq_hw",
	"qcom_cpufreq_hw_debug",
	"qcom_cpuss_sleep_stats",
	"qcom_cpu_vendor_hooks",
	"qcom_dcvs",
	"qcom_dload_mode",
	"qcom_dma_heaps",
	"qcom_edac",
	"qcom_esoc",
	"qcom_gic_intr_routing",
	"qcom_glink",
	"qcom_glink_smem",
	"qcom_glink_spss",
	"qcom_hv_haptics",
	"qcom_hwspinlock",
	"qcom_i2c_pmic",
	"qcom_iommu_debug",
	"qcom_iommu_util",
	"qcom_ipcc",
	"qcom_ipc_lite",
	"qcom_ipc_logging",
	"qcom_llcc_pmu",
	"qcom_logbuf_vendor_hooks",
	"qcom_lpm",
	"qcom_pdc",
	"qcom_pil_info",
	"qcom_pm8008_regulator",
	"qcom_pmu_lib",
	"qcom_pon",
	"qcom_q6v5",
	"qcom_q6v5_pas",
	"qcom_ramdump",
	"qcom_reboot_reason",
	"qcom_rimps",
	"qcom_rpmh",
	"qcom_scm",
	"qcom_smd",
	"qcom_soc_wdt",
	"qcom_spmi_adc5",
	"qcom_spmi_pmic",
	"qcom_spmi_temp_alarm",
	"qcom_spss",
	"qcom_sync_file",
	"qcom_sysmon",
	"qcom_tsens",
	"qcom_vadc_common",
	"qcom_wdt_core",
	"qdss_bridge",
	"qfprom_sys",
	"qmi_helpers",
	"qnoc_diwali",
	"qnoc_parrot",
	"qnoc_qos",
	"qnoc_waipio",
	"qpnp_amoled_regulator",
	"qpnp_pbs",
	"qrtr",
	"qrtr_gunyah",
	"qrtr_mhi",
	"qrtr_smd",
	"qseecom_mod",
	"qsee_ipc_irq_bridge",
	"qti_adc_tm",
	"qti_amoled_ecm",
	"qti_battery_charger",
	"qti_battery_debug",
	"qti_cpufreq_cdev",
	"qti_devfreq_cdev",
	"qti_fixed_regulator",
	"qti_qmi_cdev",
	"qti_qmi_sensor_v2",
	"qti_regmap_debugfs",
	"qti_userspace_cdev",
	"radio_i2c_rtc6226_qca",
	"rdbg",
	"reboot_mode",
	"regmap_spmi",
	"repeater",
	"repeater_i2c_eusb2",
	"rimps_log",
	"rmnet_aps",
	"rmnet_core",
	"rmnet_ctl",
	"rmnet_offload",
	"rmnet_perf",
	"rmnet_perf_tether",
	"rmnet_sch",
	"rmnet_shs",
	"rmnet_wlan",
	"rndisipam",
	"rpmh_regulator",
	"rproc_qcom_common",
	"rq_stats",
	"rtc_pm8xxx",
	"sched_walt",
	"sdhci_msm",
	"sdpm_clk",
	"secure_buffer",
	"sensors_ssc",
	"slimbus",
	"slim_qcom_ngd_ctrl",
	"smcinvoke_mod",
	"smem",
	"smp2p",
	"smp2p_sleepstate",
	"snd_event_dlkm",
	"snd_usb_audio_qmi",
	"socinfo",
	"soc_sleep_stats",
	"spcom",
	"spf_core_dlkm",
	"spi_msm_geni",
	"spmi_glink_debug",
	"spmi_pmic_arb",
	"spmi_pmic_arb_debug",
	"sps_drv",
	"spss_utils",
	"ssusb_redriver_nb7vpq904m",
	"stm_console",
	"stm_core",
	"stm_ftrace",
	"stm_p_basic",
	"stm_p_ost",
	"stub_dlkm",
	"stub_regulator",
	"subsystem_sleep_stats",
	"swr_ctrl_dlkm",
	"swr_dlkm",
	"swr_dmic_dlkm",
	"swr_haptics_dlkm",
	"synaptics_dsx",
	"synx_driver",
	"sysmon_subsystem_stats",
	"sys_pm_vx",
	"thermal_pause",
	"tmecom_intf",
	"tz_log",
	"ucsi_glink",
	"ufshcd_crypto_qti",
	"ufs_qcom",
	"usb_bam",
	"usb_f_ccid",
	"usb_f_cdev",
	"usb_f_diag",
	"usb_f_gsi",
	"usb_f_qdss",
	"videocc_diwali",
	"videocc_waipio",
	"wcd937x_dlkm",
	"wcd937x_slave_dlkm",
	"wcd938x_dlkm",
	"wcd938x_slave_dlkm",
	"wcd9xxx_dlkm",
	"wcd_core_dlkm",
	"wlan_firmware_service",
	"wsa883x_dlkm",
	"zram",
	"hwmon",
	"lzo",
	"lzo-rle",
	"sg",
	"zsmalloc",
	NULL
};

/*
 * Some drivers don't have module_init(), which will lead to lookup failure
 * from lazy_initcall when a module with the same name is asked to be loaded
 * from the userspace.
 *
 * Lazy initcall can optionally maintain a list of kernel drivers built into
 * the kernel that matches the name from the firmware so that those aren't
 * treated as errors.
 *
 * Ew, is this the best approach?
 *
 * Detecting the presense of .init.text section or initcall_t function is
 * unreliable as .init.text might not exist when a driver doesn't use __init
 * and modpost adds an empty .init stub even if a driver doesn't declare a
 * function for module_init().
 *
 * This list is generatable by putting .ko modules from vendor, vendor_boot and
 * vendor_dlkm to a directory, cd'ing to kernel's O directory and running the
 * following:
 *
 * MODDIR=/path/to/modules
 * find -name '*.o' | tr '-' '_' > list
 * find "$MODDIR" -name '*.ko' -exec modinfo {} + | grep '^name:' | awk '{print $2}' | sort | uniq | while read f; do if grep -q /"$f".o list; then printf '\t"'$f'",\n'; fi; done
 */
static const __initconst char * const builtin_list[] = {
	"adsp_sleepmon",
	"altmode_glink",
	"arm_smmu",
	"atmel_mxt_ts",
	"bam_dma",
	"bcl_pmic5",
	"bcl_soc",
	"boot_stats",
	"bt_fm_slim",
	"btpower",
	"bwmon",
	"c1dcvs_scmi",
	"c1dcvs_vendor",
	"camcc_diwali",
	"camcc_waipio",
	"cdsp_loader",
	"cdsprm",
	"cfg80211",
	"charger_ulog_glink",
	"clk_dummy",
	"clk_qcom",
	"clk_rpmh",
	"cmd_db",
	"cnss2",
	"cnss_nl",
	"cnss_plat_ipc_qmi_svc",
	"cnss_prealloc",
	"cnss_utils",
	"core_hang_detect",
	"coresight",
	"coresight_csr",
	"coresight_cti",
	"coresight_dummy",
	"coresight_funnel",
	"coresight_hwevent",
	"coresight_remote_etm",
	"coresight_replicator",
	"coresight_stm",
	"coresight_tgu",
	"coresight_tmc",
	"coresight_tpda",
	"coresight_tpdm",
	"cpu_hotplug",
	"cpu_voltage_cooling",
	"cqhci",
	"crypto_qti_common",
	"crypto_qti_hwkm",
	"dcc_v2",
	"dcvs_fp",
	"ddr_cdev",
	"debugcc_diwali",
	"debugcc_waipio",
	"debug_regulator",
	"dispcc_diwali",
	"dispcc_waipio",
	"dwc3_msm",
	"ehset",
	"ep_pcie_drv",
	"eud",
	"f_fs_ipc_log",
	"focaltech_fts",
	"frpc_adsprpc",
	"fsa4480_i2c",
	"gcc_diwali",
	"gcc_waipio",
	"gdsc_regulator",
	"gh_arm_drv",
	"gh_ctrl",
	"gh_dbl",
	"gh_irq_lend",
	"gh_mem_notifier",
	"gh_msgq",
	"gh_rm_drv",
	"gh_virtio_backend",
	"gh_virt_wdt",
	"glink_pkt",
	"glink_probe",
	"goodix_core",
	"gpi",
	"gplaf_scmi",
	"gplaf_vendor",
	"gpucc_diwali",
	"gpucc_waipio",
	"guestvm_loader",
	"hdcp_qseecom",
	"heap_mem_ext_v01",
	"hung_task_enh",
	"hvc_gunyah",
	"hwkm",
	"hyp_core_ctl",
	"i2c_msm_geni",
	"i3c_master_msm_geni",
	"icc_bcm_voter",
	"icc_debug",
	"icc_rpmh",
	"icc_test",
	"icnss2",
	"iommu_logger",
	"ipa_fmwk",
	"kryo_arm64_edac",
	"leds_qpnp_vibrator_ldo",
	"leds_qti_flash",
	"leds_qti_tri_led",
	"llcc_perfmon",
	"llcc_qcom",
	"lt9611uxc",
	"lvstest",
	"mac80211",
	"mdt_loader",
	"mem_buf",
	"mem_buf_dev",
	"mem_hooks",
	"memlat",
	"mem_offline",
	"memory_dump_v2",
	"mhi",
	"mhi_cntrl_qcom",
	"mhi_dev_drv",
	"mhi_dev_dtr",
	"mhi_dev_net",
	"mhi_dev_netdev",
	"mhi_dev_uci",
	"microdump_collector",
	"minidump",
	"msm_dma_iommu_mapping",
	"msm_ext_display",
	"msm_geni_se",
	"msm_geni_serial",
	"msm_kgsl",
	"msm_lmh_dcvs",
	"msm_memshare",
	"msm_mmrm",
	"msm_performance",
	"msm_qmp",
	"msm_rng",
	"msm_sharedmem",
	"msm_show_epoch",
	"msm_show_resume_irq",
	"msm_sysstats",
	"nfc_i2c",
	"ns",
	"nt36xxx_i2c",
	"nt36xxx_spi",
	"nvmem_qcom_spmi_sdam",
	"nvmem_qfprom",
	"panel_event_notifier",
	"pci_edma",
	"pci_msm_drv",
	"pdr_interface",
	"phy_generic",
	"phy_msm_snps_eusb2",
	"phy_msm_snps_hs",
	"phy_msm_ssusb_qmp",
	"phy_qcom_emu",
	"phy_qcom_ufs",
	"phy_qcom_ufs_qmp_14nm",
	"phy_qcom_ufs_qmp_v3",
	"phy_qcom_ufs_qmp_v4_cape",
	"phy_qcom_ufs_qmp_v4_diwali",
	"phy_qcom_ufs_qmp_v4_lahaina",
	"phy_qcom_ufs_qmp_v4_parrot",
	"phy_qcom_ufs_qmp_v4_waipio",
	"pinctrl_cape",
	"pinctrl_diwali",
	"pinctrl_msm",
	"pinctrl_spmi_gpio",
	"pinctrl_spmi_mpp",
	"pinctrl_waipio",
	"plh_scmi",
	"plh_vendor",
	"pm8941_pwrkey",
	"pmic_glink",
	"pmic_pon_log",
	"pmu_scmi",
	"pmu_vendor",
	"policy_engine",
	"proxy_consumer",
	"pwm_qti_lpg",
	"qbt_handler",
	"qce50",
	"qcedev_mod",
	"qcom_aoss",
	"qcom_cpufreq_hw",
	"qcom_cpufreq_hw_debug",
	"qcom_cpuss_sleep_stats",
	"qcom_cpu_vendor_hooks",
	"qcom_dcvs",
	"qcom_dload_mode",
	"qcom_dma_heaps",
	"qcom_edac",
	"qcom_esoc",
	"qcom_gic_intr_routing",
	"qcom_glink",
	"qcom_glink_smem",
	"qcom_glink_spss",
	"qcom_hv_haptics",
	"qcom_hwspinlock",
	"qcom_i2c_pmic",
	"qcom_iommu_debug",
	"qcom_iommu_util",
	"qcom_ipcc",
	"qcom_ipc_lite",
	"qcom_ipc_logging",
	"qcom_llcc_pmu",
	"qcom_logbuf_vendor_hooks",
	"qcom_lpm",
	"qcom_pdc",
	"qcom_pil_info",
	"qcom_pm8008_regulator",
	"qcom_pmu_lib",
	"qcom_pon",
	"qcom_q6v5",
	"qcom_q6v5_pas",
	"qcom_ramdump",
	"qcom_reboot_reason",
	"qcom_rimps",
	"qcom_rpmh",
	"qcom_scm",
	"qcom_smd",
	"qcom_soc_wdt",
	"qcom_spmi_adc5",
	"qcom_spmi_pmic",
	"qcom_spmi_temp_alarm",
	"qcom_spss",
	"qcom_sync_file",
	"qcom_sysmon",
	"qcom_tsens",
	"qcom_vadc_common",
	"qcom_wdt_core",
	"qdss_bridge",
	"qfprom_sys",
	"qmi_helpers",
	"qnoc_diwali",
	"qnoc_parrot",
	"qnoc_qos",
	"qnoc_waipio",
	"qpnp_amoled_regulator",
	"qpnp_pbs",
	"qrtr",
	"qrtr_gunyah",
	"qrtr_mhi",
	"qrtr_smd",
	"qseecom_mod",
	"qsee_ipc_irq_bridge",
	"qti_adc_tm",
	"qti_amoled_ecm",
	"qti_battery_charger",
	"qti_battery_debug",
	"qti_cpufreq_cdev",
	"qti_devfreq_cdev",
	"qti_fixed_regulator",
	"qti_qmi_cdev",
	"qti_qmi_sensor_v2",
	"qti_regmap_debugfs",
	"qti_userspace_cdev",
	"radio_i2c_rtc6226_qca",
	"rdbg",
	"reboot_mode",
	"regmap_spmi",
	"repeater",
	"repeater_i2c_eusb2",
	"rimps_log",
	"rpmh_regulator",
	"rproc_qcom_common",
	"rq_stats",
	"rtc_pm8xxx",
	"sched_walt",
	"sdhci_msm",
	"sdpm_clk",
	"secure_buffer",
	"sensors_ssc",
	"slimbus",
	"slim_qcom_ngd_ctrl",
	"smcinvoke_mod",
	"smem",
	"smp2p",
	"smp2p_sleepstate",
	"snd_usb_audio_qmi",
	"socinfo",
	"soc_sleep_stats",
	"spcom",
	"spi_msm_geni",
	"spmi_glink_debug",
	"spmi_pmic_arb",
	"spmi_pmic_arb_debug",
	"sps_drv",
	"spss_utils",
	"ssusb_redriver_nb7vpq904m",
	"stm_console",
	"stm_core",
	"stm_ftrace",
	"stm_p_basic",
	"stm_p_ost",
	"stub_regulator",
	"subsystem_sleep_stats",
	"synaptics_dsx",
	"synx_driver",
	"sysmon_subsystem_stats",
	"sys_pm_vx",
	"thermal_pause",
	"tmecom_intf",
	"tz_log",
	"ucsi_glink",
	"ufshcd_crypto_qti",
	"ufs_qcom",
	"usb_bam",
	"usb_f_ccid",
	"usb_f_cdev",
	"usb_f_diag",
	"usb_f_gsi",
	"usb_f_qdss",
	"videocc_diwali",
	"videocc_waipio",
	"wlan_firmware_service",
	"zram",
	NULL,
};

/*
 * Some drivers behave differently when it's built-in, so deferring its
 * initialization causes issues.
 *
 * Put those to this blacklist so that it is ignored from lazy_initcall.
 *
 * You can also use this as an ignorelist.
 */
static const __initconst char * const blacklist[] = {
	// Module behavior differences
	"cfg80211",
	"mac80211",

	// Name difference
	"lzo_rle",
	"snd_event_dlkm",

	// Redundant modules
	"qca6750",
	"kiwi",
	"wcd937x_dlkm",
	"wcd937x_slave_dlkm",

	NULL
};

/*
 * You may want some specific drivers to load after all lazy modules have been
 * loaded.
 *
 * Add them here.
 */
static const __initconst char * const deferred_list[] = {
	NULL
};

static struct lazy_initcall __initdata lazy_initcalls[ARRAY_SIZE(targets_list) - ARRAY_SIZE(blacklist) + ARRAY_SIZE(deferred_list)];
static int __initdata counter;

bool __init add_lazy_initcall(initcall_t fn, char modname[], char filename[])
{
	int i;
	bool match = false;
	enum lazy_initcall_type type = NORMAL;

	for (i = 0; blacklist[i]; i++) {
		if (!strcmp(blacklist[i], modname))
			return false;
	}

	for (i = 0; targets_list[i]; i++) {
		if (!strcmp(targets_list[i], modname)) {
			match = true;
			break;
		}
	}

	for (i = 0; deferred_list[i]; i++) {
		if (!strcmp(deferred_list[i], modname)) {
			match = true;
			type = DEFERRED;
			break;
		}
	}

	if (!match)
		return false;

	mutex_lock(&lazy_initcall_mutex);

	pr_info("adding lazy_initcalls[%d] from %s - %s\n",
				counter, modname, filename);

	lazy_initcalls[counter].fn = fn;
	lazy_initcalls[counter].modname = modname;
	lazy_initcalls[counter].filename = filename;
	lazy_initcalls[counter].type = type;
	counter++;

	mutex_unlock(&lazy_initcall_mutex);

	return true;
}

#ifdef CONFIG_LAZY_INITCALL_DEBUG
static void __init show_unused_modules(struct work_struct *unused)
{
	int i;

	for (i = 0; i < counter; i++) {
		if (!lazy_initcalls[i].loaded) {
			pr_info("lazy_initcalls[%d]: %s not loaded yet\n", i, lazy_initcalls[i].modname);
		}
	}

	queue_delayed_work(system_freezable_power_efficient_wq,
			&show_unused_work, 5 * HZ);
}
#endif

static noinline void __init load_modname(const char * const modname)
{
	int i, ret;
	bool match = false;
	initcall_t fn;

	pr_debug("trying to load \"%s\"\n", modname);

	// Check if the driver is blacklisted (built-in, but not lazy-loaded)
	for (i = 0; blacklist[i]; i++) {
		if (!strcmp(blacklist[i], modname)) {
			pr_debug("\"%s\" is blacklisted (not lazy-loaded)\n", modname);
			return;
		}
	}

	// Find the function pointer from lazy_initcalls[]
	for (i = 0; i < counter; i++) {
		if (!strcmp(lazy_initcalls[i].modname, modname)) {
			fn = lazy_initcalls[i].fn;
			if (lazy_initcalls[i].loaded) {
				pr_debug("lazy_initcalls[%d]: %s already loaded\n", i, modname);
				return;
			}
			lazy_initcalls[i].loaded = true;
			match = true;
			break;
		}
	}

	// Unable to find the driver that the userspace requested
	if (!match) {
		// Check if this module is built-in without module_init()
		for (i = 0; builtin_list[i]; i++) {
			if (!strcmp(builtin_list[i], modname))
				return;
		}
		__fatal("failed to find a built-in module with the name \"%s\"\n", modname);
		return;
	}

	ret = fn();
	pr_info("lazy_initcalls[%d]: %s's init function returned %d\n", i, modname, ret);

	// Check if all modules are loaded so that __init memory can be released
	match = false;
	for (i = 0; i < counter; i++) {
		if (lazy_initcalls[i].type == NORMAL && !lazy_initcalls[i].loaded)
			match = true;
	}

#ifdef CONFIG_LAZY_INITCALL_DEBUG
	if (!match)
		cancel_delayed_work_sync(&show_unused_work);
	else
		queue_delayed_work(system_freezable_power_efficient_wq,
				&show_unused_work, 5 * HZ);
#endif
	if (!match)
		WRITE_ONCE(completed, true);

	return;
}

static noinline int __init __load_module(struct load_info *info, const char __user *uargs,
		       int flags)
{
	long err;

	/*
	 * Do basic sanity checks against the ELF header and
	 * sections.
	 */
	err = elf_validity_check(info);
	if (err) {
		pr_err("Module has invalid ELF structures\n");
		goto err;
	}

	/*
	 * Everything checks out, so set up the section info
	 * in the info structure.
	 */
	err = setup_load_info(info, flags);
	if (err)
		goto err;

	load_modname(info->name);

err:
	free_copy(info);
	return err;
}

static int __ref load_module(struct load_info *info, const char __user *uargs,
		       int flags)
{
	int i, ret = 0;

	mutex_lock(&lazy_initcall_mutex);

	if (completed) {
		// Userspace may ask even after all modules have been loaded
		goto out;
	}

	ret = __load_module(info, uargs, flags);
	smp_wmb();

	if (completed) {
		if (deferred_list[0] != NULL) {
			pr_info("all userspace modules loaded, now loading built-in deferred drivers\n");

			for (i = 0; deferred_list[i]; i++)
				load_modname(deferred_list[i]);
		}
		pr_info("all modules loaded, calling free_initmem()\n");
		free_initmem();
		mark_readonly();
	}

out:
	mutex_unlock(&lazy_initcall_mutex);
	return ret;
}

static int may_init_module(void)
{
	if (!capable(CAP_SYS_MODULE))
		return -EPERM;

	return 0;
}

SYSCALL_DEFINE3(init_module, void __user *, umod,
		unsigned long, len, const char __user *, uargs)
{
	int err;
	struct load_info info = { };

	err = may_init_module();
	if (err)
		return err;

	err = copy_module_from_user(umod, len, &info);
	if (err)
		return err;

	return load_module(&info, uargs, 0);
}

SYSCALL_DEFINE3(finit_module, int, fd, const char __user *, uargs, int, flags)
{
	struct load_info info = { };
	void *hdr = NULL;
	int err;

	err = may_init_module();
	if (err)
		return err;

	err = kernel_read_file_from_fd(fd, 0, &hdr, INT_MAX, NULL,
				       READING_MODULE);
	if (err < 0)
		return err;
	info.hdr = hdr;
	info.len = err;

	return load_module(&info, uargs, flags);
}
