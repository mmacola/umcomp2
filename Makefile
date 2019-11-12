all:
	mkdir -p build
	gcc src/server/server.c -lpthread -o build/server
	gcc src/client/client.c -lpthread -o build/client
