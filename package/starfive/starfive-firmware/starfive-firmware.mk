################################################################################
#
# starfive-firmware
#
################################################################################
STARFIVE_FIRMWARE_LICENSE = Proprietary

# AP6256 Bluetooth
ifeq ($(BR2_PACKAGE_STARFIVE_FIRMWARE_AP6256_BLUETOOTH),y)
STARFIVE_FIRMWARE_FILES += $(TOPDIR)/package/starfive/starfive-firmware/ap6256-bluetooth/BCM4345C5.hcd
endif

# RTL8852BU Bluetooth
ifeq ($(BR2_PACKAGE_STARFIVE_FIRMWARE_RTL8852BU_BLUETOOTH),y)
STARFIVE_FIRMWARE_FILES += $(TOPDIR)/package/starfive/starfive-firmware/rtl8852bu-bluetooth/rtl8852bu_config
STARFIVE_FIRMWARE_FILES += $(TOPDIR)/package/starfive/starfive-firmware/rtl8852bu-bluetooth/rtl8852bu_fw
endif


# AP6256 SDIO Wifi
ifeq ($(BR2_PACKAGE_STARFIVE_FIRMWARE_AP6256_SDIO_WIFI),y)
STARFIVE_FIRMWARE_FILES += $(TOPDIR)/package/starfive/starfive-firmware/ap6256-sdio-wifi/fw_bcm43456c5_ag.bin
STARFIVE_FIRMWARE_FILES += $(TOPDIR)/package/starfive/starfive-firmware/ap6256-sdio-wifi/nvram_ap6256.txt
endif

define STARFIVE_FIRMWARE_INSTALL_TARGET_CMDS
	$(foreach f,$(STARFIVE_FIRMWARE_FILES), \
		$(INSTALL) -m 0755 -D $(f) $(TARGET_DIR)/lib/firmware/; \
	)
	$(INSTALL) -D -m 755 package/starfive/starfive-firmware/ap6256-bluetooth/S90bluetooth $(TARGET_DIR)/etc/init.d/S90bluetooth

endef

$(eval $(generic-package))
