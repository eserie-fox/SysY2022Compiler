#pragma once

#include <algorithm>
#include <string>

namespace HaveFunCompiler {

template <typename ContainerType>
inline bool Contains(const ContainerType &container, const typename ContainerType::value_type &value) {
  return (std::find(container.begin(), container.end(), value) != container.end());
}

inline bool Contains(const std::string &str, const std::string &substr) { return (str.find(substr) != str.npos); }

}  // namespace HaveFunCompiler