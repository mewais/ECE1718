run: part6
	./part6 /dev/input/by-id/usb-USB_USB_Keykoard-event-kbd

run_prepare: part6
	insmod ../KEY_SW.ko
	insmod ../LEDR_HEX.ko
	insmod ../stopwatch.ko
	insmod /home/root/Linux_Libraries/drivers/audio.ko
	./part6 /dev/input/by-id/usb-USB_USB_Keykoard-event-kbd

part6: part6.c
	gcc -std=gnu99 -o $@ $^ -lm -pthread

clean:
	rm -rf part6

cleanup:
	rmmod KEY_SW
	rmmod LEDR_HEX
	rmmod audio
	rmmod stopwatch
