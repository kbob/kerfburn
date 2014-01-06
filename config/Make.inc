D := config

GEOM_CONFIG   := config/geometry.py
GEOM_TEMPLATE := config/geom-defs.template.h
GEN_GEOM_DEFS := tools/bin/gen-geom-defs.py

 GEN_PIN_DEFS := tools/bin/gen-pin-defs.py

build-$D: $D/geom-defs.h $D/pin-defs.h

$D/geom-defs.h:
	$(GEN_GEOM_DEFS) --template=$(GEOM_TEMPLATE)            \
                         --mcu-frequency=$(BACK_MCU_FREQ)       \
                         --output=$@                            \
                         $(GEOM_CONFIG)

$D/pin-defs.h:
	$(GEN_PIN_DEFS) --mcu=$(BACK_MCU) -o $@

config/geom-defs.d:
	@$(GEN_GEOM_DEFS) -M -MT $@ -MT config/geom-defs.h      \
                          --template=$(GEOM_TEMPLATE)           \
                          $(GEOM_CONFIG) -o $@

config/pin-defs.d:
	@$(GEN_PIN_DEFS) --mcu=$(BACK_MCU) -M -o config/pin-defs.h > $@

-include config/geom-defs.d
-include config/pin-defs.d
