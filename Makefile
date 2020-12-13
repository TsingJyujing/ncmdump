all:
	g++ -std=c++17 main.cpp cJSON.cpp aes.cpp ncmcrypt.cpp -o ncmdump -ltag -I $(realpath .)
	strip ncmdump

install: all
	mv ncmdump /usr/local/bin
