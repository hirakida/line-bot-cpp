default: app

run:
	./app -c cppcms.js

app: src/index.cpp
	c++ -std=c++11 src/index.cpp -o app -Lcppcms/lib -Icppcms/include -lcppcms -lbooster -lz -lcurl

clean:
	rm -f app
