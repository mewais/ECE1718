run: part4
	insmod /home/root/Linux_Libraries/drivers/KEY.ko
	insmod /home/root/Linux_Libraries/drivers/video.ko
	insmod ../part2/signal_generator.ko
	./part4

debug: part4.c
	gcc -Wall -std=gnu99 -DDEBUG -o part4 $^ -lm -lrt

part4: part4.c
	gcc -Wall -std=gnu99 -o $@ $^ -lm -lrt

clean:
	rm -rf part4

cleanup:
	rmmod KEY
	rmmod video
	rmmod signal_generator
