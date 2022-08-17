#pragma once

#include <algorithm>
#include <string>

namespace HaveFunCompiler {

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