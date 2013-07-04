/*
  Copyright (c) 2012 Dmitry Pankratov

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
  Version 0.3.

  When compiled with recent gcc compiler with c++11 support the following features are enabled:
	* initializer list for array object construction
	* default template parameters for constructor, get<> and asArray<> functions
	* std::shared_ptr

  if C++11 support is not compiled the std::tr1::shared_ptr implementation is used.
  Feel free to replace it with boost::shared_ptr if needed (see below _SHARED_PTR_IMPL macro)

  Usage examples:
		// parse and get value
		JSONObject obj = cjsonpp::parse(jsonstr);

		// JSONObject can be copied over

		// get value of the named element
		try {
			std::cout << obj.get<int>("intval") << std::endl;
			std::cout << obj.get<JSONObject>("intval").as<int>() << std::endl;
		} catch (const JSONError& e) {
			std::cerr << e.what() << '\n';
		}

		// get array
		std::vector<double> arr1 = obj.get("elems").asArray<double>();
		std::list<std::string> arr2 = obj.get("strs").asArray<std::string, std::list>();

		...
		// construct object
		JSONObject obj;
		std::vector<int> v = {1, 2, 3, 4}; // c++11 only
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

  The following data types are supported with get<>("name") and as<>() functions:
	* int
	* int64_t
	* double
	* std::string
	* bool
	* JSONObject

  To add support for more data types add a template specialization for private static function as<>(cJSON*).
  Example:
	QString JSONObject::as<QString>(cJSON* obj)
	{
		return QString::fromStdString(as<std::string>(obj));
	}
*/

#ifndef CJSONPP_H
#define CJSONPP_H

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (__cplusplus == 201103L)
#define WITH_CPP11
#endif

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdexcept>
#include <string>
#include <set>
#include <ostream>
#include <vector>

#ifdef WITH_CPP11
#include <memory>
#include <initializer_list>
#define _SHARED_PTR_IMPL std::shared_ptr
#else
#include <tr1/memory>
#define _SHARED_PTR_IMPL std::tr1::shared_ptr
#endif

#include "cJSON.h"

namespace cjsonpp {

// JSON type wrapper enum
enum JSONType {
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
		Holder(cJSON* obj, bool own) : o(obj), own_(own) {}
		~Holder() { if (own_) cJSON_Delete(o); }

		inline cJSON* operator->()
		{
			return o;
		}
	private:
		// no copy constructor
		explicit Holder(const Holder&);

		// no assignment operator
		Holder& operator=(const Holder&);
	};

	typedef _SHARED_PTR_IMPL<Holder> HolderPtr;

	typedef std::set<JSONObject> ObjectSet;
	typedef _SHARED_PTR_IMPL<ObjectSet> ObjectSetPtr;

	// get value (specialized below)
	template <typename T>
	static T as(cJSON* obj);

	HolderPtr obj_;

	// Track added holders so that they are not destroyed prematurely before this object dies.
	// The idea behind it: when a value is added to the object or array it is stored as a reference
	//   in the cJSON structure and also added to the holder set.
	// Holders are stored in the shared set to make sure JSONObject copies will have it as well.
	// This is only relevant for object and array types.
	// Concurrency is not handled for performance reasons so it's better to avoid sharing JSONObjects
	//   across threads.
	ObjectSetPtr refs_;

public:
	inline cJSON* obj() const { return obj_->o; }

	std::string print(bool formatted=true) const
	{
		char* json = formatted ? cJSON_Print(obj_->o) : cJSON_PrintUnformatted(obj_->o);
		std::string retval(json);
		free(json);
		return retval;
	}

	// necessary for holding references in the set
	bool operator < (const JSONObject& other) const
	{
		return obj_->o < other.obj_->o;
	}

	// create empty object
	JSONObject()
		: obj_(new Holder(cJSON_CreateObject(), true)),
		  refs_(new ObjectSet)
	{
	}

	// non-virtual destructor (no subclassing intended)
	~JSONObject()
	{
	}

