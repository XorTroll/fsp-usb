
.PHONY: all clean


all:
	@$(MAKE) -C libstratosphere/
	@$(MAKE) -C fsp-usb/

clean:
	@$(MAKE) clean -C libstratosphere/
	@$(MAKE) clean -C fsp-usb/