################################################################################
#
# pptest
#
################################################################################

define PP_TEST_BUILD_CMDS
	cp package/starfive/pp_test/pp_test.c $(@D)/
	(cd $(@D); $(TARGET_CC) -Wall -O2 pp_test.c -o pp_test)
endef

define PP_TEST_INSTALL_TARGET_CMDS
	install -m 0755 -D $(@D)/pp_test $(TARGET_DIR)/usr/bin/pp_test
endef

$(eval $(generic-package))

