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










class CLASS_NAME
{
public:
  enum class Tag
  {
    #define GEN_TU_PART(name, nameCap, type) nameCap,
    FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)
    #undef GEN_TU_PART
    None,
  };
  CLASS_NAME() {}

  CLASS_NAME(const CLASS_NAME& other)
  {
    switch(other._tag)
    {
      #define GEN_TU_PART(name, nameCap, type) case Tag::nameCap: new (&vals. _ ## name) type(std::move(other.vals. _ ## name)); _tag = Tag::nameCap; break;
      FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)
      #undef GEN_TU_PART
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
      #define GEN_TU_PART(name, nameCap, type) case Tag::nameCap: new (&vals. _ ## name) type(std::move(other.vals. _ ## name)); _tag = Tag::nameCap; break;
      FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)
      #undef GEN_TU_PART
      case Tag::None: break;
    }

    return *this;
  }

  #define GEN_TU_PART(name, nameCap, type) CLASS_NAME(type name) : _tag(Tag::nameCap)  { new (&vals. _ ## name) type(std::move(name)); }
  FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)
  #undef GEN_TU_PART

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
      #define GEN_TU_PART(name, nameCap, type) case Tag::nameCap: destructIfNeeded(vals. _ ## name); break;
      FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)
      #undef GEN_TU_PART
      case Tag::None: break;
    }
  }
  Tag tag() const { return _tag; }
  operator bool() const { return _tag != Tag::None; }

  #define GEN_TU_PART(name, nameCap, type) bool is ## nameCap() const { return _tag == Tag::nameCap; }
  FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)
  #undef GEN_TU_PART

  #define GEN_TU_PART(name, nameCap, type) type& name() { release_assert(is ## nameCap()); return vals. _ ## name; } \
                                           type const & name() const { release_assert(is ## nameCap()); return vals. _ ## name; }
  FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)
  #undef GEN_TU_PART

  template<typename T>
  bool operator==(const T & other) const
  {
    static_assert(std::is_same<T, CLASS_NAME>::value);

    if (_tag != other._tag)
      return false;

    switch(_tag)
    {
      #define GEN_TU_PART(name, nameCap, type) case Tag::nameCap: return vals. _ ## name == other.vals. _ ## name; break;
      FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)
      #undef GEN_TU_PART
      case Tag::None: break;
    }

    message_and_abort("unreachable");
  }

  template<typename T>
  bool operator!=(const CLASS_NAME & other) const { return !(*this == other); }

  #define GEN_TU_PART(name, nameCap, type) if constexpr (std::is_same<T, type>::value) return name();
  template<typename T> T& get() { FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)  }
  template<typename T> const T& get() const { FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART) }
  #undef GEN_TU_PART

private:
  Tag _tag = Tag::None;

  union TaggedUnionVals
  {
    #define GEN_TU_PART(name, nameCap, type) type _ ## name;
    FOR_EACH_TAGGED_UNION_TYPE(GEN_TU_PART)
    #undef GEN_TU_PART
    TaggedUnionVals() {}
    ~TaggedUnionVals() {}
  } vals;
};


#undef FOR_EACH_TAGGED_UNION_TYPE
#undef CLASS_NAME