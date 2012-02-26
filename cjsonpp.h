/*
  Copyright (c) 2011 Dmitry Pankratov <dmitry@pankratov.net>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/*
  Type-safe thin C++ wrapper over cJSON library (header-only).
  Version 0.9.0.

  Requires C++11 compiler with lambdas, move constructors, enum classes, std::shared_ptr.
  Tested with G++ 4.6

  Usage examples:
		// parse and get value
		JSONObject obj = cjsonpp::parse(jsonstr);
		// the following two lines are equal
		std::cout << obj.get("intval").as<int>() << std::endl;
		std::cout << obj.as<int>("intval") << std::endl;

		// get array
		std::vector<double> arr = obj.get("elems").asArray<double>();

		...
		// construct object
		JSONObject obj;
		std::vector<int> v = {1, 2, 3, 4};
		obj.set("intval", 1234);
		obj.set("arrval", v);
		obj.set("doubleval", JSONObject(100.1));
		obj.set("nullval", cjsonpp::nullObject());

		...
		// another way of constructing array
		JSONObject arr = cjsonpp::arrayObject();
		arr.add("s1");
		arr.add("s2");
		JSONObject obj;
		obj.set("arrval", arr);
		std::cout << obj << std::endl;
*/

#ifndef CJSONPP_H
#define CJSONPP_H

#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include <ostream>
#include <algorithm>

#include "cJSON.h"

namespace cjsonpp {

// JSON type wrapper enum
enum class JSONType {
	Bool,
	Null,
	String,
	Number,
	Array,
	Object
};

// Exception thrown in case of parse or value errors
class JSONError: public std::runtime_error
{
public:
	explicit JSONError(const char* what)
		: std::runtime_error(what)
	{
	}
};

// JSONObject class is a thin wrapper over cJSON data type
class JSONObject
{
	// internal cJSON holder with ownership flag
	struct Holder {
		cJSON* o;
		bool own_;
		explicit Holder(cJSON* obj, bool own=true) : o(obj), own_(own) {}
		~Holder() { if (own_) cJSON_Delete(o); }

		// no copy constructor
		explicit Holder(const Holder&other) = delete;

		// no assignment operator
		Holder& operator=(const Holder&) = delete;

		inline cJSON* operator->()
		{
			return o;
		}
	};

public:
	// create empty object
	JSONObject()
		: obj_(new Holder(cJSON_CreateObject()))
	{
	}

	// non-virtual destructor (no subclassing intended)
	~JSONObject()
	{
	}

	// wrap existing cJSON object
	explicit JSONObject(cJSON* obj, bool own=false)
		: obj_(new Holder(obj, own))
	{
	}

	// create boolean object
	explicit JSONObject(bool value)
		: obj_(new Holder(value ? cJSON_CreateTrue() : cJSON_CreateFalse()))
	{
	}

	// create double object
	explicit JSONObject(double value)
		: obj_(new Holder(cJSON_CreateNumber(value)))
	{
	}

	// create integer object
	explicit JSONObject(int value)
		: obj_(new Holder(cJSON_CreateNumber(value)))
	{
	}

	// create integer object
	explicit JSONObject(long value)
		: obj_(new Holder(cJSON_CreateNumber(value)))
	{
	}

	// create string object
	explicit JSONObject(const char* value)
		: obj_(new Holder(cJSON_CreateString(value)))
	{
	}

	explicit JSONObject(const std::string& value)
		: obj_(new Holder(cJSON_CreateString(value.c_str())))
	{
	}

	// create array object
	template <typename T>
	explicit JSONObject(const std::vector<T>& elems)
		: obj_(new Holder(cJSON_CreateArray()))
	{
		std::for_each(elems.cbegin(), elems.cend(),
					  [this](const T& e) { add(e); });
	}

	// copy constructor
	JSONObject(const JSONObject& other)
		: obj_(other.obj_)
	{
	}

	// move constructor
	JSONObject(JSONObject&& other)
		: obj_(other.obj_)
	{
		other.obj_.reset();
	}

