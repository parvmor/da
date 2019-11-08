#include <da/init/parser.h>

#include <unistd.h>
#include <climits>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <da/process/process.h>
#include <da/util/statusor.h>

namespace da {
namespace init {
namespace {

util::StatusOr<std::vector<std::unique_ptr<process::Process>>>
parseMembershipFile(const char* file, uint16_t current_process_id,
                    int messages) {
  std::ifstream membership(file);
  if (!membership.good()) {
    membership.close();
    return util::Status(
        util::StatusCode::kNotFound,
        "Unable to open file: " + std::string(file) + " in read mode.");
  }
  int no_of_processes;
  membership >> no_of_processes;
  if (no_of_processes <= 0 || no_of_processes > UINT16_MAX) {
    membership.close();
    return util::Status(
        util::StatusCode::kOutOfRange,
        "Number of processes: " + std::to_string(no_of_processes) +
            " is out of bounds.");
  }
  std::vector<std::unique_ptr<process::Process>> processes(no_of_processes);
  for (int i = 0; i < no_of_processes; i++) {
    if (!membership.good()) {
      membership.close();
      return util::Status(
          util::StatusCode::kUnavailable,
          "Unable to read more input from file: " + std::string(file) + ".");
    }
    int process_id, port;
    std::string ip_addr;
    membership >> process_id >> ip_addr >> port;
    // Make the process id's zero indexed.
    process_id -= 1;
    if (process_id < 0 || process_id >= no_of_processes) {
      membership.close();
      return util::Status(
          util::StatusCode::kOutOfRange,
          "Process id: " + std::to_string(process_id) + " is out of bounds.");
    }
    processes[process_id] = std::make_unique<process::Process>(
        process_id, ip_addr, port, messages, current_process_id == process_id);
  }
  // TODO: Read extra parameters for Localized Causal Broadcast here.
  membership.close();
  return processes;
}

}  // namespace

util::StatusOr<std::vector<std::unique_ptr<process::Process>>> parse(
    int argc, char** argv) {
  if (argc <= 2 || argc > 4) {
    return util::Status(util::StatusCode::kInvalidArgument,
                        "Exactly 3 command line arguments expected.");
  }
  int messages = INT_MAX;
  if (argc == 4) {
    messages = atoi(argv[3]);
  }
  // TODO: Validate the input before doing the conversion.
  return parseMembershipFile(argv[2], atoi(argv[1]) - 1, messages);
}

}  // namespace init
}  // namespace da
