#ifndef ddwhouses_ddhelper_idl_h_
#define ddwhouses_ddhelper_idl_h_

#include "ddicli.h"

namespace NSP_DD {
class ddmain_cli : public ddicli
{
public:
    ~ddmain_cli() override = default;

    std::string help() override;
    bool is_cmd_for_current_idl(const std::vector<std::wstring>& cmds) override;
    s32 run(const std::vector<std::wstring>& cmds) override;

private:
    bool run_in_helper_process(const std::wstring& target, const std::wstring& hepler_exe_path);
};
}
#endif // ddwhouses_ddhelper_idl_h_