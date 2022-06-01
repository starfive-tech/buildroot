################################################################################
#
# codaj12
#
################################################################################

# CODAJ12_VERSION:=1.0.0
CODAJ12_SITE=$(TOPDIR)/../soft_3rdpart/codaj12
CODAJ12_SITE_METHOD=local
CODAJ12_INSTALL_STAGING = YES

export KERNELDIR=$(TOPDIR)/../work/linux

define CODAJ12_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) -f $(@D)/codaj12_buildroot.mak
#	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D) -f $(@D)/codaj12Driver_buildroot.mak
endef

define CODAJ12_CLEAN_CMDS

endef

define CODAJ12_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0777 $(@D)/jdi/linux/driver/load.sh $(TARGET_DIR)/root/codaj12/load.sh
	$(INSTALL) -D -m 0777 $(@D)/jdi/linux/driver/unload.sh $(TARGET_DIR)/root/codaj12/unload.sh
	$(INSTALL) -D -m 0644 $(@D)/libcodadec.so $(TARGET_DIR)/usr/lib/libcodadec.so
#	$(INSTALL) -D -m 0644 $(@D)/jdi/linux/driver/jpu.ko $(TARGET_DIR)/root/codaj12/jpu.ko
endef


define CODAJ12_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/include/codaj12
	$(INSTALL) -D -m 0644 $(@D)/jpuapi/jpuapi.h                  $(STAGING_DIR)/usr/include/codaj12/jpuapi/jpuapi.h
	$(INSTALL) -D -m 0644 $(@D)/jpuapi/jpuapifunc.h              $(STAGING_DIR)/usr/include/codaj12/jpuapi/jpuapifunc.h
	$(INSTALL) -D -m 0644 $(@D)/jpuapi/regdefine.h               $(STAGING_DIR)/usr/include/codaj12/jpuapi/regdefine.h
	$(INSTALL) -D -m 0644 $(@D)/jpuapi/jpuconfig.h               $(STAGING_DIR)/usr/include/codaj12/jpuapi/jpuconfig.h
	$(INSTALL) -D -m 0644 $(@D)/jpuapi/jputypes.h                $(STAGING_DIR)/usr/include/codaj12/jpuapi/jputypes.h
	$(INSTALL) -D -m 0644 $(@D)/jpuapi/jputable.h                $(STAGING_DIR)/usr/include/codaj12/jpuapi/jputable.h
	$(INSTALL) -D -m 0644 $(@D)/sample/helper/cnm_fpga.h         $(STAGING_DIR)/usr/include/codaj12/sample/helper/cnm_fpga.h
	$(INSTALL) -D -m 0644 $(@D)/sample/helper/platform.h         $(STAGING_DIR)/usr/include/codaj12/sample/helper/platform.h
	$(INSTALL) -D -m 0644 $(@D)/sample/helper/yuv_feeder.h       $(STAGING_DIR)/usr/include/codaj12/sample/helper/yuv_feeder.h
	$(INSTALL) -D -m 0644 $(@D)/sample/helper/datastructure.h    $(STAGING_DIR)/usr/include/codaj12/sample/helper/datastructure.h
	$(INSTALL) -D -m 0644 $(@D)/sample/helper/jpulog.h           $(STAGING_DIR)/usr/include/codaj12/sample/helper/jpulog.h
	$(INSTALL) -D -m 0644 $(@D)/sample/main_helper.h             $(STAGING_DIR)/usr/include/codaj12/sample/main_helper.h
	$(INSTALL) -D -m 0644 $(@D)/jdi/linux/driver/jpu.h           $(STAGING_DIR)/usr/include/codaj12/jdi/linux/driver/jpu.h
	$(INSTALL) -D -m 0644 $(@D)/jdi/linux/driver/jmm.h           $(STAGING_DIR)/usr/include/codaj12/jdi/linux/driver/jmm.h
	$(INSTALL) -D -m 0644 $(@D)/jdi/jdi.h                        $(STAGING_DIR)/usr/include/codaj12/jdi/jdi.h
	$(INSTALL) -D -m 0644 $(@D)/jdi/mm.h                         $(STAGING_DIR)/usr/include/codaj12/jdi/mm.h
	$(INSTALL) -D -m 0644 $(@D)/config.h                         $(STAGING_DIR)/usr/include/codaj12/config.h

endef

define CODAJ12_UNINSTALL_TARGET_CMDS
	rm -rf $(TARGET_DIR)/root/codaj12/jpu.ko
	rm -rf $(TARGET_DIR)/root/codaj12/load.sh
	rm -rf $(TARGET_DIR)/root/codaj12/unload.sh
endef

codaj12_WORK_DIR := $(TARGET_DIR)/../build/codaj12
codaj12driver:
ifneq ($(wildcard $(codaj12_WORK_DIR)/codaj12Driver_buildroot.mak),)
	$(TARGET_MAKE_ENV) $(MAKE) -C $(codaj12_WORK_DIR) -f $(codaj12_WORK_DIR)/codaj12Driver_buildroot.mak
	$(INSTALL) -D -m 0644 $(codaj12_WORK_DIR)/jdi/linux/driver/jpu.ko $(TARGET_DIR)/root/codaj12/jpu.ko
endif

$(eval $(generic-package))
