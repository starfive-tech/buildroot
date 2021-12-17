################################################################################
#
# v4l-utils
#
################################################################################

V4LUTILS_VERSION = 1.20.0
V4LUTILS_SOURCE = v4l-utils-$(V4LUTILS_VERSION).tar.bz2
V4LUTILS_SITE = https://linuxtv.org/downloads/v4l-utils
V4LUTILS_LICENSE_FILES = COPYING
V4LUTILS_INSTALL_STAGING = YES
V4LUTILS_INSTALL_TARGET = NO
V4LUTILS_CONF_OPTS = --disable-shared
V4LUTILS_AUTORECONF = YES

define V4LUTILS_INSTALL_TARGET_CMDS
        install -m 0755 -D $(@D)/utils/media-ctl/media-ctl $(TARGET_DIR)/usr/bin/media-ctl
        install -m 0755 -D $(@D)/utils/v4l2-compliance/v4l2-compliance $(TARGET_DIR)/usr/bin/v4l2-compliance
        install -m 0755 -D $(@D)/utils/v4l2-ctl/v4l2-ctl $(TARGET_DIR)/usr/bin/v4l2-ctl
        install -m 0755 -D $(@D)/utils/v4l2-sysfs-path/v4l2-sysfs-path $(TARGET_DIR)/usr/bin/v4l2-sysfs-path
endef

V4LUTILS_DEPENDENCIES = libv4l
$(eval $(autotools-package))