	// wrap existing cJSON object
	JSONObject(cJSON* obj, bool own)
		: obj_(new Holder(obj, own)),
		  refs_(new ObjectSet)
	{
	}

	// create boolean object
	explicit JSONObject(bool value)
		: obj_(new Holder(value ? cJSON_CreateTrue() : cJSON_CreateFalse(), true))
	{
	}

	// create double object
	explicit JSONObject(double value)
		: obj_(new Holder(cJSON_CreateNumber(value), true))
	{
	}

	// create integer object
	explicit JSONObject(int value)
		: obj_(new Holder(cJSON_CreateNumber(static_cast<double>(value)), true))
	{
	}

	// create integer object
	explicit JSONObject(int64_t value)
		: obj_(new Holder(cJSON_CreateNumber(static_cast<double>(value)), true))
	{
	}

	// create string object
	explicit JSONObject(const char* value)
		: obj_(new Holder(cJSON_CreateString(value), true))
	{
	}

	explicit JSONObject(const std::string& value)
		: obj_(new Holder(cJSON_CreateString(value.c_str()), true))
	{
	}

	// create array object
#ifdef WITH_CPP11
	template <typename T,
			  template<typename T, typename A> class ContT=std::vector>
#else
	template <typename T,
			  template<typename T, typename A> class ContT>
#endif
	explicit JSONObject(const ContT<T, std::allocator<T> >& elems)
		: obj_(new Holder(cJSON_CreateArray(), true)),
		  refs_(new ObjectSet)
	{
		for (typename ContT<T, std::allocator<T> >::const_iterator it = elems.begin();
			 it != elems.end(); it++)
			add(*it);
	}
#ifdef WITH_CPP11
	template <typename T>
	JSONObject(const std::initializer_list<T>& elems)
		: obj_(new Holder(cJSON_CreateArray(), true)),
		  refs_(new ObjectSet)
	{
		for (auto it = elems.begin(); it != elems.end(); it++)
			add(*it);
	}
#endif
	// for Qt-style containers
	template <typename T,
			  template<typename T> class ContT>
	explicit JSONObject(const ContT<T>& elems)
		: obj_(new Holder(cJSON_CreateArray(), true)),
		  refs_(new ObjectSet)
	{
		for (typename ContT<T>::const_iterator it = elems.begin(); it != elems.end(); it++)
			add(*it);
	}

	// copy constructor
	JSONObject(const JSONObject& other)
		: obj_(other.obj_), refs_(other.refs_)
	{
	}

	// copy operator
	inline JSONObject& operator=(const JSONObject& other)
	{
		if (&other != this) {
			obj_ = other.obj_;
			refs_ = other.refs_;
		}
		return *this;
	}

	// get object type
	inline JSONType type() const
	{
		static JSONType vmap[] = {
			Bool, Bool, Null, Number,
			String, Array, Object
		};
		return vmap[(*obj_)->type & 0xff];
	}

	// get value from this object
	template <typename T>
	inline T as() const
	{
		return as<T>(obj_->o);
	}

	// get array
#ifdef WITH_CPP11
	template <typename T=JSONObject,
			  template<typename T, typename A> class ContT=std::vector>
#else
	template <typename T, template<typename T, typename A> class ContT>
#endif
	inline ContT<T, std::allocator<T> > asArray() const
	{
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");

		ContT<T, std::allocator<T> > retval;
		for (int i = 0; i < cJSON_GetArraySize(obj_->o); i++)
			retval.push_back(as<T>(cJSON_GetArrayItem(obj_->o, i)));

		return retval;
	}

	// for Qt-style containers
	template <typename T, template<typename T> class ContT>
	inline ContT<T> asArray() const
	{
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");

		ContT<T> retval;
		for (int i = 0; i < cJSON_GetArraySize(obj_->o); i++)
			retval.push_back(as<T>(cJSON_GetArrayItem(obj_->o, i)));

		return retval;
	}

