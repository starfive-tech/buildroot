################################################################################
#
# libcamera-apps
#
################################################################################

LIBCAMERA_APPS_SITE = https://github.com/starfive-tech/libcamera-apps.git
LIBCAMERA_APPS_VERSION = 31377be38defcfbd6bc70bd12895c6f145fc8171
LIBCAMERA_APPS_SITE_METHOD = git
LIBCAMERA_APPS_INSTALL_STAGING = YES

LIBCAMERA_APPS_DEPENDENCIES = libcamera libexif tiff boost host-pkgconf jpeg libpng sf-omx-il

$(eval $(cmake-package))
