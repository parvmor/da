#include <da/init/parser.h>
#include <da/process/process.h>
#include <da/util/statusor.h>
#include <unistd.h>

#include <climits>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace da {
namespace init {
namespace {

bool isNumeric(const std::string& str) {
  for (long unsigned int i = 0; i < str.length(); i++) {
    if (!std::isdigit(str[i])) {
      return false;
    }
  }
  return true;
}

util::StatusOr<std::vector<std::unique_ptr<process::Process>>>
parseMembershipFile(const char* file, int current_process_id, int messages) {
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
  // Read process ip and port.
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

  // Read localized dependencies.
  std::string line;
  std::string element;
  // Go to the next line.
  std::getline(membership, line);
  while (!membership.eof()) {
    int current_process = -1;
    std::getline(membership, line);
    std::stringstream linestream(line);
    while (!linestream.eof()) {
      // Read each element of the line split by a space.
      std::getline(linestream, element, ' ');
      // Ignore empty elements caused by potential additional spaces.
      if (element.length() == 0) {
        continue;
      }
      // Terminate if a non-numeric element is found.
      if (!isNumeric(element)) {
        membership.close();
        return util::Status(util::StatusCode::kInvalidArgument,
                            element + " is not a process id.");
      }
      // Set current_process if it is the first element of the line.
      if (current_process == -1) {
        current_process = std::stoi(element) - 1;
      }
      int dependency = std::stoi(element) - 1;
      // Terminate if the current process id is out of bounds.
      if (current_process < 0 || current_process >= no_of_processes) {
        membership.close();
        return util::Status(util::StatusCode::kOutOfRange,
                            "Process id: " + std::to_string(current_process) +
                                " is out of bounds.");
      }
      // Terminate if the dependency id is out of bounds
      if (dependency < 0 || dependency >= no_of_processes) {
        membership.close();
        return util::Status(
            util::StatusCode::kOutOfRange,
            "Process id: " + std::to_string(dependency) + " is out of bounds.");
      }
      // Add dependency to current process.
      processes[current_process]->addDependency(dependency);
    }
    if (current_process != -1) {
      processes[current_process]->finalizeDependencies();
    }
  }
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
