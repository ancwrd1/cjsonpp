#include "cjsonpp.h"

namespace cjsonpp {

// A traditional C++ streamer
std::ostream& operator<<(std::ostream& os, const cjsonpp::JSONObject& obj)
{
	os << obj.print();
	return os;
}

// Specialized getters
template <>
int JSONObject::as() const
{
	if (((*obj_)->type & 0xff) != cJSON_Number)
		throw JSONError("Bad value type");
	return (*obj_)->valueint;
}

template <>
long JSONObject::as() const
{
	if (((*obj_)->type & 0xff) != cJSON_Number)
		throw JSONError("Not a number type");
	return (long)(*obj_)->valuedouble;
}

template <>
std::string JSONObject::as() const
{
	if (((*obj_)->type & 0xff) != cJSON_String)
		throw JSONError("Not a string type");
	return (*obj_)->valuestring;
}

template <>
double JSONObject::as() const
{
	if (((*obj_)->type & 0xff) != cJSON_Number)
		throw JSONError("Not a number type");
	return (*obj_)->valuedouble;
}

template <>
bool JSONObject::as() const
{
	if (((*obj_)->type & 0xff) == cJSON_True)
		return true;
	else if (((*obj_)->type & 0xff) == cJSON_False)
		return false;
	else
		throw JSONError("Not a boolean type");
}

template <>
JSONObject JSONObject::as() const
{
	return *this;
}

}
