run: part5
	./part5 /dev/input/by-id/usb-USB_USB_Keykoard-event-kbd

run_prepare: part5
	insmod ../KEY_SW.ko
	insmod ../LEDR_HEX.ko
	insmod ../stopwatch.ko
	./part5 /dev/input/by-id/usb-USB_USB_Keykoard-event-kbd

part5: part5.c
	gcc -std=gnu99 -o $@ $^ -lm -pthread

clean:
	rm -rf part5

cleanup:
	rmmod KEY_SW
	rmmod LEDR_HEX
	rmmod stopwatch
