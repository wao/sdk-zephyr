.PHONY: build cis clean

build:
	mkdir -p build
	#west build -d build/nrf5340dk -b nrf5340dk_nrf5340_cpunet  -- -DOVERLAY_CONF=nrf5340_cpunet_cis-bt_ll_sw_split.conf
	west build -d build/nrf5340dk -b nrf5340dk_nrf5340_cpunet  -- -DEXTRA_CONF_FILE=nrf5340_cpunet_cis-bt_ll_sw_split.conf
	west build -d build/nrf5340audio -b nrf5340_audio_dk_nrf5340_cpunet  -- -DEXTRA_CONF_FILE=nrf5340_cpunet_cis-bt_ll_sw_split.conf
	#west build -d build/nrf5340audio -b nrf5340_audio_dk_nrf5340_cpunet  -- -DCONF_FILE=nrf5340_cpunet_bis-bt_ll_sw_split.conf
	#west build -d build/nrf5340audio_fem -b nrf5340_audio_dk_nrf5340_cpunet  -- -DCONF_FILE=nrf5340_cpunet_bis-bt_ll_sw_split.conf -DEXTRA_CONF_FILE=fem.conf -DEXTRA_DTC_OVERLAY_FILE=my_nrf21540_ek.overlay
	#west build -d build/nrf5340audio_cis -b nrf5340_audio_dk_nrf5340_cpunet  -- -DCONF_FILE=nrf5340_cpunet_cis-bt_ll_sw_split.conf

clean:
	/usr/bin/rm -rf build

fdk:
	west flash -d	build/nrf5340dk -i 1050029049
	west flash -d	build/nrf5340dk -i 1050048541


fad:
	#west flash -d	build/nrf5340audio -i 1050138562
	west flash -d	build/nrf5340audio 
