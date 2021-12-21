################################################################################
#
# ffmpegtest
#
################################################################################
FFMPEG_TEST_VERSION:=1.0.0
FFMPEG_TEST_SITE=$(TOPDIR)/package/starfive/ffmpeg_test/src
FFMPEG_TEST_SITE_METHOD=local

define FFMPEG_TEST_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) -f $(@D)/Makefile
endef

define FFMPEG_TEST_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/ffmpeg_dec_test $(TARGET_DIR)/usr/bin/
endef

FFMPEG_TEST_DEPENDENCIES = libdrm ffmpeg
$(eval $(generic-package))
