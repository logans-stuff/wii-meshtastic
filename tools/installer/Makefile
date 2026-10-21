# One-shot SD installer -> build/meshtasticd-installer.dol (install.sh builds
# it in the devkitPPC container). Embeds ../../hbc/{boot.dol,meta.xml,icon.png}
# and writes them to sd:/apps/meshtasticd/; config.txt, prefs/ and maps/ there
# are left alone.
include $(DEVKITPPC)/base_tools

MACHDEP := -DGEKKO -mrvl -mcpu=750 -meabi -mhard-float
CFLAGS  := -O2 -Wall $(MACHDEP) -I$(DEVKITPRO)/libogc/include -Ibuild
LDFLAGS := $(MACHDEP) -L$(DEVKITPRO)/libogc/lib/wii
HBC     := ../../hbc

all: build/meshtasticd-installer.dol

build/install_data.h: make_install_data.py $(wildcard $(HBC)/*)
	@mkdir -p build
	python3 $< --app-dir $(HBC) --output $@

build/installer.o: installer.c build/install_data.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/meshtasticd-installer.elf: build/installer.o
	$(CC) $(LDFLAGS) -o $@ $< -lfat -lwiiuse -lbte -logc -lm

build/meshtasticd-installer.dol: build/meshtasticd-installer.elf
	elf2dol $< $@

clean:
	rm -rf build

.PHONY: all clean
