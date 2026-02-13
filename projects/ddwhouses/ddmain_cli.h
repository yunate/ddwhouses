#ifndef ddwhouses_ddhelper_idl_h_
#define ddwhouses_ddhelper_idl_h_

#include "ddicli.h"
#include <windows.h>

namespace NSP_DD {
class ddmain_cli : public ddicli
{
public:
    ~ddmain_cli() override = default;

    std::string help() override;
    bool is_cmd_for_current_cli(const std::vector<std::wstring>& cmds) override;
    s32 run(const std::vector<std::wstring>& cmds) override;

private:
    struct printable_process_info
    {
        std::wstring name = L"-";
        std::wstring user = L"-";
        std::wstring session = L"-";
        std::wstring started = L"-";
        std::wstring executable = L"-";
        std::wstring command_line = L"-";
    };

    static std::wstring get_process_user(HANDLE process);
    static std::wstring get_process_started(HANDLE process);
    static std::wstring get_process_command_line(HANDLE process);
    static bool query_process_info(DWORD process_id, printable_process_info& info);
    static void print_process_info(
        const std::wstring& file_name,
        DWORD process_id,
        const printable_process_info& info);

    bool run_in_helper_process(const std::wstring& target, const std::wstring& hepler_exe_path);
};
}
#endif // ddwhouses_ddhelper_idl_h_