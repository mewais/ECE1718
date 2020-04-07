run: part3
	insmod /home/root/Linux_Libraries/drivers/video.ko
	insmod ../part2/signal_generator.ko
	./part3

part3: part3.c
	gcc -Wall -std=gnu99 -o $@ $^ -lm -lrt

clean:
	rm -rf part3

cleanup:
	rmmod video
	rmmod signal_generator
