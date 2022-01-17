CFG_TEE_TA_LOG_LEVEL ?= 4

# The UUID for the Trusted Application
BINARY=0183a595-0fd1-4037-82b0-1fc25c647744

CFLAGS += -march=armv8-a+crypto

-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
	@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif
