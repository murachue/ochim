DEBUG ?= 0
PROFILE ?= 0
EMU ?= 0
GAMELANG ?= en_US

ifneq ($(DEBUG),0)
CFLAGS_DEBUG = -O0 -DDEBUG -Wno-unused-function -Wno-unused-variable
LDFLAGS_DEBUG =
else
ifneq ($(PROFILE),0)
CFLAGS_DEBUG = -O3 -flto -DPROFILE
LDFLAGS_DEBUG = -flto
else
CFLAGS_DEBUG = -O3 -flto
LDFLAGS_DEBUG = -flto
endif
endif

ifneq ($(EMU),0)
CFLAGS_DEBUG += -DEMU
endif

N64_INST ?= /usr/local
#TRIPLET ?= mips64-elf
TRIPLET ?= mips-elf
TOOLCHAIN ?= $(N64_INST)/bin/$(TRIPLET)-
CC=$(TOOLCHAIN)gcc
AS=$(TOOLCHAIN)as
CPP=$(TOOLCHAIN)cpp
RSPAS=$(AS)
RSPLD=$(TOOLCHAIN)ld
LD=$(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
N64TOOL = $(N64_INST)/bin/n64tool
CHKSUM64PATH = $(N64_INST)/bin/chksum64
HEADERPATH = $(N64_INST)/$(TRIPLET)/lib/header
#ABI = -mabi=64 -msym32
ABI ?= -mabi=32
COMMONFLAGS ?= -march=vr4300 -mtune=vr4300 $(ABI)
CFLAGS ?= $(COMMONFLAGS) -std=gnu99 -ffunction-sections -fdata-sections -G0 -ggdb3 -Wall -Werror --no-common $(CFLAGS_DEBUG) -I.
ASFLAGS ?= $(COMMONFLAGS) -g
RSPASFLAGS ?= -g -mips1 -mrsp
LDFLAGS ?= -Wl,--gc-sections $(LDFLAGS_DEBUG)
# ROM must be larger than 2M
ROMSIZE = 2M

STUBOBJS = gdbstub.o gdbstubl.o

TITLE = "ochimono"
BASENAME = ochim
ROM = $(BASENAME).z64
BIN = $(BASENAME).bin
ELF = $(BASENAME).elf
DEBUGELF = $(BASENAME).debug
OBJDIR = obj
RESDIR = res
OBJS = entry.o main.o rdp.o hotload.o cache.o $(STUBOBJS)
TEXS = ochimtex.o minotex.o turutex.o numtex.o gtypetex.o guitex.o back2.o
SNDS = erase1.o erase2.o garbage.o go.o holdfail.o hold.o initialhold.o initialrotate.o linefall.o lock.o ready.o rotate.o step.o point1.o point2.o point3.o point4.o rakka.o regret.o se000026.o ssshk1_r.o move.o sewol001.o brmpshrr.o shakin.o tamaf.o tamaido.o pageenter2.o pageexit2.o
UCODES = rspaudio.elf
# gcc does not append... why?
LIBS = -lm -lc

.PHONY: all clean

all: $(ROM)

clean:
	-rm $(ROM) $(BIN:%=$(OBJDIR)/%) $(ELF) $(ELF).map $(DEBUGELF) $(OBJS:%=$(OBJDIR)/%) $(TEXS:%=$(OBJDIR)/%) $(TEXS:%.o=$(OBJDIR)/%.s) $(TEXS:%.o=$(OBJDIR)/%.raw) $(SNDS:%=$(OBJDIR)/%) $(SNDS:%.o=$(OBJDIR)/%.s) $(SNDS:%.o=$(OBJDIR)/%.raw) $(UCODES:%=$(OBJDIR)/%) $(UCODES:%.elf=$(OBJDIR)/%.o)
	-rmdir $(OBJDIR)

tags:
	ctags *.c *.S

reflags: clean-objs all

clean-objs:
	-rm $(OBJS:%=$(OBJDIR)/%)

$(OBJDIR):
	mkdir -p $@

$(ROM): $(BIN:%=$(OBJDIR)/%)
	rm -f $@
	$(N64TOOL) -l $(ROMSIZE) -t $(TITLE) -h $(HEADERPATH) -o $@ $<
	$(CHKSUM64PATH) $@

$(BIN:%=$(OBJDIR)/%): $(ELF) | $(OBJDIR)
	$(OBJCOPY) $< $@ -O binary

$(DEBUGELF): $(ELF)
	$(OBJCOPY) --only-keep-debug $< $@

ALLOBJS = $(OBJS:%=$(OBJDIR)/%) $(TEXS:%=$(OBJDIR)/%) $(SNDS:%=$(OBJDIR)/%) $(UCODES:%=$(OBJDIR)/%)
$(ELF): $(ALLOBJS) rom.x
	$(LD) -Trom.x -Wl,-Map,$@.map -o $@ $(ALLOBJS) $(LIBS) $(LDFLAGS)

$(OBJDIR)/%.o: %.S | $(OBJDIR)
	$(CC) $(ASFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/entry.o: entry.S
$(OBJDIR)/main.o: main.c rdp.h cache.h regs.h
$(OBJDIR)/cache.o: cache.c cache.h
$(OBJDIR)/rdp.o: rdp.c rdp.h cache.h regs.h
$(OBJDIR)/hotload.o: hotload.c hotload.h regs.h

$(OBJDIR)/rspaudio.o: rspaudio.S
	$(CPP) $< | $(RSPAS) $(RSPASFLAGS) -o $@ -

$(OBJDIR)/rspaudio.elf: $(OBJDIR)/rspaudio.o
	$(RSPLD) -Trsp.x -o $@ $<

define tex2o
$(1:%=$(OBJDIR)/%): $(1:%.o=$(OBJDIR)/%.s) $(1:%.o=$(OBJDIR)/%.raw) | $(OBJDIR)

$(1:%.o=$(OBJDIR)/%.s): | $(OBJDIR)
	echo '.section .rodata' > $$@
	echo '.globl $(1:%.o=%)' >> $$@
	echo '.balign 8' >> $$@
	echo '$(1:%.o=%): .incbin "$$(@:%.s=%.raw)"' >> $$@

endef
$(foreach obj,$(TEXS),$(eval $(call tex2o,$(obj))))

# ia44
$(OBJDIR)/ochimtex.raw: $(RESDIR)/ochimtex_$(GAMELANG).png | $(OBJDIR)
	convert $< -depth 8 rgba:- | ruby imgutil.rb rxxa8888toia44 $@

# rgba5551, with preswapping and interleaved
$(OBJDIR)/back2.raw: $(RESDIR)/back2.png | $(OBJDIR)
	convert $< -depth 8 rgb:- | ruby imgutil.rb rgb888torgba5551be | ruby imgutil.rb 640x480i_swap_interleave $@

# ia44
$(OBJDIR)/minotex.raw: $(RESDIR)/minotex.png | $(OBJDIR)
	convert $< -depth 8 rgba:- | ruby imgutil.rb rxxa8888toia44 $@

# ia44 with blockize
$(OBJDIR)/turutex.raw: $(RESDIR)/turutex.png | $(OBJDIR)
	convert $< \
		-crop 16x16 -append \
		-depth 8 rgba:- | ruby imgutil.rb rxxa8888toia44 $@

# ia44 with blockize; to use dp_load_block width*bypp must be multiple of 8... extent it.
$(OBJDIR)/numtex.raw: $(RESDIR)/numtex.png | $(OBJDIR)
	convert $< \
		-crop 12x16 -background none -extent 16x16! -append \
		-depth 8 rgba:- | ruby imgutil.rb rxxa8888toia44 $@

# rgba5551
$(OBJDIR)/gtypetex.raw: $(RESDIR)/gtypetex.png | $(OBJDIR)
	convert $< -depth 8 rgba:- | ruby imgutil.rb rgba8888torgba5551be $@

# ia44
$(OBJDIR)/guitex.raw: $(RESDIR)/guitex.png | $(OBJDIR)
	convert $< -depth 8 rgba:- | ruby imgutil.rb rxxa8888toia44 $@

define wav2o
$(1:%=$(OBJDIR)/%): $(1:%.o=$(OBJDIR)/%.s) $(1:%.o=$(OBJDIR)/%.raw) | $(OBJDIR)

$(1:%.o=$(OBJDIR)/%.s): | $(OBJDIR)
	echo '.section .rodata' > $$@
	echo '.globl $(1:%.o=%)' >> $$@
	echo '.balign 8' >> $$@
	echo '$(1:%.o=%): .incbin "$$(@:%.s=%.raw)"' >> $$@
	echo '.globl $(1:%.o=%)_size' >> $$@
	echo '.set $(1:%.o=%)_size, . - $(1:%.o=%)' >> $$@

$(1:%.o=$(OBJDIR)/%.raw): $(1:%.o=$(RESDIR)/%.wav) | $(OBJDIR)
	sox $$< -t s16 -r 22050 --endian big - | ruby adpcmenc.rb > $$@

endef
$(foreach obj,$(SNDS),$(eval $(call wav2o,$(obj))))


$(OBJDIR)/gdbstubl.o: gdbstubl.S gdbstub.h | $(OBJDIR)
$(OBJDIR)/gdbstub.o: gdbstub.c gdbstub.h | $(OBJDIR)
