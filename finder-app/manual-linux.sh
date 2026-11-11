#!/bin/bash
# Build the course ARM64 kernel and a standalone BusyBox initramfs.
set -euo pipefail

if [ "$#" -gt 1 ]; then
    echo "Usage: $0 [outdir]" >&2
    exit 1
fi

OUTDIR=${1:-/tmp/aeld}
mkdir -p "$OUTDIR"
OUTDIR=$(realpath "$OUTDIR")
FINDER_APP_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
ARCH=arm64
CROSS_COMPILE=${CROSS_COMPILE:-aarch64-none-linux-gnu-}
KERNEL_VERSION=5.15.163
BUSYBOX_VERSION=1.33.1
JOBS=$(nproc)
ROOTFS="$OUTDIR/rootfs"

if [ "$OUTDIR" = / ]; then
    echo "Choose an output directory other than /" >&2
    exit 1
fi

for tool in make wget tar cpio gzip "${CROSS_COMPILE}gcc" "${CROSS_COMPILE}readelf"; do
    command -v "$tool" >/dev/null || { echo "Missing build tool: $tool" >&2; exit 1; }
done
if [ "$(id -u)" -eq 0 ]; then
    SUDO=()
else
    SUDO=(sudo)
fi

fetch_archive() {
    local url=$1 archive=$2
    if [ ! -s "$archive" ]; then
        wget --tries=3 --timeout=60 -O "$archive.part" "$url"
        mv "$archive.part" "$archive"
    fi
}

cd "$OUTDIR"
if [ ! -d linux-stable ]; then
    fetch_archive "https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-${KERNEL_VERSION}.tar.xz" \
        "linux-${KERNEL_VERSION}.tar.xz"
    tar -xJf "linux-${KERNEL_VERSION}.tar.xz"
    mv "linux-${KERNEL_VERSION}" linux-stable
fi

echo "Building Linux ${KERNEL_VERSION} for ${ARCH}"
make -C linux-stable ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" defconfig
make -C linux-stable -j"$JOBS" ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" Image
cp linux-stable/arch/arm64/boot/Image "$OUTDIR/Image"

# Recreate staging on every run so deleted or changed files cannot leak in.
"${SUDO[@]}" rm -rf -- "$ROOTFS"
mkdir -p "$ROOTFS"/{bin,dev,etc,home/conf,lib64,proc,sbin,sys,tmp,usr/bin,usr/sbin,var/log}
ln -s lib64 "$ROOTFS/lib"
chmod 1777 "$ROOTFS/tmp"

if [ ! -d busybox ]; then
    fetch_archive "https://busybox.net/downloads/busybox-${BUSYBOX_VERSION}.tar.bz2" \
        "busybox-${BUSYBOX_VERSION}.tar.bz2"
    tar -xjf "busybox-${BUSYBOX_VERSION}.tar.bz2"
    mv "busybox-${BUSYBOX_VERSION}" busybox
fi
make -C busybox ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" defconfig
make -C busybox -j"$JOBS" ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE"
make -C busybox ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" CONFIG_PREFIX="$ROOTFS" install

make -C "$FINDER_APP_DIR" clean
make -C "$FINDER_APP_DIR" CROSS_COMPILE="$CROSS_COMPILE"
cp "$FINDER_APP_DIR/writer" "$ROOTFS/home/"
cp "$FINDER_APP_DIR/"{finder.sh,finder-test.sh,autorun-qemu.sh} "$ROOTFS/home/"
cp "$FINDER_APP_DIR/../conf/"{username.txt,assignment.txt} "$ROOTFS/home/conf/"
chmod 755 "$ROOTFS/home/"{writer,finder.sh,finder-test.sh,autorun-qemu.sh}

# Copy each binary's loader and its transitive shared-library dependencies.
SYSROOT=$("${CROSS_COMPILE}gcc" -print-sysroot)
declare -A COPIED_LIBRARIES=()
copy_dependencies() {
    local binary=$1 interpreter library source
    interpreter=$("${CROSS_COMPILE}readelf" -l "$binary" |
        sed -n 's/.*Requesting program interpreter: \(.*\)]/\1/p')
    if [ -n "$interpreter" ]; then
        mkdir -p "$(dirname "$ROOTFS$interpreter")"
        cp -L "$SYSROOT$interpreter" "$ROOTFS$interpreter"
    fi
    while IFS= read -r library; do
        [ -n "$library" ] || continue
        [ -z "${COPIED_LIBRARIES[$library]:-}" ] || continue
        source=$("${CROSS_COMPILE}gcc" -print-file-name="$library")
        if [ ! -f "$source" ]; then
            source=$(find -L "$SYSROOT" -type f -name "$library" -print -quit)
        fi
        if [ -z "$source" ] || [ ! -f "$source" ]; then
            echo "Cannot locate shared library $library for $binary" >&2
            exit 1
        fi
        cp -L "$source" "$ROOTFS/lib/$library"
        COPIED_LIBRARIES[$library]=1
        copy_dependencies "$source"
    done < <("${CROSS_COMPILE}readelf" -d "$binary" |
        sed -n 's/.*Shared library: \[\(.*\)\]/\1/p')
}
copy_dependencies "$ROOTFS/bin/busybox"
copy_dependencies "$ROOTFS/home/writer"

"${SUDO[@]}" mknod -m 600 "$ROOTFS/dev/console" c 5 1
"${SUDO[@]}" mknod -m 666 "$ROOTFS/dev/null" c 1 3
"${SUDO[@]}" chown -R 0:0 "$ROOTFS"
(
    cd "$ROOTFS"
    find . -print0 | LC_ALL=C sort -z | "${SUDO[@]}" cpio --null -o --format=newc --owner=0:0
) | gzip -n > "$OUTDIR/initramfs.cpio.gz"

echo "Built $OUTDIR/Image and $OUTDIR/initramfs.cpio.gz"
