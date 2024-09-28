
export FW_MAJOR := 0
export FW_MINOR := 0

PROJ = samisara
VER := $(FW_MAJOR).$(FW_MINOR)

PYTHON := python3

export ROOT := $(CURDIR)

.PHONY: FORCE

.DEFAULT_GOAL := all

prod-%: FORCE
	$(MAKE) target mcu=$* target=$(PROJ) level=prod

debug-%: FORCE
	$(MAKE) target mcu=$* target=$(PROJ) level=debug

all-%: FORCE prod-% debug-% ;

all: FORCE all-at32f4 ;

clean: FORCE
	rm -rf out

out: FORCE
	+mkdir -p out/$(mcu)/$(level)/$(target)

target: FORCE out
	$(MAKE) -C out/$(mcu)/$(level)/$(target) -f $(ROOT)/Rules.mk target.bin target.hex target.dfu $(mcu)=y $(level)=y $(target)=y

dist: level := prod
dist: t := $(ROOT)/out/$(PROJ)-$(VER)
dist: FORCE all
	rm -rf out/$(PROJ)-*
	mkdir -p $(t)
	cd out/at32f4/$(level)/$(PROJ); \
	  cp -a target.hex $(t)/$(PROJ)-at32f4-$(VER).hex; \
	  cp -a target.dfu $(t)/$(PROJ)-at32f4-$(VER).dfu
	cp -a COPYING $(t)/
	cp -a README $(t)/
	cp -a RELEASE_NOTES $(t)/
	cd out && zip -r $(PROJ)-$(VER).zip $(PROJ)-$(VER)
