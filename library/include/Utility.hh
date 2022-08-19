#pragma once

#include <algorithm>
#include <string>
#include <ostream>
#include <utility>

namespace std {

template <typename KeyType, typename ValueType>
struct hash<pair<KeyType, ValueType>> {
  using hash_type = pair<KeyType, ValueType>;
  size_t operator()(const hash_type &pr) const {
    size_t val1 = hash<KeyType>()(pr.first);
    size_t val2 = hash<ValueType>()(pr.second);
    return ((val1 << 32) + (val1 << 14) + (val1 << 2)) ^ val2;
  }
};

}  // namespace std

namespace HaveFunCompiler {

template <typename KeyType, typename ValueType>
std::ostream &operator<<(std::ostream &os, const std::pair<KeyType, ValueType> pr) {
  os << "( " << pr.first << ", " << pr.second << " )";
  return os;
}

template <typename ContainerType>
inline bool Contains(const ContainerType &container, const typename ContainerType::value_type &value) {
  return (std::find(container.begin(), container.end(), value) != container.end());
}

inline bool Contains(const std::string &str, const std::string &substr) { return (str.find(substr) != str.npos); }

template <typename ContainerType>
inline void Erase(ContainerType &container, const typename ContainerType::value_type &value) {
  container.erase(std::remove(std::begin(container), std::end(container), value), std::end(container));
}

template <typename ContainerType,typename PredictionType>
inline void EraseIf(ContainerType &container, PredictionType pred) {
  container.erase(std::remove_if(std::begin(container), std::end(container), pred), std::end(container));
}

}  // namespace HaveFunCompiler