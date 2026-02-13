#ifndef ddwhouses_ddicli_h_
#define ddwhouses_ddicli_h_

#include "ddbase/dddef.h"
#include <vector>
#include <string>
#include <memory>

namespace NSP_DD {
class ddicli
{
public:
    static std::vector<std::unique_ptr<ddicli>> get_all_cli_tools();

    virtual ~ddicli() = default;

    virtual std::string help() = 0;
    // check if the cmds is for current cli tool.
    virtual bool is_cmd_for_current_idl(const std::vector<std::wstring>& cmds) = 0;
    virtual s32 run(const std::vector<std::wstring>& cmds) = 0;
};
}
#endif // ddwhouses_ddicli_h_