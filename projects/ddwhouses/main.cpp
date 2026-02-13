#include "ddwhouses/stdafx.h"
#include "ddbase/dddef.h"
#include "ddbase/ddassert.h"
#include "ddbase/windows/ddprocess.h"
#include "ddbase/ddcmd_line_utils.h"
#include "ddbase/ddlocale.h"
#include "ddbase/ddio.h"
#include "ddbase/str/ddstr.h"
#include "ddbase/ddtime.h"
#include "ddbase/windows/ddmoudle_utils.h"

#include "ddicli.h"

#include <filesystem>
#include <process.h>


#pragma comment(lib, "ddbase.lib")

namespace NSP_DD {

int ddmain()
{
    ddtimer timer;
    (void)ddlocale::set_utf8_locale_and_io_codepage();
    std::filesystem::path path(ddmoudle_utils::get_moudle_pathW());
    ::SetCurrentDirectoryW(path.parent_path().c_str());

    s32 exit_code = -1;
    std::vector<std::wstring> cmds;
    ddcmd_line_utils::get_cmds(cmds);
    auto all_tools = ddicli::get_all_cli_tools();
    for (auto& it : all_tools) {
        if (!it->is_cmd_for_current_idl(cmds)) {
            continue;
        }

        exit_code = it->run(cmds);
        if (exit_code != 0) {
            ddcout(ddconsole_color::red) << ddstr::format("Execute cmd failure, exit_code: %d\n", exit_code);
            ddcout(ddconsole_color::gray) << it->help();
        }
        break;
    }
    ddcout(ddconsole_color::cyan) << ddstr::format("exec cost:%dms \n", timer.get_time_pass() / 1000000);
    return exit_code;
}
} // namespace NSP_DD

void main()
{
    // ::_CrtSetBreakAlloc(918);
    int result = NSP_DD::ddmain();

#ifdef _DEBUG
    _cexit();
    DDASSERT_FMT(!::_CrtDumpMemoryLeaks(), L"Memory leak!!! Check the output to find the log.");
    ::_exit(result);
#else
    ::exit(result);
#endif
}