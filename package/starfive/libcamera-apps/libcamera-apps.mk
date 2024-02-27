################################################################################
#
# libcamera-apps
#
################################################################################

LIBCAMERA_APPS_SITE = https://github.com/starfive-tech/libcamera-apps.git
LIBCAMERA_APPS_VERSION = d88f3b7c9c199d87180b551016bf894bd5196f09
LIBCAMERA_APPS_SITE_METHOD = git
LIBCAMERA_APPS_INSTALL_STAGING = YES

LIBCAMERA_APPS_DEPENDENCIES = libcamera libexif tiff boost host-pkgconf jpeg libpng sf-omx-il

$(eval $(cmake-package))
