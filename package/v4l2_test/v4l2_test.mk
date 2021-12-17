################################################################################
#
# v4l2test
#
################################################################################

V4L2_TEST_LICENSE = GPL-2.0+

define V4L2_TEST_BUILD_CMDS
	cp package/v4l2_test/v4l2_test.c $(@D)/
	cp package/v4l2_test/yuv.c $(@D)/
	cp package/v4l2_test/yuv.h $(@D)/
	cp package/v4l2_test/convert.c $(@D)/
	cp package/v4l2_test/convert.h $(@D)/
	cp package/v4l2_test/config.h $(@D)/
	cp package/v4l2_test/string.c $(@D)/
	cp package/v4l2_test/pipeline_setting.sh $(@D)/
        (cd $(@D); $(TARGET_CC) -Wall -O2 v4l2_test.c yuv.c convert.c string.c -l v4l2 -l jpeg -o v4l2test)
        #(cd $(@D); $(TARGET_CC) -Wall -O2 v4l2_test.c -o v4l2test)
endef

define V4L2_TEST_INSTALL_TARGET_CMDS
        install -m 0755 -D $(@D)/v4l2test $(TARGET_DIR)/usr/bin/v4l2test
        install -m 0755 -D $(@D)/pipeline_setting.sh $(TARGET_DIR)/usr/bin/pipeline_setting.sh
endef

V4L2_TEST_DEPENDENCIES = jpeg libv4l
$(eval $(generic-package))

