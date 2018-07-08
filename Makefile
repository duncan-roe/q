# default paths

include config.mak

.PHONY: q clean
q:
	cd q && $(MAKE) \
ETC_DIR=$(ETC_DIR) \
HELP_CMD=$(HELP_CMD) \
HELP_DIR=$(HELP_DIR) \
MACRO_DIR=$(MACRO_DIR)
clean:
	cd  q && $(MAKE) clean
