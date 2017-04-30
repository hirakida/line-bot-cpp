default: app

run:
	./app -c cppcms.js

app: demo.cpp
	c++ -std=c++11 demo.cpp -o app -Lcppcms/lib -Icppcms/include -lcppcms -lbooster -lz -lcurl

clean:
	rm -f app
