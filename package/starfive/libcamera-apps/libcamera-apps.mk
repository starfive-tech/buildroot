################################################################################
#
# libcamera-apps
#
################################################################################

LIBCAMERA_APPS_SITE = https://github.com/starfive-tech/libcamera-apps.git
LIBCAMERA_APPS_VERSION = ae96f800c39c1a8a27eb390e97ea08062eb925eb
LIBCAMERA_APPS_SITE_METHOD = git
LIBCAMERA_APPS_INSTALL_STAGING = YES

LIBCAMERA_APPS_DEPENDENCIES = libcamera libexif tiff boost host-pkgconf jpeg libpng sf-omx-il

$(eval $(cmake-package))
