
PROJ := samisara
PYTHON := python3

export BUILD_VER := $(shell $(PYTHON) -c \
    'from setuptools_scm import get_version; print(get_version())')
$(if $(BUILD_VER),,$(error Failed))
export BUILD_DATE := $(shell date -u '+%F %H:%M:%S %Z')

export ROOT := $(CURDIR)

.PHONY: FORCE

.DEFAULT_GOAL := all

version: FORCE
	@echo $(BUILD_VER)

prod-%: FORCE
	$(MAKE) target mcu=$* target=$(PROJ) level=prod

debug-%: FORCE
	$(MAKE) target mcu=$* target=$(PROJ) level=debug

all-%: FORCE prod-% debug-% ;

all: FORCE all-at32f4

clean: FORCE
	rm -rf out

out: FORCE
	+mkdir -p out/$(mcu)/$(level)/$(target)

target: FORCE out
	$(MAKE) -C out/$(mcu)/$(level)/$(target) -f $(ROOT)/Rules.mk target.bin target.hex target.dfu $(mcu)=y $(level)=y $(target)=y

dist: FORCE all
	rm -rf out/$(PROJ)-*
	$(MAKE) _dist level=prod n=$(PROJ)-$(BUILD_VER)
	$(MAKE) _dist level=debug n=$(PROJ)-$(BUILD_VER)-debug

_dist: t := $(ROOT)/out/$(n)
_dist: FORCE
	mkdir -p $(t)
	cd out/at32f4/$(level)/$(PROJ); \
	  cp -a target.hex $(t)/$(PROJ)-at32f4-$(BUILD_VER).hex; \
	  cp -a target.dfu $(t)/$(PROJ)-at32f4-$(BUILD_VER).dfu
	cp -a COPYING $(t)/
	cp -a README $(t)/
	cd out && zip -r $(n).zip $(n)
