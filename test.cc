#include <iostream>
#include "cjsonpp.h"

int main()
{
	using namespace cjsonpp;
	try {
		JSONObject o;
		std::vector<int> v = {1, 2, 3, 4};


		o.set("test", 1234);
		o.set("eee", "1234");
		o.set("rrr", "vvv");
		o.set("lll", v);

		std::cout << o.get("eee").as<std::string>() << '\n';
		std::cout << o.as<std::string>("eee") << '\n';

		std::cout << o << '\n';

		std::vector<JSONObject> jv = o.get("lll").asArray<JSONObject>();

		for (size_t i = 0; i < jv.size(); i++)
			std::cout << jv[i].as<int>();

	} catch (const JSONError& e) {
		std::cout << e.what() << '\n';
	}

	return 0;
}
