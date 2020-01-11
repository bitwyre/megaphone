default:
	g++ -c -std=c++17 -I hiredis -I libuv/include -I uWebSockets/src -I uWebSockets/uSockets/src trades.cpp
	g++ -pthread -ldl trades.o hiredis/libhiredis.a libuv/.libs/libuv.a uWebSockets/uSockets/uSockets.a -lz -o trades

deps:
	WITH_LIBUV=1 make -C uWebSockets/uSockets
