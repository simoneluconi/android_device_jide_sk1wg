# android_device_jide_sk1wg

LineageOS 21 (Android 14) device tree for the **Jide Remix Ultra Tablet** (REMIX SK1WG, `sk1wg`): NVIDIA Tegra 4 (T114) on the `macallan` reference board, on top of LineageOS-UL.

Project home, status, test guide and docs: https://github.com/simoneluconi/JideRemixTabletRevived

> Under development: the kernel is RAM-booted with `fastboot boot`, SELinux is permissive, and some bring-up aids are on (see "Before a release" below).

## What is here

| Folder | What it is |
|---|---|
| `interfaces/` | HIDL graphics: allocator and mapper 2.0 on NVIDIA's gralloc0 (adapters from html6405's `hardware/samsung`, with fixes for NVIDIA's buffer handles), composer 2.1 passthrough with AOSP's `HwcLoader` for NVIDIA's HWC 1.1. |
| `shims/` | What NVIDIA's Android 5.1 graphics blobs need on Android 14: `android_atomic_*`, the 5.1 `GraphicBuffer` constructor, binder-free stand-ins for `libnvcpl`/`libcplconnectclient`, and `libEGL_tegra.so`, a pass-through in front of NVIDIA's EGL loader that makes ANGLE's texture images work. Wired with `TARGET_LD_SHIM_LIBS`. |
| `audio/` | Audio HAL (HIDL 7.1) for the TLV320AIC325x codec, based on the Android emulator's tinyalsa HAL, with stock Remix OS's mixer routing. Audio policy configuration. |
| `sensors/` | `sensors.sk1wg`: presents stock's Invensense sensors HAL (device API 0.1) as a 1.3 HAL. |
| `lights/` | AIDL Lights HAL for the backlight. |
| `legacy/health/` | The AOSP health@2.0 HAL and libraries, which LineageOS-UL removed. |
| `configs/` | Bluetooth (`libbt-vendor` on `/dev/ttyHS2`), touch IDC, key layout, wpa_supplicant overlays, the power HAL's `powerhint.json` (CPU, EMC and GPU floors on touch). |
| `rootdir/` | fstab (stock GPT; `fstab.macallan.sdcard` for the microSD test variant), init scripts, ueventd rules, the Bluetooth address and app compilation scripts. |
| `overlay/` | Framework overlay (lid switch, brightness levels from stock, ...). |
| `sepolicy/` | Minimal vendor policy for the NVIDIA device nodes. SELinux is still permissive. |
| `RemovePackages/` | Drops apps a Wi-Fi tablet without a camera HAL does not need: the system partition is 1000 MB. |
| `proprietary-files.txt`, `extract-files.sh`, `setup-makefiles.sh` | The blob list and extraction. No blob is committed. |

## Building

Full instructions are in the main repo's [docs/TECHNICAL.md](https://github.com/simoneluconi/JideRemixTabletRevived/blob/main/docs/TECHNICAL.md). In short:

```
repo init -u https://github.com/LineageOS-UL/android.git -b lineage-21.0 \
    -g default,-qcom,-device,-mips,-darwin,-notdefault --depth=1
# local manifests from the main repo: manifests/sk1wg.xml, manifests/zz-lineage-snapshot.xml
repo sync -c -j8
# this tree in device/jide/sk1wg, the kernel (branch sk1wg) in kernel/jide/sk1wg
# blobs: see "Blobs" below
source build/envsetup.sh
lunch lineage_sk1wg-ap2a-userdebug
m droid
```

Outputs: `boot.img`, `system.img` (flashed to APP, with `/system/vendor` inside) and `system_ext.img` (flashed to CAC, the stock cache partition). The stock recovery partition is left alone. `SK1WG_SDCARD_ROOT=true` builds a variant that runs from a microSD card.

## Blobs

Proprietary files are never committed. `extract-files.sh <dir holding system/>` takes them from the stock Remix OS 2.0.305 `/system` (the official `system.img`, unsparsed and dumped) into `vendor/jide/sk1wg`. It also patches a few bytes of NVIDIA's `gralloc.tegra.so` and `hwcomposer.tegra.so` (explained in the script); each patch checks the original bytes first.

## Safety

The fstab mounts no eMMC partition other than APP (system), CAC (system_ext), UDA (data) and MSC (misc). Never add a partition that the bootloader reads, and never write USP (`/staging`): NVIDIA's bootloader applies a bootloader update it finds there. The anti-brick rules are in the main repo's `docs/INSTALL.md`.

## Before a release

- `WITH_ADB_INSECURE` (`lineage_sk1wg.mk`) and adb in the "mtp" configuration (`rootdir/etc/init.macallan.usb.rc`) on debuggable builds.
- `oops=panic` in `BOARD_KERNEL_CMDLINE`.
- SELinux permissive, and `SELINUX_IGNORE_NEVERALLOWS`.

## Licence

Apache-2.0 (`LICENSE`). Files taken from AOSP and html6405's trees keep their own Apache-2.0 headers (`audio/NOTICE`).

## Credits

TerenceWSK (the CyanogenMod 13 device tree and kernel for the SK1WG), html6405, LineageOS-UL, and the LineageOS, AOSP and CyanogenMod contributors.
