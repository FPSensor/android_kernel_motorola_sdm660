# Motorola G7 Plus (lake) on 4.19: porting notes

Work in progress. The files in this directory are lake's 4.4 device tree,
imported unmodified from LineageOS/android_kernel_motorola_msm8998
(lineage-22.2, arch/arm/boot/dts/qcom). They don't build yet.

## Boot layout

lake boots like on stock Pie: a base DTB appended to `Image.gz-dtb`
(`sdm636-lake-base.dtb`) plus the board overlay picked by the bootloader
from the dtbo partition (`BOARD_KERNEL_SEPARATED_DTBO`). The overlays are
selected by `qcom,msm-id = <345 0x0>` and `qcom,board-id`:

| Overlay | board-id | Touch |
|---|---|---|
| `sdm636-lake-evt1-overlay` | `<0x4C 0xA100>` | Synaptics TCM |
| `sdm636-lake-evt2-overlay` | `<0x4C 0xA200>` | Synaptics TCM |
| `sdm636-lake-na-evt2-overlay` | `<(LAKE_RADIO_VZW << 8 \| 0x4C) 0xA200>` | Synaptics TCM, NA radio, parallel charging |

The wiring is in place: `CONFIG_MOTOROLA_LAKE_DTB` (drivers/soc/qcom/Kconfig),
`motorola/Makefile` and `vendor/ext_config/lake.config`.

## Device tree port (4.4 -> 4.19)

Follow what was done for payton (`../payton/`, ported from
moto-common/android_kernel_motorola_msm-4.19 by electimon):

- Include the 4.19 bases with relative paths: `../../sdm636.dtsi`,
  `../moto-common.dtsi`, `../moto-pinctrl.dtsi`, `../moto-mem-reserve.h`.
  `sdm636-moto-common.dtsi` from 4.4 is replaced by `../moto-common.dtsi`;
  diff the two and move what is lake specific into `sdm636-lake.dtsi`.
- Panels: split each `dsi-panel-mot-*-624-1080p-vid.dtsi` into the panel
  description and the `-common.dtsi` with the timings, as payton's
  `dsi-panel-mot-tianma-520-1080p-vid{,-common}.dtsi`.
- Fix the labels and nodes renamed between the 4.4 and 4.19 sdm660 trees
  (i2c/spi/qupv3, pinctrl, regulators, pm660/pm660l, mdss). The 4.19
  `sdm636.dtsi` and `sdm660*.dtsi` in `../../` are the reference.
- Audio: `sdm636-lake-audio.dtsi` uses the same Madera machine
  (`qcom,sdm660-asoc-snd-madera`) as payton's `audio.dtsi`; compare them.

## Drivers lake needs that payton doesn't have

| Hardware | 4.4 config | Status |
|---|---|---|
| Touch: Synaptics TCM, Novatek, Himax | `TOUCHSCREEN_SYNAPTICS_TCM_MMI`, `TOUCHSCREEN_NOVATEK_CHIPSET`, `TOUCHSCREEN_HIMAX_*` | to port (per panel variant) |
| Fingerprint: Egistec | `INPUT_EGISTEC_FPS_MMI` | to port |
| Haptics: Awinic | `INPUT_AWINIC_HAPTIC` | to port |
| Speaker amp: Cirrus CS35L41 | `SND_SOC_CS35L41_I2C`, `SND_SOC_CS35L41_STEREO` | to port (codec CS47L35 is shared with payton) |
| ToF laser (camera AF): ST VL53L0 | `INPUT_STMVL53L0` | to port |
| Parallel charging | `MMI_PARALLEL_CHARGING` | to check |

Already shared with payton: Madera CS47L35 codec stack, camera_v2-legacy,
NQ NFC, utags, mmi_info, mmi_sys_temp, the sensors class.
