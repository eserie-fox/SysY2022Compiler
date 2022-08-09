#include <algorithm>

template <typename ContainerType>
bool Contains(ContainerType container, typename ContainerType::value_type value) {
  return (std::find(container.begin(), container.end(), value) != container.end());
}