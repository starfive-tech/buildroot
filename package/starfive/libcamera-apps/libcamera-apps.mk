################################################################################
#
# libcamera-apps
#
################################################################################

LIBCAMERA_APPS_SITE = https://github.com/raspberrypi/libcamera-apps.git
LIBCAMERA_APPS_VERSION = 54a781dffdd101954dcfa6acd0bd80006f67da83
LIBCAMERA_APPS_SITE_METHOD = git
LIBCAMERA_APPS_INSTALL_STAGING = YES

LIBCAMERA_APPS_DEPENDENCIES = libcamera libexif tiff boost host-pkgconf jpeg libpng sf-omx-il

$(eval $(cmake-package))
