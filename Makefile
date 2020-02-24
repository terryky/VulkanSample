#
# > make TARGET_ENV=edgetpu_devboard
#

TARGET_ENV ?= default
#TARGET_ENV = edgetpu_devboard

all:
	make -C query_vk_devices
	make -C vkclear
	make -C vktri
	make -C vktexture
	make -C vktexcube
	make -C vkteapot
	make -C vkfbo

clean:
	make -C query_vk_devices clean
	make -C vkclear clean
	make -C vktri clean
	make -C vktexture clean
	make -C vktexcube clean
	make -C vkteapot clean
	make -C vkfbo clean
