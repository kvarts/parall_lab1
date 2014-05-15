myhash: main.o
	gcc main.o -o myhash -lm -lpthread -g
main.o: main.c
	gcc -c main.c -lm -lpthread -g
clean:
	rm *.o myhash
