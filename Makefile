receiver.o: receiver.c
	gcc -c -pthread src/receiver.c

receiver:
	g++ -pthread src/receiver.cpp -o bin/receiver

clean:
	rm *.o hello
