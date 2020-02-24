include ../Makefile.env

TARGET = vkfbo

SRCS = 
SRCS += main.c
SRCS += teapot.c
SRCS += texcube.c
SRCS += ../common/vk_init.c
SRCS += ../common/vk_debug.c
SRCS += ../common/vk_render.c
SRCS += ../common/vk_render_target.c
SRCS += ../common/util_matrix.c
SRCS += ../common/util_debugstr.c
SRCS += ../common/util_pmeter.c
SRCS += ../common/winsys/$(WINSYS_SRC).c

OBJS += $(patsubst %.cc,%.o,$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRCS))))

INCLUDES += -I../common/
INCLUDES += -I../common/winsys
INCLUDES += -I../third_party

CFLAGS   += -DVKUTIL_COMMON_DIR=\"../common/\"

LDFLAGS  +=

include ../Makefile.include
