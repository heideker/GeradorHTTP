# for raspbian, include libcurl4-openssl-dev package
all: 
	gcc -Wall -o clienteHTTP client.cpp -lstdc++ -lcurl -lpthread -std=c++11 -lm

