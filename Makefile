default: app

run:
	./app -c cppcms.js

app: main.cpp
	c++ -std=c++11 main.cpp -o app -Lcppcms/lib -Icppcms/include -lcppcms -lbooster -lz -lcurl

clean:
	rm -f app
