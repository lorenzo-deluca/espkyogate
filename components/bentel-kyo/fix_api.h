#include "esphome/components/api/api_pb2.h"
#include "esphome/components/api/user_services.h"

namespace esphome {
namespace api {

template<> enums::ServiceArgType to_service_arg_type<int>() {
  return enums::ServiceArgType::SERVICE_ARG_TYPE_INT;
}

template<> int get_execute_arg_value<int>(const ExecuteServiceArgument &arg) {
  return arg.int_;
}

}  // namespace api
}  // namespace esphome