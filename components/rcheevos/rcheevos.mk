################################################################################
#
# rcheevos
#
################################################################################

RCHEEVOS_VERSION = 12.5.0
RCHEEVOS_SITE = $(call github,RetroAchievements,rcheevos,v$(RCHEEVOS_VERSION))
# The github helper yields a bare ".../archive/<ref>" URL. Like cjson.mk, name
# the tarball explicitly so the download URL keeps the ".tar.gz" suffix that
# GitHub needs before it redirects to codeload.
RCHEEVOS_SOURCE = rcheevos-$(RCHEEVOS_VERSION).tar.gz
RCHEEVOS_LICENSE = MIT
RCHEEVOS_LICENSE_FILES = LICENSE
RCHEEVOS_INSTALL_STAGING = YES

# Only the pieces Phobos needs: the runtime, the API parsers used to read a
# cached "patch" response, and the hash helpers. rc_client is deliberately
# excluded because it owns the HTTP session, and rc_validate needs the
# validator-only signatures.
RCHEEVOS_SOURCES = \
	src/rc_compat.c \
	src/rc_libretro.c \
	src/rc_util.c \
	src/rc_version.c \
	src/rapi/rc_api_common.c \
	src/rapi/rc_api_runtime.c \
	src/rhash/aes.c \
	src/rhash/cdreader.c \
	src/rhash/hash.c \
	src/rhash/hash_disc.c \
	src/rhash/hash_encrypted.c \
	src/rhash/hash_rom.c \
	src/rhash/hash_zip.c \
	src/rhash/md5.c \
	src/rcheevos/alloc.c \
	src/rcheevos/condition.c \
	src/rcheevos/condset.c \
	src/rcheevos/consoleinfo.c \
	src/rcheevos/format.c \
	src/rcheevos/lboard.c \
	src/rcheevos/memref.c \
	src/rcheevos/operand.c \
	src/rcheevos/richpresence.c \
	src/rcheevos/runtime.c \
	src/rcheevos/runtime_progress.c \
	src/rcheevos/trigger.c \
	src/rcheevos/value.c

RCHEEVOS_CFLAGS = \
	-DRC_NO_THREADS \
	-DRC_DISABLE_LUA \
	-I$(@D)/include \
	-I$(@D)/src

define RCHEEVOS_BUILD_CMDS
	mkdir -p $(@D)/build
	$(foreach s,$(RCHEEVOS_SOURCES), \
		$(TARGET_CC) $(TARGET_CFLAGS) $(RCHEEVOS_CFLAGS) \
			-c $(@D)/$(s) -o $(@D)/build/$(notdir $(basename $(s))).o;)
	$(TARGET_AR) rcs $(@D)/build/librcheevos.a $(@D)/build/*.o
endef

define RCHEEVOS_INSTALL_STAGING_CMDS
	$(INSTALL) -d $(STAGING_DIR)/usr/include $(STAGING_DIR)/usr/lib
	$(INSTALL) -m 0644 $(@D)/include/*.h $(STAGING_DIR)/usr/include/
	$(INSTALL) -m 0644 $(@D)/src/rc_libretro.h $(STAGING_DIR)/usr/include/
	$(INSTALL) -m 0644 $(@D)/build/librcheevos.a $(STAGING_DIR)/usr/lib/
endef

$(eval $(generic-package))
