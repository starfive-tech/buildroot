KERNEL_MAKEFILE_PATH := ../linux/Makefile

K_VERSION := 5
K_PATCHLEVEL := 15
K_SUBLEVEL := 0

ifneq ($(wildcard $(KERNEL_MAKEFILE_PATH)),)
K_VERSION = $(shell grep '^VERSION' ../linux/Makefile | awk '{print $$3}')
K_PATCHLEVEL = $(shell grep '^PATCHLEVEL' ../linux/Makefile | awk '{print $$3}')
K_SUBLEVEL = $(shell grep '^SUBLEVEL' ../linux/Makefile | awk '{print $$3}')
endif


#if kernel version > 6.6.20, use new version of libv4l
ifeq ($(shell if ([ $(K_VERSION) -gt 6 ]) || \
	([ $(K_VERSION) -eq 6 ] && [ $(K_PATCHLEVEL) -gt 6 ]) || \
	([ $(K_VERSION) -eq 6 ] && [ $(K_PATCHLEVEL) -eq 6 ] && [ $(K_SUBLEVEL) -ge 20 ]); \
	then echo true; else echo false; fi) , true)
include package/libv4l/libv4l-1.28.1.mak
else
include package/libv4l/libv4l-1.24.1.mak
endif