	// copy operator
	inline JSONObject& operator=(const JSONObject& other)
	{
		if (&other != this)
			obj_ = other.obj_;
		return *this;
	}

	// move operator
	inline JSONObject& operator=(JSONObject&& other)
	{
		if (&other != this) {
			obj_ = other.obj_;
			other.obj_.reset();
		}

		return *this;
	}

	// get object type
	inline JSONType type() const
	{
		static JSONType vmap[] = {
			JSONType::Bool, JSONType::Bool, JSONType::Null, JSONType::Number,
			JSONType::String, JSONType::Array, JSONType::Object
		};
		return vmap[(*obj_)->type & 0xff];
	}

	// get value (specialized below)
	template <typename T>
	T as() const;

	// get value of the element
	template <typename T, typename P>
	T as(const P& param) const
	{
		return get(param).as<T>();
	}

	// get array
	template <typename T>
	std::vector<T> asArray() const
	{
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");

		std::vector<T> retval(cJSON_GetArraySize(obj_->o));
		for (size_t i = 0; i < retval.size(); i++)
			retval[i] = JSONObject(cJSON_GetArrayItem(obj_->o, i), false).as<T>();

		return retval;
	}

	// get object by name
	inline JSONObject get(const char* name) const
	{
		if (((*obj_)->type & 0xff) != cJSON_Object)
			throw JSONError("Not an object");

		cJSON* item = cJSON_GetObjectItem(obj_->o, name);
		if (item != nullptr)
			return JSONObject(item);
		else
			throw JSONError("No such item");
	}

	inline JSONObject get(const std::string& value) const
	{
		return get(value.c_str());
	}

	// get value from array
	inline JSONObject get(int index) const
	{
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");

		cJSON* item = cJSON_GetArrayItem(obj_->o, index);
		if (item != nullptr)
			return JSONObject(item);
		else
			throw JSONError("No such item");
	}

	// add value to array
	inline void add(const JSONObject& value)
	{
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");
		cJSON_AddItemReferenceToArray(obj_->o, value.obj());
		refs_.push_back(value);
	}

	// set value in object
	inline void set(const char* name, const JSONObject& value) {
		if (((*obj_)->type & 0xff) != cJSON_Object)
			throw JSONError("Not an object type");
		cJSON_AddItemReferenceToObject(obj_->o, name, value.obj());
		refs_.push_back(value);
	}

	// set value in object (std::string)
	inline void set(const std::string& name, const JSONObject& value) {
		return set(name.c_str(), value);
	}

	// template versions
	template <typename T>
	inline void add(const T& value)
	{
		return add(JSONObject(value));
	}

	template <typename P, typename T>
	inline void set(const P& param, const T& value) {
		return set(param, JSONObject(value));
	}

	cJSON* obj() const { return obj_->o; }

	std::string print() const
	{
		char* json = cJSON_Print(obj_->o);
		std::string retval(json);
		free(json);
		return retval;
	}

	std::string printUnformatted() const
	{
		char* json = cJSON_PrintUnformatted(obj_->o);
		std::string retval(json);
		free(json);
		return retval;
	}

private:
	std::shared_ptr<Holder> obj_;
	std::vector<JSONObject> refs_;

	friend JSONObject parse(const char* str);
	friend JSONObject nullObject();
	friend JSONObject arrayObject();
};

// parse from C string
inline JSONObject parse(const char* str)
{
	return JSONObject(cJSON_Parse(str), true);
}

// parse from std::string
inline JSONObject parse(const std::string& str)
{
	return parse(str.c_str());
}

// create null object
inline JSONObject nullObject()
{
	return JSONObject(cJSON_CreateNull(), true);
}

// create empty array object
inline JSONObject arrayObject()
{
	return JSONObject(cJSON_CreateArray(), true);
}

// A traditional C++ streamer
std::ostream& operator<<(std::ostream& os, const cjsonpp::JSONObject& obj);

} // namespace cjsonpp

#endif
