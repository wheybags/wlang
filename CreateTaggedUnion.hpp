// genuinely not sure this is a good idea
// Usage example:
//
// #define FOR_EACH_TAGGED_UNION_TYPE(XX) \
//   XX(name, Name, std::string) \
//   XX(int, Int, int32_t)
// #define CLASS_NAME NameOrInt
// #include "CreateTaggedUnion.hpp"
//
// This will get you a class NameOrInt with a Tag enum with members Name, Int and None. tag() returns current type.
// Values are stored in a union. To get the values you can call getName() or getInt().
// To create, just assign or construct from an int or string.


#define GEN_ENUM_MEMBERS(name, nameCap, type) nameCap,

#ifndef GEN_CONSTRUCTORS
  #define GEN_CONSTRUCTORS(name, nameCap, type) CLASS_NAME(type name) : _tag(Tag::nameCap)  { new (&vals. _ ## name) type(std::move(name)); }
#endif

#define GEN_COPY_CONSTRUCTOR(name, nameCap, type) case Tag::nameCap: new (&vals. _ ## name) type(std::move(other.vals. _ ## name)); _tag = Tag::nameCap; break;

#define GEN_DESTRUCTORS(name, nameCap, type) case Tag::nameCap: destructIfNeeded(vals. _ ## name); break;

#define GEN_IS_XX(name, nameCap, type) bool is ## nameCap() const { return _tag == Tag::nameCap; }
#define GEN_GETTERS(name, nameCap, type) type& name() { release_assert(is ## nameCap()); return vals. _ ## name; } \
                                         type const & name() const { release_assert(is ## nameCap()); return vals. _ ## name; }

#define GEN_UNION_MEMBERS(name, nameCap, type) type _ ## name;

#define GEN_TEMPLATE_GETTER(name, nameCap, type) if constexpr (std::is_same<T, type>::value) return name();

class CLASS_NAME
{
public:
  enum class Tag
  {
    FOR_EACH_TAGGED_UNION_TYPE(GEN_ENUM_MEMBERS)
    None,
  };
  CLASS_NAME() {}

  CLASS_NAME(const CLASS_NAME& other)
  {
    switch(other._tag)
    {
      FOR_EACH_TAGGED_UNION_TYPE(GEN_COPY_CONSTRUCTOR)
      case Tag::None: break;
    }
  }

  CLASS_NAME& operator=(const CLASS_NAME& other)
  {
    if (&other == this)
      return *this;

    this->~CLASS_NAME();
    switch(other._tag)
    {
      FOR_EACH_TAGGED_UNION_TYPE(GEN_COPY_CONSTRUCTOR)
      case Tag::None: break;
    }
    return *this;
  }

  FOR_EACH_TAGGED_UNION_TYPE(GEN_CONSTRUCTORS)

  template <typename T>
  typename std::enable_if<std::is_destructible<T>::value>::type destructIfNeeded(T& obj)
  {
      obj.~T();
  }

  template <typename T>
  typename std::enable_if<!std::is_destructible<T>::value>::type destructIfNeeded(T&) { }

  ~CLASS_NAME()
  {
    switch(_tag)
    {
      FOR_EACH_TAGGED_UNION_TYPE(GEN_DESTRUCTORS)
      case Tag::None: break;
    }
  }
  Tag tag() const { return _tag; }
  operator bool() const { return _tag != Tag::None; }

  FOR_EACH_TAGGED_UNION_TYPE(GEN_IS_XX)
  FOR_EACH_TAGGED_UNION_TYPE(GEN_GETTERS)

  template<typename T>
  T& get()
  {
    FOR_EACH_TAGGED_UNION_TYPE(GEN_TEMPLATE_GETTER)
  }

  template<typename T>
  const T& get() const
  {
    FOR_EACH_TAGGED_UNION_TYPE(GEN_TEMPLATE_GETTER)
  }

private:
  Tag _tag = Tag::None;

  union TaggedUnionVals
  {
    FOR_EACH_TAGGED_UNION_TYPE(GEN_UNION_MEMBERS)
    TaggedUnionVals() {}
    ~TaggedUnionVals() {}
  } vals;
};


#undef GEN_ENUM_MEMBERS
#undef GEN_CONSTRUCTORS
#undef GEN_COPY_CONSTRUCTOR
#undef GEN_DESTRUCTORS
#undef GEN_IS_XX
#undef GEN_GETTERS
#undef GEN_UNION_MEMBERS
#undef CLASS_NAME
#undef FOR_EACH_TAGGED_UNION_TYPE