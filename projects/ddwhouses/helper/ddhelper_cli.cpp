#include "ddwhouses/stdafx.h"

#include "ddhelper_cli.h"
#include "ddbase/ddio.h"
#include "ddbase/str/ddstr.h"
#include "ddbase/file/ddfile.h"
#include "ddbase/file/dddir.h"
#include "ddbase/windows/ddprocess.h"

#include <set>

namespace NSP_DD {

enum dderror_code {
    success = 0,
    cmd_error,
    open_out_file_failure,
    delete_out_file_failure,
};

std::string ddhelper_cli::help()
{
    std::string helper_str;
    helper_str += ddstr::format("ddwhouses.exe <-helper> <target_file_path> [out_file_path] \n");
    helper_str += ddstr::format("* represents zoro or more arbitrary characters; '?' represents one character (in Unicode, Chinese characters count as two characters)\n");
    return helper_str;
}

bool ddhelper_cli::is_cmd_for_current_cli(const std::vector<std::wstring>& cmds)
{
    if (cmds.size() < 2) {
        return false;
    }

    if (cmds[1] != L"-helper") {
        return false;
    }

    return true;
}

s32 ddhelper_cli::run(const std::vector<std::wstring>& cmds)
{
    if (cmds.size() < 3) {
        return dderror_code::cmd_error;
    }

    std::set<std::pair<ULONG_PTR, std::wstring>> process_files;
    std::wstring target_file = cmds[2];
    if (target_file.empty()) {
        return false;
    }
    ddstr::to_lower(target_file.data());
    if (target_file[target_file.length() - 1] != L'*') {
        target_file.append(1, L'*');
    }
    if (target_file[0] != L'*') {
        target_file = L"*" + target_file;
    }
    (void)ddprocess::enum_file_handles([&process_files, &target_file](const ddhandle_info& info) {
        std::wstring lower = ddstr::lower(info.base_object_name.c_str());
        if (ddstr::strwildcard(lower.c_str(), target_file.c_str())) {
            process_files.insert({ info.process_id, info.base_object_name });
        }
    });

    for (const auto& [process_id, file_name] : process_files) {
        ddcout(ddconsole_color::cyan) << ddstr::format(
            L"process: %llu, file: %s\n",
            static_cast<unsigned long long>(process_id),
            file_name.c_str());
    }

    if (cmds.size() > 3) {
        std::wstring output_file = cmds[3];
        if (!dddir::is_dir(output_file)) {
            if (!dddir::delete_path(output_file)) {
                return dderror_code::delete_out_file_failure;
            }
        }

        std::unique_ptr<ddfile> out_file(ddfile::create_utf8_file(output_file));
        if (out_file == nullptr) {
            return dderror_code::open_out_file_failure;
        }
        for (const auto& [process_id, file_name] : process_files) {
            std::string line = ddstr::format(
                "%llu\t%s\n",
                static_cast<unsigned long long>(process_id),
                ddstr::utf16_8(file_name).c_str());
            (void)out_file->write((u8*)line.c_str(), (s32)line.size());
        }
    }

    return dderror_code::success;
}
}
