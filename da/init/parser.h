#ifndef __INCLUDED_DA_INIT_PARSER_H_
#define __INCLUDED_DA_INIT_PARSER_H_

#include <da/process/process.h>
#include <da/util/statusor.h>

#include <memory>
#include <vector>

namespace da {
namespace init {

util::StatusOr<std::vector<std::unique_ptr<process::Process>>> parse(
    int argc, char** argv);

}  // namespace init
}  // namespace da

#endif  // __INCLUDED_DA_INIT_PARSER_H_
