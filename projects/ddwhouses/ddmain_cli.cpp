#include "ddwhouses/stdafx.h"

#include "ddmain_cli.h"
#include "ddbase/ddio.h"
#include "ddbase/str/ddstr.h"
#include "ddbase/ddexec_guard.hpp"
#include "ddbase/file/ddfile.h"
#include "ddbase/file/dddir.h"
#include "ddbase/file/ddpath.h"
#include "ddbase/ddrandom.h"
#include "ddbase/windows/ddprocess.h"
#include "ddbase/windows/ddmoudle_utils.h"

#include <set>

namespace NSP_DD {
enum dderror_code {
    success = 0,
    error
};

std::string ddmain_cli::help()
{
    std::string helper_str;
    helper_str += ddstr::format("ddwhouses.exe <target_file_path> \n");
    helper_str += ddstr::format("* represents zoro or more arbitrary characters; '?' represents one character (in Unicode, Chinese characters count as two characters)\n");
    return helper_str;
}

bool ddmain_cli::is_cmd_for_current_idl(const std::vector<std::wstring>& cmds)
{
    if (cmds.size() < 2) {
        return false;
    }

    return true;
}

s32 ddmain_cli::run(const std::vector<std::wstring>& cmds)
{
    std::wstring target = cmds[1];
    std::wstring exe_path = ddmoudle_utils::get_moudle_pathW();

    s32 try_times = 3;
    for (s32 i = 0; i < try_times; ++i) {
        if (run_in_helper_process(target, exe_path)) {
            return dderror_code::success;
        }
    }
    ddcout(ddconsole_color::red) << ddstr::format("Error after %d try!!!\n", try_times);
    return dderror_code::error;
}

bool ddmain_cli::run_in_helper_process(const std::wstring& target, const std::wstring& hepler_exe_path)
{
    std::wstring out_file_dir_path = L"";
    wchar_t temp_path[MAX_PATH];
    DWORD result = GetTempPathW(MAX_PATH, temp_path);
    if (result > 0 && result < MAX_PATH) {
        out_file_dir_path = temp_path;
    } else {
        out_file_dir_path = ddpath::parent(hepler_exe_path);
    }
    out_file_dir_path = ddpath::join({ out_file_dir_path, L"ddwhouses"});
    if (dddir::is_path_exist(out_file_dir_path)) {
        (void)dddir::delete_path(out_file_dir_path);
    }

    if (!dddir::create_dir_ex(out_file_dir_path)) {
        return false;
    }

    ddexec_guard guard([&out_file_dir_path]() {
        if (dddir::is_path_exist(out_file_dir_path)) {
            (void)dddir::delete_path(out_file_dir_path);
        }
    });

    std::wstring guid;
    if (!ddguid::generate_guid(guid)) {
        guid = L"{6597E619-B9A1-4048-9874-A4575E1F2F1C}";
    }

    std::wstring out_file_path = ddpath::join({ out_file_dir_path, guid});
    auto cmd = ddstr::format("-helper %s %s", ddstr::utf16_ansi(target).c_str(), ddstr::utf16_ansi(out_file_path).c_str());
    auto helper_porcess = ddsub_process::create_instance(ddstr::utf16_ansi(hepler_exe_path), cmd);
    s32 wait_times = 6;
    s32 wait_duration = 10000; // 10s
    for (s32 i = 0; i < wait_times; ++i) {
        if (helper_porcess->wait(wait_duration)) {
            break;
        }
        if (i == wait_times - 1) {
            return false;
        }
        ddcout(ddconsole_color::cyan) << ddstr::format("Still Finding...\n");
    }
    s32 exit_code = 0;
    if (!helper_porcess->get_exit_code(exit_code) || exit_code != 0) {
        return false;
    }

    std::unique_ptr<ddfile> out_file(ddfile::create_utf8_file(out_file_path));
    if (out_file == nullptr) {
        return false;
    }

    ddcout(ddconsole_color::cyan) << L"Process id:\n";
    while (true) {
        std::string line;
        if (!out_file->read_linea(line)) {
            break;
        }

        ddcout(ddconsole_color::blue) << ddstr::format("%s", line.c_str());
    }

    ddcout(ddconsole_color::cyan) << L"\n";
    return true;
}
}
