#!/bin/bash
export KERNELDIR=`readlink -f .`
export RAMFS_SOURCE=`readlink -f $KERNELDIR/ramdisk`
export PARTITION_SIZE=134217728

export OS="12.0.0"
export SPL="2022-05"

echo "kerneldir = $KERNELDIR"
echo "ramfs_source = $RAMFS_SOURCE"

RAMFS_TMP="/tmp/arter97-waipio-hdk-ramdisk"

echo "ramfs_tmp = $RAMFS_TMP"
cd $KERNELDIR

if [[ "${1}" == "skip" ]] ; then
	echo "Skipping Compilation"
else
	echo "Compiling kernel"
	cp defconfig .config
	make "$@" || exit 1
fi

(
  # Build dtbo.img
  fdtoverlaymerge \
    -i arch/arm64/boot/dts/vendor/qcom/waipiop-hdk-pm8010-overlay.dtbo \
    arch/arm64/boot/dts/vendor/qcom/audio/waipio-audio-hdk.dtbo \
    arch/arm64/boot/dts/vendor/qcom/camera/waipio-camera-sensor-mtp.dtbo \
    arch/arm64/boot/dts/vendor/qcom/display/waipio-sde-display-hdk-overlay.dtbo \
    -o .dtbo
  mkdtimg create dtbo.img --page_size=4096 .dtbo
) &

(
  # Build vendor_boot.img (with dtb)
  fdtoverlay \
    -i arch/arm64/boot/dts/vendor/qcom/waipiop.dtb \
    -o .dtb \
    arch/arm64/boot/dts/vendor/qcom/audio/waipio-audio.dtbo \
    arch/arm64/boot/dts/vendor/qcom/camera/waipio-camera.dtbo \
    arch/arm64/boot/dts/vendor/qcom/display/waipio-sde.dtbo \
    arch/arm64/boot/dts/vendor/qcom/mmrm/waipio-mmrm.dtbo \
    arch/arm64/boot/dts/vendor/qcom/mmrm/waipio-mmrm-test.dtbo \
    arch/arm64/boot/dts/vendor/qcom/video/waipio-vidc.dtbo
  mkbootimg.py \
    --header_version 4 \
    --pagesize 0x00001000 \
    --base 0x00000000 \
    --kernel_offset 0x00008000 \
    --ramdisk_offset 0x01000000 \
    --tags_offset 0x00000100 \
    --dtb_offset 0x0000000001f00000 \
    --vendor_cmdline 'video=vfb:640x400,bpp=32,memsize=3072000 bootconfig buildvariant=userdebug' \
    --dtb .dtb \
    --vendor_bootconfig vendor_boot/bootconfig \
    --ramdisk_type 1 --ramdisk_name '' --vendor_ramdisk_fragment vendor_boot/vendor_ramdisk00 \
    --vendor_boot vendor_boot.img
) &

echo "Building new ramdisk"
#remove previous ramfs files
rm -rf '$RAMFS_TMP'*
rm -rf $RAMFS_TMP
rm -rf $RAMFS_TMP.cpio
#copy ramfs files to tmp directory
cp -axpP $RAMFS_SOURCE $RAMFS_TMP
cd $RAMFS_TMP

#clear git repositories in ramfs
find . -name .git -exec rm -rf {} \;
find . -name EMPTY_DIRECTORY -exec rm -rf {} \;

$KERNELDIR/ramdisk_fix_permissions.sh 2>/dev/null

cd $KERNELDIR
rm -rf $RAMFS_TMP/tmp/*

cd $RAMFS_TMP
find . | fakeroot cpio -H newc -o | lz4 -l > $RAMFS_TMP.cpio.lz4
ls -lh $RAMFS_TMP.cpio.lz4
cd $KERNELDIR

echo "Making new boot image"
mkbootimg.py \
    --kernel $KERNELDIR/arch/arm64/boot/Image \
    --ramdisk $RAMFS_TMP.cpio.lz4 \
    --pagesize 4096 \
    --os_version     $OS \
    --os_patch_level $SPL \
    --header_version 4 \
    -o $KERNELDIR/boot.img

GENERATED_SIZE=$(stat -c %s boot.img)
if [[ $GENERATED_SIZE -gt $PARTITION_SIZE ]]; then
	echo "boot.img size larger than partition size!" 1>&2
	exit 1
fi

wait
echo "done"
ls -al boot.img dtbo.img vendor_boot.img
echo ""
