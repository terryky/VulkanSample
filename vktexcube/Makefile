include ../Makefile.env

TARGET = vktexcube

SRCS = 
SRCS += main.c
SRCS += ../common/vk_init.c
SRCS += ../common/vk_debug.c
SRCS += ../common/vk_render.c
SRCS += ../common/util_matrix.c
SRCS += ../common/winsys/$(WINSYS_SRC).c

OBJS += $(patsubst %.cc,%.o,$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRCS))))

INCLUDES += -I../common/
INCLUDES += -I../common/winsys
INCLUDES += -I../third_party

CFLAGS   +=

LDFLAGS  +=

include ../Makefile.include
