default:
	g++ -O3 -flto -c -std=c++17 -I hiredis -I libuv/include -I rapidjson/include/rapidjson -I uWebSockets/src -I uWebSockets/uSockets/src megaphone.cpp
	# Link with our libuv and hiredis static libs (built in deps)
	g++ -O3 -flto -pthread -Wl,--no-as-needed -ldl megaphone.o hiredis/libhiredis.a libuv/.libs/libuv.a uWebSockets/uSockets/uSockets.a -lz -o megaphone

deps:
	# Build libuv (requires autotools, automake, autoconf)
	cd libuv && ./autogen.sh && CFLAGS="-fPIC" ./configure --enable-shared=false && make
	# Build uSockets using our libuv
	CFLAGS="-I ../../libuv/include" WITH_LIBUV=1 make -C uWebSockets/uSockets
	# Build hiredis static libs
	make -C hiredis

clean:
	rm -rf megaphone megaphone.o && echo "megaphone cleaned"
