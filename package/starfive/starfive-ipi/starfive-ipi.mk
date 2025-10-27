################################################################################
#
# starfive-ipi
#
################################################################################
STARFIVE_IPI_LICENSE = Proprietary

# Starfive IPI and RPMSG modules
ifeq ($(BR2_PACKAGE_STARFIVE_IPI),y)
define STARFIVE_IPI_INSTALL_MODULES
	@echo "install starfive ipi and rpmsg modules ..."
	$(INSTALL) -m 0755 -D $(STARFIVE_IPI_PKGDIR)/starfive_ipi_mailbox.ko $(TARGET_DIR)/lib/modules/
	$(INSTALL) -m 0755 -D $(STARFIVE_IPI_PKGDIR)/starfive_rpmsg.ko $(TARGET_DIR)/lib/modules/
	$(INSTALL) -m 0755 -D $(STARFIVE_IPI_PKGDIR)/S98starfive-ipi $(TARGET_DIR)/etc/init.d/
endef
endif

# install modules
define STARFIVE_IPI_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/modules
	$(STARFIVE_IPI_INSTALL_MODULES)
endef

$(eval $(generic-package))
