################################################################################
#
# img-gpu-powervr
#
# This file is for Imagination PowerVR GPU, modified from freescale-imx/imx-gpu-viv
#
# Windsome Zeng <windsome.zeng@starfivetech.com>
#
################################################################################

# ifeq ($(BR2_aarch64),y)
# IMG_GPU_POWERVR_VERSION = 6.4.3.p2.0-aarch64
# else
# IMG_GPU_POWERVR_VERSION = 6.4.3.p2.0-aarch32
# endif
# IMG_GPU_POWERVR_SITE = $(FREESCALE_IMX_SITE)
# IMG_GPU_POWERVR_SOURCE = imx-gpu-viv-$(IMG_GPU_POWERVR_VERSION).bin

IMG_GPU_POWERVR_INSTALL_STAGING = YES

IMG_GPU_POWERVR_LICENSE = Strictly Confidential
IMG_GPU_POWERVR_REDISTRIBUTE = NO

IMG_GPU_POWERVR_PROVIDES = libgles libopencl
IMG_GPU_POWERVR_LIB_TARGET = $(call qstrip,$(BR2_PACKAGE_IMG_GPU_POWERVR_OUTPUT))

ifeq ($(IMG_GPU_POWERVR_LIB_TARGET),x11)
# The libGAL.so library provided by imx-gpu-viv uses X functions. Packages
# may want to link against libGAL.so (QT5 Base with OpenGL and X support
# does so). For this to work we need build dependencies to libXdamage,
# libXext and libXfixes so that X functions used in libGAL.so are referenced.
IMG_GPU_POWERVR_DEPENDENCIES += xlib_libXdamage xlib_libXext xlib_libXfixes
endif

# Libraries are linked against libdrm, except framebuffer output on ARM
ifneq ($(IMG_GPU_POWERVR_LIB_TARGET)$(BR2_riscv),fby)
IMG_GPU_POWERVR_DEPENDENCIES += libdrm
endif

ifeq ($(IMG_GPU_POWERVR_LIB_TARGET),wayland)
IMG_GPU_POWERVR_DEPENDENCIES += wayland
endif

# define IMG_GPU_POWERVR_EXTRACT_CMDS
# 	$(call FREESCALE_IMX_EXTRACT_HELPER,$(IMG_GPU_POWERVR_DL_DIR)/$(IMG_GPU_POWERVR_SOURCE))
# endef

# ifeq ($(IMG_GPU_POWERVR_LIB_TARGET),fb)
# define IMG_GPU_POWERVR_FIXUP_PKGCONFIG
# 	ln -sf egl_linuxfb.pc $(@D)/gpu-core/usr/lib/pkgconfig/egl.pc
# endef
# else ifeq ($(IMG_GPU_POWERVR_LIB_TARGET),wayland)
# define IMG_GPU_POWERVR_FIXUP_PKGCONFIG
# 	ln -sf egl_wayland.pc $(@D)/gpu-core/usr/lib/pkgconfig/egl.pc
# endef
# else ifeq ($(IMG_GPU_POWERVR_LIB_TARGET),x11)
# define IMG_GPU_POWERVR_FIXUP_PKGCONFIG
# 	$(foreach lib,egl gbm glesv1_cm glesv2 vg, \
# 		ln -sf $(lib)_x11.pc $(@D)/gpu-core/usr/lib/pkgconfig/$(lib).pc
# 	)
# endef
# endif

# Instead of building, we fix up the inconsistencies that exist
# in the upstream archive here. We also remove unused backend files.
# Make sure these commands are idempotent.
# define IMG_GPU_POWERVR_BUILD_CMDS
# 	cp -dpfr $(@D)/gpu-core/usr/lib/$(IMG_GPU_POWERVR_LIB_TARGET)/* $(@D)/gpu-core/usr/lib/
# 	$(foreach backend,fb x11 wayland, \
# 		$(RM) -r $(@D)/gpu-core/usr/lib/$(backend)
# 	)
# 	$(IMG_GPU_POWERVR_FIXUP_PKGCONFIG)
# endef
# 
# define IMG_GPU_POWERVR_INSTALL_STAGING_CMDS
# 	cp -r $(@D)/gpu-core/usr/* $(STAGING_DIR)/usr
# endef

# ifeq ($(BR2_PACKAGE_IMG_GPU_POWERVR_EXAMPLES),y)
# define IMG_GPU_POWERVR_INSTALL_EXAMPLES
# 	mkdir -p $(TARGET_DIR)/usr/share/examples/
# 	cp -r $(@D)/gpu-demos/opt/* $(TARGET_DIR)/usr/share/examples/
# endef
# endif
# 
# ifeq ($(BR2_PACKAGE_IMG_GPU_POWERVR_GMEM_INFO),y)
# define IMG_GPU_POWERVR_INSTALL_GMEM_INFO
# 	cp -dpfr $(@D)/gpu-tools/gmem-info/usr/bin/* $(TARGET_DIR)/usr/bin/
# endef
# endif

# define IMG_GPU_POWERVR_INSTALL_TARGET_CMDS
# 	$(IMG_GPU_POWERVR_INSTALL_EXAMPLES)
# 	$(IMG_GPU_POWERVR_INSTALL_GMEM_INFO)
# 	cp -a $(@D)/gpu-core/usr/lib $(TARGET_DIR)/usr
# 	$(INSTALL) -D -m 0644 $(@D)/gpu-core/etc/Vivante.icd $(TARGET_DIR)/etc/OpenCL/vendors/Vivante.icd
# endef

$(eval $(generic-package))
