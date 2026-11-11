# Assignment 3 Part 2: Manual Linux Build

## Build and boot on a Linux host

Use the course `cuaesd/aesd-autotest:24-assignment3` image, or install the
dependencies listed in the course Dockerfile and add the Arm GNU
`aarch64-none-linux-gnu-` toolchain to PATH.

```sh
./finder-app/manual-linux.sh                 # output: /tmp/aeld
./finder-app/manual-linux.sh /tmp/aeld-build # optional output directory
./finder-app/start-qemu-terminal.sh /tmp/aeld-build
./finder-app/start-qemu-app.sh /tmp/aeld-build
./full-test.sh
```

The script builds Linux 5.15.163 and BusyBox 1.33.1 from pinned upstream
release archives, cross-compiles `writer`, copies the ELF loaders and
transitive shared libraries, and creates console/null device nodes. It
packages `rootfs` as a root-owned newc archive compressed into
`initramfs.cpio.gz`, next to the ARM64 `Image`.

Run the same build command again to rebuild in an existing output directory.
Source archives and compiled objects are reused; the rootfs staging directory
is recreated so that it reflects the current scripts, configuration, and writer.
Kernel modules are deliberately not installed, as required by this assignment.

The target's `/home` contains `writer`, `finder.sh`, `finder-test.sh`,
`autorun-qemu.sh`, and the `conf` directory. No target login is required.
The app boot script runs the finder test and prints `Completed with success!!`
on success, then starts a shell.

## Automated validation

The `full-test` GitHub Actions job uses the official Assignment 3 Linux image.
It runs `./full-test.sh`, rebuilds using the existing output directory, and
boots and validates the rebuilt image with `SKIP_BUILD=1 DO_VALIDATE=1`.
It uploads the serial console log and test log as `assignment3-qemu-evidence`.
The existing unit-test and ARM cross-compile jobs are retained.

## Assistance disclosure

The build implementation and CI updates were prepared with OpenAI Codex AI
assistance using the official course starter source and tests. No other
student's solution was used as a reference.
