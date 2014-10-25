#include <iostream>
#include <list>
#include "cjsonpp.h"

cjsonpp::JSONObject create_arr()
{
	cjsonpp::JSONObject obj;

	cjsonpp::JSONObject arr = cjsonpp::arrayObject();
	arr.add("foo");
	arr.add("bar");

	obj.set("arr", arr);
	return obj.get<cjsonpp::JSONObject>("arr");
}

int main()
{
	using namespace cjsonpp;
	try {
		JSONObject o;
#ifdef WITH_CPP11
		std::vector<int> v = {1, 2, 3, 4};
		JSONObject vo = {1, 2, 3, 4};
#else
		std::vector<int> v;
		v.push_back(1); v.push_back(2); v.push_back(3); v.push_back(4);
		JSONObject vo = cjsonpp::arrayObject();
		vo.add(1); vo.add(2); vo.add(3); vo.add(4);
#endif
		o.set("num", 1234);
		o.set("str1", "1234");
		o.set("str2", "vvv");
		o.set("v", v);
		o.set("vo", vo);

		std::cout << o.get<std::string>("str1") << '\n';
		std::cout << parse(o.print()) << '\n';

#ifdef WITH_CPP11
		std::vector<JSONObject> jv = o.get("v").asArray();
		std::vector<int> iv = o.get("vo").asArray<int>();
#else
		std::vector<JSONObject> jv = o.get<JSONObject>("v").asArray<JSONObject, std::vector>();
		std::vector<int> iv = o.get<JSONObject>("vo").asArray<int, std::vector>();
#endif

		for (size_t i = 0; i < jv.size(); i++)
			std::cout << jv[i].as<int>() << iv[i];
		std::cout << '\n';

		JSONObject obj;
		obj.set("intval", 1234);
		obj.set("arrval", v);
		obj.set("doubleval", 100.1);
		obj.set("nullval", cjsonpp::nullObject());

		std::cout << obj << '\n';
		std::list<int> arr2 = obj.get<JSONObject>("arrval").asArray<int, std::list>();

		JSONObject arr = cjsonpp::arrayObject();
		arr.add("s1");
		arr.add("s2");
		JSONObject obj2;
		obj2.set("arrval", arr);
		std::cout << obj2 << std::endl;

		const cjsonpp::JSONObject arr3 = create_arr();
		const std::string json = arr3.print();
		std::cout << json << std::endl;


	} catch (const JSONError& e) {
		std::cout << e.what() << '\n';
      return -1;
	}

	const char* garbageJSON = "This is not valid JSON.";
	try {
      JSONObject obj = parse(garbageJSON);
      return -1;
   } catch (const JSONError&) {
		// no-op
   }
	return 0;
}
