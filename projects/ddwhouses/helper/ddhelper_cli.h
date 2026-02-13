#ifndef ddwhouses_ddhelper_cli_h_
#define ddwhouses_ddhelper_cli_h_

#include "../ddicli.h"

namespace NSP_DD {
class ddhelper_cli: public ddicli
{
public:
    ~ddhelper_cli() override = default;

    std::string help() override;
    bool is_cmd_for_current_idl(const std::vector<std::wstring>& cmds) override;
    s32 run(const std::vector<std::wstring>& cmds) override;
};
}
#endif // ddwhouses_ddhelper_cli_h_