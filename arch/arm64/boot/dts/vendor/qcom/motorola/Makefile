# SPDX-License-Identifier: GPL-2.0-only
#
# Motorola sdm630/sdm636/sdm660 devices. Included from the parent Makefile
# when CONFIG_MOTOROLA_DTB is set, so paths are relative to vendor/qcom.
# Add one line per device; device specific files live in motorola/<device>/
# and shared ones (moto-common.dtsi, ...) in motorola/.

dtb-$(CONFIG_MOTOROLA_PAYTON_DTB) += \
	motorola/payton/sdm630-payton.dtb \
	motorola/payton/sdm630-payton-p1a.dtb \
	motorola/payton/sdm630-payton-p2a.dtb

# lake boots a base DTB appended to the kernel plus its board overlay from
# the dtbo partition (BOARD_KERNEL_SEPARATED_DTBO).
dtb-$(CONFIG_MOTOROLA_LAKE_DTB) += motorola/lake/sdm636-lake-base.dtb
dtbo-$(CONFIG_MOTOROLA_LAKE_DTB) += \
	motorola/lake/sdm636-lake-evt1-overlay.dtbo \
	motorola/lake/sdm636-lake-evt2-overlay.dtbo \
	motorola/lake/sdm636-lake-na-evt2-overlay.dtbo
motorola/lake/sdm636-lake-evt1-overlay.dtbo-base := motorola/lake/sdm636-lake-base.dtb
motorola/lake/sdm636-lake-evt2-overlay.dtbo-base := motorola/lake/sdm636-lake-base.dtb
motorola/lake/sdm636-lake-na-evt2-overlay.dtbo-base := motorola/lake/sdm636-lake-base.dtb
