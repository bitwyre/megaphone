#include <iostream>
#include <libphone.hpp>

#include <spdlog/spdlog.h>
#include <zenohc.hxx>

using namespace zenohc;

auto main(int argc, char** argv) -> int {

	zenohc::Config config;
	config = expect(config_from_file(argv[1]));

	auto session = expect<zenohc::Session>(open(std::move(config)));

	auto subscriber = expect<Subscriber>(session.declare_subscriber("demo/example/*", [](const Sample& sample) {
		std::cout << sample.get_payload().as_string_view() << '\t'
				  << sample.get_encoding().get_suffix().as_string_view() << std::endl;
	}));

	// Wait for a key press to exit
	auto c = getchar();
	return 0;
}
