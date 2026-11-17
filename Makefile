# Needs devkitPro: devkitARM, libctru, citro2d/citro3d, 3dstools (3dsxtool, smdhtool),
# plus bannertool + makerom on PATH for the .cia (home-menu banner).
ifeq ($(strip $(DEVKITARM)),)
$(error Set DEVKITARM, e.g. export DEVKITARM=/opt/devkitpro/devkitARM)
endif
include $(DEVKITARM)/base_rules

TARGET   := catastrophe_mixer
TITLE    := Catastrophe Mixer
CTRULIB  := $(DEVKITPRO)/libctru
PORTLIBS := $(DEVKITPRO)/portlibs/3ds
ARCH     := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS   := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) -std=gnu11 -D__3DS__ \
            -I$(CTRULIB)/include -I$(PORTLIBS)/include
LDFLAGS  := -specs=3dsx.specs -g $(ARCH) -L$(CTRULIB)/lib -L$(PORTLIBS)/lib
LIBS     := -lcitro2d -lcitro3d -lctru -lm

.PHONY: all cia clean
all: $(TARGET).3dsx
cia: $(TARGET).cia

$(TARGET).elf: source/main.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -o $@
$(TARGET).smdh: icon.png
	smdhtool --create "$(TITLE)" "Mix natural disasters. Destroy the city." "You" icon.png $@
$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --smdh=$(TARGET).smdh
banner.bnr: banner.png audio.wav
	bannertool makebanner -i banner.png -a audio.wav -o $@
icon.icn: icon.png
	bannertool makesmdh -s "$(TITLE)" -l "Mix natural disasters. Destroy the city." -p "You" -i icon.png -o $@
$(TARGET).cia: $(TARGET).elf banner.bnr icon.icn app.rsf
	makerom -f cia -o $@ -elf $(TARGET).elf -rsf app.rsf -target t -exefslogo -icon icon.icn -banner banner.bnr
clean:
	rm -f *.elf *.3dsx *.smdh *.cia *.bnr *.icn
