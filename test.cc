#include <iostream>
#include <list>
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

		std::cout << o.get<std::string>("eee") << '\n';
		std::cout << o << '\n';

		std::vector<JSONObject> jv = o.get<JSONObject>("lll").asArray<JSONObject>();

		for (size_t i = 0; i < jv.size(); i++)
			std::cout << jv[i].as<int>();

		JSONObject obj;
		obj.set("intval", 1234);
		obj.set("arrval", v);
		obj.set("doubleval", 100.1);
		obj.set("nullval", cjsonpp::nullObject());

		std::cout << obj << '\n';
		std::list<int> arr2 = obj.get("arrval").asArray<int, std::list>();

		JSONObject arr = cjsonpp::arrayObject();
		arr.add("s1");
		arr.add("s2");
		JSONObject obj2;
		obj2.set("arrval", arr);
		std::cout << obj2 << std::endl;

	} catch (const JSONError& e) {
		std::cout << e.what() << '\n';
	}

	return 0;
}
