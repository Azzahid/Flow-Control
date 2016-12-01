
all:
	@echo ">> Check directory bin and make it if doesn't exits"
	@mkdir -p bin
	@echo ">> Build Receiver"
	@g++ -pthread src/receiver.cpp -o bin/receiver
	@echo ">> Build Transmitter"
	@g++ src/transmitter.cpp -o bin/transmitter
	@echo ">> Finished!"

receiver.o: src/receiver.cpp
	g++ -c -pthread src/receiver.cpp

transmitter.o: src/transmitter.cpp
	g++ -c -Wall src/transmitter.cpp

receiver:
	@echo ">> Build Receiver"
	@g++ -pthread src/receiver.cpp -o bin/receiver
	@echo ">> Finished!"

transmitter:
	@echo ">> Build Transmitter"
	@g++ src/transmitter.cpp -o bin/transmitter
	@echo ">> Finished!"

clean:
	@echo ">> Removing all build file."
	@rm -f *.o -Rf bin
	@echo ">> Clean Finished!"


init:
	@echo ">> Make Bin Directory..."
	@mkdir -p bin
	@echo ">> Init Finished!"
