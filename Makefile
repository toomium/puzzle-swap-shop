#---------------------------------------------------------------------------------
# Puzzle Swap Shop: 3DS build (devkitARM, libctru, citro2d)
# Based on the devkitPro 3DS example template.
#
#   make             build puzzle-swap-shop.3dsx
#   make WERROR=1    treat warnings as errors (used by CI)
#   make diag        build puzzle-swap-shop-diag.3dsx for the hardware tests
#   make cia         build puzzle-swap-shop.cia (needs makerom and bannertool)
#   make clean
#
# Host tests for source/core live in Makefile.host.
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# project settings
#---------------------------------------------------------------------------------
TARGET			:=	puzzle-swap-shop
BUILD			:=	build
SOURCES			:=	source source/core source/ui source/ui/widgets source/ui/scenes source/ui/smoke source/platform source/app
DATA			:=	data
INCLUDES		:=	source
GRAPHICS		:=	gfx
ROMFS			:=	romfs
GFXBUILD		:=	$(ROMFS)/gfx

APP_TITLE		:=	Puzzle Swap Shop
APP_DESCRIPTION	:=	Puzzleteile gegen Spielmünzen
APP_AUTHOR		:=	toomium
ICON			:=	assets/icon.png

#---------------------------------------------------------------------------------
# Diagnosis build for the hardware tests (make diag): a text
# console tool instead of the app, without graphics and RomFS.
#---------------------------------------------------------------------------------
ifeq ($(DIAG),1)
TARGET			:=	puzzle-swap-shop-diag
BUILD			:=	build-diag
SOURCES			:=	source/diag source/core source/platform source/app
GRAPHICS		:=
ROMFS			:=
GFXBUILD		:=	$(BUILD)
APP_TITLE		:=	Puzzle Swap Shop Diagnose
APP_DESCRIPTION	:=	Hardware-Tests
endif

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -std=gnu11 -Wall -Wextra -O2 -mword-relocations \
			-ffunction-sections \
			$(ARCH)

ifneq ($(strip $(WERROR)),)
CFLAGS	+=	-Werror
endif

CFLAGS	+=	$(INCLUDE) -D__3DS__

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lcitro2d -lcitro3d -lctru -lm

LIBDIRS	:= $(CTRULIB)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
GFXFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.t3s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

ifeq ($(GFXBUILD),$(BUILD))
export T3XFILES :=  $(GFXFILES:.t3s=.t3x)
else
export ROMFS_T3XFILES	:=	$(patsubst %.t3s, $(GFXBUILD)/%.t3x, $(GFXFILES))
export T3XHFILES		:=	$(patsubst %.t3s, $(BUILD)/%.h, $(GFXFILES))
endif

export OFILES_SOURCES 	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
			$(addsuffix .o,$(T3XFILES))

export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES	:=	$(PICAFILES:.v.pica=_shbin.h) $(SHLISTFILES:.shlist=_shbin.h) \
			$(addsuffix .h,$(subst .,_,$(BINFILES))) \
			$(GFXFILES:.t3s=.h)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export _3DSXDEPS	:=	$(if $(NO_SMDH),,$(OUTPUT).smdh)

export APP_ICON := $(TOPDIR)/$(ICON)

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all clean diag clean-diag

#---------------------------------------------------------------------------------
all: $(BUILD) $(GFXBUILD) $(DEPSDIR) $(ROMFS_T3XFILES) $(T3XHFILES)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

ifneq ($(GFXBUILD),$(BUILD))
$(GFXBUILD):
	@mkdir -p $@
endif

ifneq ($(DEPSDIR),$(BUILD))
$(DEPSDIR):
	@mkdir -p $@
endif

#---------------------------------------------------------------------------------
diag:
	@$(MAKE) --no-print-directory DIAG=1

clean: clean-diag
clean-diag:
	@rm -fr build-diag puzzle-swap-shop-diag.3dsx puzzle-swap-shop-diag.smdh puzzle-swap-shop-diag.elf

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf $(GFXBUILD)

#---------------------------------------------------------------------------------
# CIA: makerom and bannertool must be on PATH or passed as variables.
# The release workflow installs pinned versions of both.
#---------------------------------------------------------------------------------
MAKEROM		?=	makerom
BANNERTOOL	?=	bannertool
VERSION_H	:=	source/core/version.h
pss_version	=	$(shell sed -n 's/^\#define PSS_VERSION_$(1)  *//p' $(VERSION_H))

.PHONY: cia clean-cia

cia: all
	@$(BANNERTOOL) makebanner -i assets/banner.png -a assets/banner.wav -o $(BUILD)/banner.bnr
	@$(MAKEROM) -f cia -o $(TARGET).cia -target t -exefslogo \
		-elf $(TARGET).elf -icon $(TARGET).smdh -banner $(BUILD)/banner.bnr \
		-rsf assets/app.rsf -DAPP_ROMFS=$(ROMFS) \
		-major $(call pss_version,MAJOR) -minor $(call pss_version,MINOR) -micro $(call pss_version,PATCH)
	@echo built ... $(TARGET).cia

clean: clean-cia
clean-cia:
	@rm -f $(TARGET).cia

#---------------------------------------------------------------------------------
$(GFXBUILD)/%.t3x	$(BUILD)/%.h	:	%.t3s
	@echo $(notdir $<)
	@tex3ds -i $< -H $(BUILD)/$*.h -d $(DEPSDIR)/$*.d -o $(GFXBUILD)/$*.t3x

#---------------------------------------------------------------------------------
else

$(OUTPUT).3dsx	:	$(OUTPUT).elf $(_3DSXDEPS)

$(OFILES_SOURCES) : $(HFILES)

$(OUTPUT).elf	:	$(OFILES)

%.bin.o	%_bin.h :	%.bin
	@echo $(notdir $<)
	@$(bin2o)

.PRECIOUS	:	%.t3x %.shbin

%.t3x.o	%_t3x.h :	%.t3x
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

%.shbin.o %_shbin.h : %.shbin
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
