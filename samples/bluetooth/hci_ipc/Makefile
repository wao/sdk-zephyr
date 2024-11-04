.PHONY: build clean fd

build:
	west build --no-sysbuild -d build/5340dk -b nrf5340dk/nrf5340/cpunet -- -DEXTRA_CONF_FILE=nrf5340_cpunet_cis-bt_ll_sw_split.conf -DSHIELD=nrf21540ek -DDTC_OVERLAY_FILE=boards/nrf5340dk_nrf5340_cpunet.overlay

fd:
	west flash -d build/5340dk


clean:
	rm -rf build
