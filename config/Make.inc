D := config

GEN_PIN_DEFS := tools/bin/gen-pin-defs.py

$D/pin-defs.h: $D/${BACK_MCU}.py $(GEN_PIN_DEFS)
	$(GEN_PIN_DEFS) --mcu=$(BACK_MCU) -o $@

config/pin-defs.d:
	@$(GEN_PIN_DEFS) --mcu=$(BACK_MCU) -M -o config/pin-defs.h > $@

-include config/pin-defs.d
