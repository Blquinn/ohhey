#pragma once

#include <functional>
#include <string>
#include <vector>

/**
 * Cli contains resources that are shared by all the cli sub-commands.
 */
namespace Cli {

static std::string CMD_COMPARE = "compare";

typedef std::function<void(std::vector<std::string> &)> BaseCommand;

} // namespace Cli
