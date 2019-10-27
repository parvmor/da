#ifndef __INCLUDED_DA_UTIL_IDENTITY_MANAGER_H_
#define __INCLUDED_DA_UTIL_IDENTITY_MANAGER_H_

#include <mutex>
#include <unordered_map>
#include <vector>

namespace da {
namespace util {

template <typename T>
class IdentityManager {
 public:
  int assignId(const T& t);

  const T* getValue(int id);

  int getId(const T& t);

 private:
  std::shared_timed_mutex mutex_;
  std::unordered_map<T, int> type_to_id_;
  std::vector<const T*> id_to_type_;
};

template <typename T>
int IdentityManager<T>::assignId(const T& t) {
  std::unique_lock<std::shared_timed_mutex> lock(mutex_);
  if (type_to_id_.find(t) != type_to_id_.end()) {
    return type_to_id_[t];
  }
  int id = id_to_type_.size();
  type_to_id_[t] = id;
  const T* t_ptr = &(type_to_id_.find(t)->first);
  id_to_type_.push_back(t_ptr);
  return id;
}

template <typename T>
const T* IdentityManager<T>::getValue(int id) {
  std::shared_lock<std::shared_timed_mutex> lock(mutex_);
  if (id >= int(id_to_type_.size())) {
    return nullptr;
  }
  return id_to_type_[id];
}

template <typename T>
int IdentityManager<T>::getId(const T& t) {
  std::shared_lock<std::shared_timed_mutex> lock(mutex_);
  if (type_to_id_.find(t) == type_to_id_.end()) {
    return -1;
  }
  return type_to_id_[t];
}

}  // namespace util
}  // namespace da

#endif  // __INCLUDED_DA_UTIL_IDENTITY_MANAGER_H_
