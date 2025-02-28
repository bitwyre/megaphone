#include <libphone.hpp>

#include <spdlog/spdlog.h>
#include <zenohc.hxx>

auto main(int argc, char** argv) -> int {
	if (argc < 2) {
		spdlog::error("Config file path not provided");
		return 1;
	}

	try {
		zenohc::Config config;
		config = zenohc::expect(zenohc::config_from_file(argv[1]));

		auto session = zenohc::expect<zenohc::Session>(zenohc::open(std::move(config)));

		// auto subscriber = expect<Subscriber>(session.declare_subscriber("demo/example/*", [](const Sample& sample) {
		// 	std::cout << sample.get_payload().as_string_view() << '\t'
		// 			  << sample.get_encoding().get_suffix().as_string_view() << std::endl;
		// }));

		// // Wait for a key press to exit
		// auto c = getchar();

		try {
			LibPhone::Phone phone {};
			phone.run(session);
		} catch (const std::exception& e) {
			spdlog::error("Fatal error: {}", e.what());
			return 1;
		} catch (...) {
			spdlog::error("Unknown fatal error occurred");
			return 1;
		}

		return 0;
	} catch (const zenohc::ErrorMessage& e) {
		spdlog::error("Zenoh config error: {}", e.as_string_view());
		return 1;
	} catch (const std::exception& e) {
		spdlog::error("Failed to initialize Zenoh: {}", e.what());
		return 1;
	}
}
