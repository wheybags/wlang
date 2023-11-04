#pragma once

// genuinely not sure this is a good idea
// Usage example:
//
// class NameOrInt
// {
//   #define FOR_EACH_TAGGED_UNION_TYPE(XX) \
//   XX(name, Name, std::string) \
//   XX(int, Int, int)
//
//   #define CLASS_NAME NameOrInt
//     CREATE_TAGGED_UNION
//   #undef CLASS_NAME
//   #undef FOR_EACH_TAGGED_UNION_TYPE
// };
//
// This will get you an enum class NameOrInt::Tag with members Name, Int and None. tag() returns current type.
// To get the values you can call getName() or getInt(). To create, just assign from an int or string.


#include <type_traits>

template <typename T>
typename std::enable_if<std::is_destructible<T>::value>::type destructIfNeeded(T& obj) {
    obj.~T();
}

template <typename T>
typename std::enable_if<!std::is_destructible<T>::value>::type destructIfNeeded(T&) {
}


#define GEN_ENUM_MEMBERS(name, nameCap, type) nameCap,
#define GEN_CONSTRUCTORS(name, nameCap, type) CLASS_NAME(type name) : _tag(Tag::nameCap)  { new (& _ ## name) type(std::move(name)); }

#define GEN_COPY_CONSTRUCTOR(name, nameCap, type) case Tag::nameCap: new (& _ ## name) type(std::move(other._ ## name)); _tag = Tag::nameCap; break;

#define GEN_DESTRUCTORS(name, nameCap, type) case Tag::nameCap: destructIfNeeded(_ ## name); break;

#define GEN_IS_XX(name, nameCap, type) bool is ## nameCap() const { return _tag == Tag::nameCap; }
#define GEN_GETTERS(name, nameCap, type) type& name() { release_assert(is ## nameCap()); return _ ## name; } \
                                         type const & name() const { release_assert(is ## nameCap()); return _ ## name; }

#define GEN_UNION_MEMBERS(name, nameCap, type) type _ ## name;

#define CREATE_TAGGED_UNION \
  public: \
  enum class Tag \
  { \
    FOR_EACH_TAGGED_UNION_TYPE(GEN_ENUM_MEMBERS) \
    None, \
  }; \
  CLASS_NAME() {} \
 \
  CLASS_NAME(const CLASS_NAME& other) \
  { \
    switch(other._tag) \
    { \
      FOR_EACH_TAGGED_UNION_TYPE(GEN_COPY_CONSTRUCTOR) \
      case Tag::None: break; \
    } \
  } \
 \
  CLASS_NAME& operator=(const CLASS_NAME& other) \
  { \
    if (&other == this) \
      return *this; \
 \
    this->~CLASS_NAME(); \
    switch(other._tag) \
    { \
      FOR_EACH_TAGGED_UNION_TYPE(GEN_COPY_CONSTRUCTOR) \
      case Tag::None: break; \
    } \
    return *this; \
  } \
 \
  FOR_EACH_TAGGED_UNION_TYPE(GEN_CONSTRUCTORS) \
  ~CLASS_NAME() \
  { \
    switch(_tag) \
    { \
      FOR_EACH_TAGGED_UNION_TYPE(GEN_DESTRUCTORS) \
      case Tag::None: break; \
    } \
  } \
  Tag tag() const { return _tag; } \
  operator bool() const { return _tag != Tag::None; } \
 \
  FOR_EACH_TAGGED_UNION_TYPE(GEN_IS_XX) \
  FOR_EACH_TAGGED_UNION_TYPE(GEN_GETTERS) \
 \
  private: \
  Tag _tag = Tag::None; \
 \
  union \
  { \
    FOR_EACH_TAGGED_UNION_TYPE(GEN_UNION_MEMBERS) \
  };