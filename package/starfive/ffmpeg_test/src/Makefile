CROSS_CC_PREFIX = riscv64-buildroot-linux-gnu-
CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
LINKER = $(CC)
AR  = $(CROSS_CC_PREFIX)ar

CFLAGS = -Wall -g -O2 -D_FILE_OFFSET_BITS=64 
CFLAGS += -I$(STAGING_DIR)/usr/include/ -I$(STAGING_DIR)/usr/include/drm/
LDLIBS += -lavdevice -lavformat -lavfilter -lavcodec -lswresample -lswscale \
		  -lavutil -ldrm -ljpeg

COMMON_SRC = stf_log.c common.c stf_drm.c

TARGET1 = ffmpeg_input_test
TARGET1_SRC := $(addsuffix .c,$(TARGET1)) $(COMMON_SRC)
TARGET1_OBJ := $(subst .c,.o,$(TARGET1_SRC))

TARGET2 = ffmpeg_dec_test
TARGET2_SRC := $(addsuffix .c,$(TARGET2))
TARGET2_OBJ := $(subst .c,.o,$(TARGET2_SRC))

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(TARGET1_OBJ)
$(TARGET2): $(TARGET2_OBJ)

clean:
	rm -rf *.o $(TARGET1) $(TARGET2)

# %.o: %.c
# 	$(CC) -fPIC -shared $(FLAGS) -Wall -Werror -c $< -o $@ -MD -MF $(@:.o=.dep)
