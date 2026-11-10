# Cobra Arena v2 - Makefile (PSL1GHT)
#
# Requer o toolchain instalado e as variáveis:
#   PS3DEV=/opt/ps3dev
#   PSL1GHT=$(PS3DEV)/PSL1GHT
#
# ATENÇÃO: as regras de .self/.pkg abaixo seguem o padrão do PSL1GHT.
# Se o seu workflow que já funciona usa outros comandos, copie-os para cá.

include $(PSL1GHT)/ppu_rules

TARGET   := cobra_arena
TITLE    := Cobra Arena
TITLE_ID := COBRA0001
CONTENT  := UP0001-COBRA0001_00-COBRAGAMEAPP0001
PKG_DIR  := pkg
PKG_OUT  := $(TARGET).pkg

SOURCES  := .
INCLUDE  :=
LIBS     := -lsysutil -lio -lgcm_sys -lsysmodule -lm -lrt -lc
LIBPATHS :=

.PHONY: pkg

# gera o .self (EBOOT.BIN) pela regra padrão do ppu_rules
pkg: $(TARGET).self
	mkdir -p $(PKG_DIR)/USRDIR
	cp $(TARGET).self $(PKG_DIR)/USRDIR/EBOOT.BIN
	python3 tools/make_sfo.py $(PKG_DIR)/PARAM.SFO "$(TITLE)" $(TITLE_ID) $(CONTENT) 01.00
	python3 tools/make_icon.py $(PKG_DIR)/ICON0.PNG
	pkg.py --contentid $(CONTENT) $(PKG_DIR) $(PKG_OUT)
	@echo "Pronto: $(PKG_OUT)"
