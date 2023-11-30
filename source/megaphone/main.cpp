#include <iostream>

//
// zenoh.hxx automatically selects zenoh-c or zenoh-pico C++ wrapper
// depending on ZENOHCXX_ZENOHPICO or ZENOHCXX_ZENOHC setting
// and places it to the zenoh namespace
//
#include "zenohpico.hxx"
#include <cstddef>
#include <iostream>

class CustomerClass {
public:
	CustomerClass(CustomerClass&&) = delete;
	CustomerClass(const CustomerClass&) = delete;
	CustomerClass& operator=(const CustomerClass&) = delete;
	CustomerClass& operator=(CustomerClass&&) = delete;

	CustomerClass(zenohpico::Session& session, const zenohpico::KeyExprView& keyexpr) : pub(nullptr) {
		pub = zenohpico::expect<zenohpico::Publisher>(session.declare_publisher(keyexpr));
	}

	void put(const zenohpico::BytesView& value) { pub.put(value); }

private:
	zenohpico::Publisher pub;
};

int main(int, char**) {
	try {
		zenohpico::Config config;
		auto session = zenohpico::expect<zenohpico::Session>(zenohpico::open(std::move(config)));
		std::string keyexpr = "demo/example/simple";
		std::string value = "Simple!";
		CustomerClass customer(session, keyexpr);
		uint64_t counter {};
		while (true) {
			customer.put(value + std::to_string(counter++));
		}
	} catch (zenohpico::ErrorMessage e) {
		std::cout << "Error: " << e.as_string_view() << std::endl;
	}
}
