#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

CURRENT_DIR=$(pwd)

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
    # wget https://github.com/bwalle/ptxdist-vetero/blob/f1332461242e3245a47b4685bc02153160c0a1dd/patches/linux-5.0/dtc-multiple-definition.patch
    # git apply dtc-multiple-definition.patch
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}


    make -j 8 ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make -j 8 ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j 8 ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE} dtbs
    # make -j 8 ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE} modules
    make -j 8 ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE} all
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories

cd $OUTDIR
mkdir rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    git switch -c ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi

# busy box 
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
cd ${OUTDIR}/rootfs
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
export SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -a $SYSROOT/lib/ld-linux-aarch64.so.1 lib
cp -a $SYSROOT/lib64/ld-2.31.so lib64
cp -a $SYSROOT/lib64/libm.so.6 lib64
cp -a $SYSROOT/lib64/libm-2.31.so lib64
cp -a $SYSROOT/lib64/libresolv.so.2 lib64
cp -a $SYSROOT/lib64/libresolv-2.31.so lib64
cp -a $SYSROOT/lib64/libc.so.6 lib64
cp -a $SYSROOT/lib64/libc-2.31.so lib64


cd $CURRENT_DIR
make clean
make arm

# copy test files
cp finder.sh $OUTDIR/rootfs/home
cp finder-test.sh $OUTDIR/rootfs/home
cp writer $OUTDIR/rootfs/home
cp writer.sh $OUTDIR/rootfs/home
cp autorun-qemu.sh $OUTDIR/rootfs/home
cp -r conf/ ${OUTDIR}/rootfs/home
cp -r conf/ ${OUTDIR}/rootfs/

cd ${OUTDIR}/rootfs
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1


# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..
gzip initramfs.cpio