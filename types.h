#ifndef __TYPES_H_INCLUDED__
#define __TYPES_H_INCLUDED__

#include <filesystem>
#include <map>
#include <tuple>
#include <type_traits>
#include <vector>

using object_path_t = std::filesystem::path;

template <typename U, typename V> constexpr bool decay_same_v = std::is_same_v<std::decay_t<U>, V>;

template <typename T> struct is_output_type {
    static constexpr bool value
        = std::is_lvalue_reference_v<T> && !std::is_const_v<typename std::remove_reference_t<T>>;
};

template <typename T> inline constexpr bool is_output_type_v = is_output_type<T>::value;

// is_specialization_of<> lifted from the WG21 P2098R0 proposal document.
template <typename T, template <typename...> typename Primary> struct is_specialization_of : std::false_type {};

template <template <typename...> typename Primary, typename... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type {};

template <typename T, template <typename...> typename Primary>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Primary>::value;

template <typename T> inline constexpr bool is_tuple_v = is_specialization_of_v<std::decay_t<T>, std::tuple>;

template <typename T> inline constexpr bool is_vector_v = is_specialization_of_v<std::decay_t<T>, std::vector>;

template <typename T> inline constexpr bool is_map_v = is_specialization_of_v<std::decay_t<T>, std::map>;

#endif // __TYPES_H_INCLUDED__
