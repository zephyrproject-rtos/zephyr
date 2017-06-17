
# Sensor Subsystem
SS_APP = sensor

# Application Processor
APP = ap

all: sensor_app ap_app

sensor_app:
	$(MAKE) -C $(SS_APP) O=$(CURDIR)/outdir/$(SS_APP)

ap_app:
	$(MAKE) -C $(APP) O=$(CURDIR)/outdir/$(APP)

flash: sensor_app ap_app
	$(MAKE) -C $(SS_APP) O=outdir/$(SS_APP) flash
	$(MAKE) -C $(APP) O=outdir/$(APP) flash

pristine:
	@rm -rf $(CURDIR)/outdir

