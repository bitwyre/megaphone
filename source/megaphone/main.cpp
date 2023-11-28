#include <iostream>
#include <time.h>

#include <App.h>
#include <libphone.hpp>

uWS::App* globalApp;
static int64_t counter = 0;

int main() {
	LibPhone::Phone phone {};

	struct us_loop_t* loop = (struct us_loop_t*)uWS::Loop::get();
	struct us_timer_t* delayTimer = us_create_timer(loop, 0, 0);

	// broadcast the unix time as millis every 8 millis
	us_timer_set(
		delayTimer,
		[](struct us_timer_t* /*t*/) {
			struct timespec ts;
			timespec_get(&ts, TIME_UTC);

			globalApp->publish("broadcast", "Hello, Megaphone: " + std::to_string(counter), uWS::OpCode::BINARY, false);
			counter++;
		},
		8, 8);

	globalApp = &phone.get_app();

	phone.run();
}