	// get object by name
#ifdef WITH_CPP11
	template <typename T=JSONObject>
#else
	template <typename T>
#endif
	inline T get(const char* name) const
	{
		if (((*obj_)->type & 0xff) != cJSON_Object)
			throw JSONError("Not an object");

		cJSON* item = cJSON_GetObjectItem(obj_->o, name);
		if (item != NULL)
			return as<T>(item);
		else
			throw JSONError("No such item");
	}

#ifdef WITH_CPP11
	template <typename T=JSONObject>
#else
	template <typename T>
#endif
	inline JSONObject get(const std::string& value) const
	{
		return get<T>(value.c_str());
	}

	// get value from array
#ifdef WITH_CPP11
	template <typename T=JSONObject>
#else
	template <typename T>
#endif
	inline T get(int index) const
	{
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");

		cJSON* item = cJSON_GetArrayItem(obj_->o, index);
		if (item != NULL)
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
		refs_->insert(o);
	}

	// set value in object
	template <typename T>
	inline void set(const char* name, const T& value) {
		if (((*obj_)->type & 0xff) != cJSON_Object)
			throw JSONError("Not an object type");
		JSONObject o(value);
		cJSON_AddItemReferenceToObject(obj_->o, name, o.obj_->o);
		refs_->insert(o);
	}

	// set value in object
	template <typename T>
	inline void set(const std::string& name, const T& value) {
		set(name.c_str(), value);
	}

	// set value in object (std::string)
	inline void set(const std::string& name, const JSONObject& value) {
		return set(name.c_str(), value);
	}

	// remove item from object
	inline void remove(const char* name) {
		if (((*obj_)->type & 0xff) != cJSON_Object)
			throw JSONError("Not an object type");
		cJSON* detached = cJSON_DetachItemFromObject(obj_->o, name);
		if (!detached)
			throw JSONError("No such item");
		for (ObjectSet::iterator it = refs_->begin(); it != refs_->end(); it++)
			if (it->obj_->o == detached) {
				refs_->erase(it);
				break;
			}
		cJSON_Delete(detached);
	}

	inline void remove(const std::string& name) {
		return remove(name.c_str());
	}

	// remove item from array
	inline void remove(int index) {
		if (((*obj_)->type & 0xff) != cJSON_Array)
			throw JSONError("Not an array type");
		cJSON* detached = cJSON_DetachItemFromArray(obj_->o, index);
		if (!detached)
			throw JSONError("No such item");
		for (ObjectSet::iterator it = refs_->begin(); it != refs_->end(); it++)
			if (it->obj_->o == detached) {
				refs_->erase(it);
				break;
			}
		cJSON_Delete(detached);
	}
};

// parse from C string
inline JSONObject parse(const char* str)
{
	cJSON* cjson = cJSON_Parse(str);
	if (cjson)
		return JSONObject(cjson, true);
	else
		throw JSONError("Parse error");
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
inline int JSONObject::as(cJSON* obj)
{
	if ((obj->type & 0xff) != cJSON_Number)
		throw JSONError("Bad value type");
	return obj->valueint;
}

template <>
inline int64_t JSONObject::as(cJSON* obj)
{
	if ((obj->type & 0xff) != cJSON_Number)
		throw JSONError("Not a number type");
	return (int64_t)obj->valuedouble;
}

template <>
inline std::string JSONObject::as(cJSON* obj)
{
	if ((obj->type & 0xff) != cJSON_String)
		throw JSONError("Not a string type");
	return obj->valuestring;
}

template <>
inline double JSONObject::as(cJSON* obj)
{
	if ((obj->type & 0xff) != cJSON_Number)
		throw JSONError("Not a number type");
	return obj->valuedouble;
}

template <>
inline bool JSONObject::as(cJSON* obj)
{
	if ((obj->type & 0xff) == cJSON_True)
		return true;
	else if ((obj->type & 0xff) == cJSON_False)
		return false;
	else
		throw JSONError("Not a boolean type");
}

template <>
inline JSONObject JSONObject::as(cJSON* obj)
{
	return JSONObject(obj, false);
}

// A traditional C++ streamer
inline std::ostream& operator<<(std::ostream& os, const cjsonpp::JSONObject& obj)
{
	os << obj.print();
	return os;
}

} // namespace cjsonpp

#endif
