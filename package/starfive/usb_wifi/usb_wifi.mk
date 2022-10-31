
################################################################################
#
# usb wifi firmware
#
################################################################################

USB_WIFI_LICENSE = GPL-2.0+

define USB_WIFI_BUILD_CMDS
	cp package/starfive/usb_wifi/ECR6600U_transport.bin $(@D)/
endef

define USB_WIFI_INSTALL_TARGET_CMDS
	install -m 0755 -D $(@D)/ECR6600U_transport.bin $(TARGET_DIR)/lib/firmware/
endef

$(eval $(generic-package))
