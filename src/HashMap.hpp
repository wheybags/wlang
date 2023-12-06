#include <string>
#include <unordered_map>

struct string_hash
{
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char* str) const        { return hash_type{}(str); }
    std::size_t operator()(std::string_view str) const   { return hash_type{}(str); }
    std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
};


// just an unordered_map<std::string, T> that allows lookup by std::string_view without creating a temporary std::string
template<typename T>
using HashMap = std::unordered_map<std::string, T, string_hash, std::equal_to<>>;
