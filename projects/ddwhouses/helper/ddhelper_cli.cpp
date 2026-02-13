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

bool ddhelper_cli::is_cmd_for_current_idl(const std::vector<std::wstring>& cmds)
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

    std::set<ULONG_PTR> process_ids;
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
    (void)ddprocess::enum_file_handles([&process_ids, &target_file](const ddhandle_info& info) {
        std::wstring lower = ddstr::lower(info.base_object_name.c_str());
        if (ddstr::strwildcard(lower.c_str(), target_file.c_str())) {
            process_ids.insert(info.process_id);
        }
    });

    for (auto porcess_id : process_ids) {
        ddcout(ddconsole_color::cyan) << ddstr::format("process: %d\n", porcess_id);
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
        for (auto porcess_id : process_ids) {
            std::string id = ddstr::format("%d\n", porcess_id);
            (void)out_file->write((u8*)id.c_str(), (s32)id.size());
        }
    }

    return dderror_code::success;
}
}
