#ifndef __INCLUDED_DA_INIT_PARSER_CC_H_
#define __INCLUDED_DA_INIT_PARSER_CC_H_

#include <vector>

#include <da/process/process.h>

namespace da {
namespace init {

std::vector<process::Process> parse(int argc, char** argv);

}  // init
}  // da

#endif  // __INCLUDED_DA_INIT_PARSER_CC_H_
