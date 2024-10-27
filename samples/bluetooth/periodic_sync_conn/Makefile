.PHONY: build clean flash

build:
	#west build -b nrf52dk/nrf52832 --sysbuild
	west build -d build/slot1 -b nrf5340_audio_dk/nrf5340/cpuapp --sysbuild -- -DCONFIG_YA_SECOND_SLOT=n
	west build -d build/slot2 -b nrf5340_audio_dk/nrf5340/cpuapp --sysbuild -- -DCONFIG_YA_SECOND_SLOT=y
	#west build -b nrf5340_audio_dk/nrf5340/cpuapp --sysbuild 

clean:
	rm -rf build


flash: f2 f1
	#west flash -i 682758055

f2:
	west flash -d build/slot1 -i 1050138562

f1:
	west flash -d build/slot1 -i 1050143548


