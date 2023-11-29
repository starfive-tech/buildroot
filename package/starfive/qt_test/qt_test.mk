################################################################################
#
# qttest
#
################################################################################
QT_TEST_VERSION:=1.0.0
QT_TEST_SITE=$(TOPDIR)/package/starfive/qt_test/src
QT_TEST_SITE_METHOD=local
QT_TEST_DEPENDENCIES = qt5base

define QT_TEST_INSTALL_TARGET_CMDS
	cd $(@D); $(TARGET_MAKE_ENV) $(MAKE) INSTALL_ROOT=$(TARGET_DIR) install
endef

$(eval $(qmake-package))
