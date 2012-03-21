// This code is public domain software

/*
  Type-safe thin C++ wrapper over cJSON library (header-only).
  Version 0.1.0.

  Requires C++11 compiler with lambdas, move constructors, enum classes, std::shared_ptr.
  Tested with gcc 4.6

  Usage examples:
		// parse and get value
		JSONObject obj = cjsonpp::parse(jsonstr);
		// the following two lines are equal
		std::cout << obj.get<int>("intval") << std::endl;
		std::cout << obj.get<JSONObject>("intval").as<int>() << std::endl;

		// get array
		std::vector<double> arr1 = obj.get("elems").asArray<double>();
		std::list<std::string> arr2 = obj.get("strs").asArray<std::string, std::list>();

		...
		// construct object
		JSONObject obj;
		std::vector<int> v = {1, 2, 3, 4};
		obj.set("intval", 1234);
		obj.set("arrval", v);
		obj.set("doubleval", 100.1);
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
#include <set>
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
		explicit Holder(const Holder&) = delete;

		// no assignment operator
		Holder& operator=(const Holder&) = delete;

		inline cJSON* operator->()
		{
			return o;
		}
	};
	typedef std::shared_ptr<Holder> HolderPtr;

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
	template <typename T,
			  template<typename T, typename A> class ContT=std::vector,
			  template<typename T> class AllocT=std::allocator>
	explicit JSONObject(const ContT<T, AllocT<T>>& elems)
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

	// get value from this object
	template <typename T>
	T as() const
	{
		return as<T>(obj_->o);
	}

	// get array
	template <typename T,
			  template<typename T, typename A> class ContT=std::vector,
			  template<typename T> class AllocT=std::allocator>
	ContT<T, AllocT<T>> asArray() const
	{
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");

		ContT<T, AllocT<T>> retval;
		for (int i = 0; i < cJSON_GetArraySize(obj_->o); i++)
			retval.push_back(as<T>(cJSON_GetArrayItem(obj_->o, i)));

		return retval;
	}

	// get object by name
	template <typename T=JSONObject>
	inline T get(const char* name) const
	{
		if (((*obj_)->type & 0xff) != cJSON_Object)
			throw JSONError("Not an object");

		cJSON* item = cJSON_GetObjectItem(obj_->o, name);
		if (item != nullptr)
			return as<T>(item);
		else
			throw JSONError("No such item");
	}

	template <typename T=JSONObject>
	inline JSONObject get(const std::string& value) const
	{
		return get<T>(value.c_str());
	}

	// get value from array
	template <typename T=JSONObject>
	inline T get(int index) const
	{
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");

		cJSON* item = cJSON_GetArrayItem(obj_->o, index);
		if (item != nullptr)
			return as<T>(item);
		else
			throw JSONError("No such item");
	}

	// add value to array
	template <typename T>
	inline void add(const T& value)
	{
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");
		JSONObject o(value);
		cJSON_AddItemReferenceToArray(obj_->o, o.obj_->o);
		refs_.insert(o.obj_);
	}

	// set value in object
	template <typename T>
	inline void set(const char* name, const T& value) {
		if (((*obj_)->type & 0xff) != cJSON_Object)
			throw JSONError("Not an object type");
		JSONObject o(value);
		cJSON_AddItemReferenceToObject(obj_->o, name, o.obj_->o);
		refs_.insert(o.obj_);
	}

	// set value in object (std::string)
	inline void set(const std::string& name, const JSONObject& value) {
		return set(name.c_str(), value);
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
	// get value (specialized below)
	template <typename T>
	T as(cJSON* obj) const;

	HolderPtr obj_;

	// track added holders so that they are not destroyed prematurely before this object dies
	std::set<HolderPtr> refs_;

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

// Specialized getters
template <>
inline int JSONObject::as(cJSON* obj) const
{
	if ((obj->type & 0xff) != cJSON_Number)
		throw JSONError("Bad value type");
	return obj->valueint;
}

template <>
inline long JSONObject::as(cJSON* obj) const
{
	if ((obj->type & 0xff) != cJSON_Number)
		throw JSONError("Not a number type");
	return (long)obj->valuedouble;
}

template <>
inline std::string JSONObject::as(cJSON* obj) const
{
	if ((obj->type & 0xff) != cJSON_String)
		throw JSONError("Not a string type");
	return obj->valuestring;
}

template <>
inline double JSONObject::as(cJSON* obj) const
{
	if ((obj->type & 0xff) != cJSON_Number)
		throw JSONError("Not a number type");
	return obj->valuedouble;
}

template <>
inline bool JSONObject::as(cJSON* obj) const
{
	if ((obj->type & 0xff) == cJSON_True)
		return true;
	else if ((obj->type & 0xff) == cJSON_False)
		return false;
	else
		throw JSONError("Not a boolean type");
}

template <>
inline JSONObject JSONObject::as(cJSON* obj) const
{
	return JSONObject(obj);
}

// A traditional C++ streamer
inline std::ostream& operator<<(std::ostream& os, const cjsonpp::JSONObject& obj)
{
	os << obj.print();
	return os;
}

} // namespace cjsonpp

#endif
