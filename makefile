myhash: main.o
	gcc -o myhash main.o -lpthread
main.o: main.c
	gcc -c main.c
clean:
	rm *.o myhash
