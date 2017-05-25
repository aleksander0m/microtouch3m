
################################################################################
# generic udev rules
################################################################################

if INSTALL_UDEV_RULES

udevrulesdir = $(UDEV_BASE_DIR)/rules.d
udevrules_DATA = \
	99-microtouch3m.rules \
	$(NULL)

endif

EXTRA_DIST = \
	$(udevrules_DATA) \
	$(NULL)
