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

#include <map>
#include <winternl.h>

#pragma comment(lib, "advapi32.lib")

namespace NSP_DD {
std::wstring ddmain_cli::get_process_user(HANDLE process)
{
    HANDLE token = nullptr;
    if (!::OpenProcessToken(process, TOKEN_QUERY, &token)) {
        return L"-";
    }
    ddexec_guard token_guard([token]() {
        ::CloseHandle(token);
    });

    DWORD token_size = 0;
    (void)::GetTokenInformation(token, TokenUser, nullptr, 0, &token_size);
    std::vector<u8> token_buffer(token_size);
    if (token_size == 0 || !::GetTokenInformation(token, TokenUser, token_buffer.data(), token_size, &token_size)) {
        return L"-";
    }

    auto token_user = reinterpret_cast<TOKEN_USER*>(token_buffer.data());
    DWORD account_size = 0;
    DWORD domain_size = 0;
    SID_NAME_USE sid_type = SidTypeUnknown;
    (void)::LookupAccountSidW(nullptr, token_user->User.Sid, nullptr, &account_size, nullptr, &domain_size, &sid_type);
    std::vector<wchar_t> account(account_size);
    std::vector<wchar_t> domain(domain_size);
    bool found = account_size > 0 && ::LookupAccountSidW(
        nullptr,
        token_user->User.Sid,
        account.data(),
        &account_size,
        domain.data(),
        &domain_size,
        &sid_type);
    if (!found) {
        return L"-";
    }
    if (domain_size == 0) {
        return account.data();
    }
    return std::wstring(domain.data()) + L"\\" + account.data();
}

std::wstring ddmain_cli::get_process_started(HANDLE process)
{
    FILETIME created = {};
    FILETIME exited = {};
    FILETIME kernel = {};
    FILETIME user = {};
    if (!::GetProcessTimes(process, &created, &exited, &kernel, &user)) {
        return L"-";
    }

    FILETIME local_created = {};
    SYSTEMTIME system_time = {};
    if (!::FileTimeToLocalFileTime(&created, &local_created) ||
        !::FileTimeToSystemTime(&local_created, &system_time)) {
        return L"-";
    }
    return ddstr::format(
        L"%04u-%02u-%02u %02u:%02u:%02u",
        system_time.wYear,
        system_time.wMonth,
        system_time.wDay,
        system_time.wHour,
        system_time.wMinute,
        system_time.wSecond);
}

std::wstring ddmain_cli::get_process_command_line(HANDLE process)
{
    using nt_query_information_process = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    auto ntdll = ::GetModuleHandleW(L"ntdll.dll");
    auto query = reinterpret_cast<nt_query_information_process>(
        ::GetProcAddress(ntdll, "NtQueryInformationProcess"));
    if (query == nullptr) {
        return L"-";
    }

    constexpr ULONG process_command_line_information = 60;
    ULONG buffer_size = 0;
    (void)query(process, process_command_line_information, nullptr, 0, &buffer_size);
    if (buffer_size == 0) {
        return L"-";
    }

    std::vector<u8> buffer(buffer_size);
    if (query(process, process_command_line_information, buffer.data(), buffer_size, &buffer_size) < 0) {
        return L"-";
    }
    auto command_line = reinterpret_cast<UNICODE_STRING*>(buffer.data());
    if (command_line->Buffer == nullptr || command_line->Length == 0) {
        return L"-";
    }
    return std::wstring(command_line->Buffer, command_line->Length / sizeof(wchar_t));
}

bool ddmain_cli::query_process_info(DWORD process_id, printable_process_info& info)
{
    DWORD session_id = 0;
    if (::ProcessIdToSessionId(process_id, &session_id)) {
        info.session = std::to_wstring(session_id);
    }

    HANDLE process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (process == nullptr) {
        return false;
    }
    ddexec_guard process_guard([process]() {
        ::CloseHandle(process);
    });

    DWORD exit_code = 0;
    if (!::GetExitCodeProcess(process, &exit_code) || exit_code != STILL_ACTIVE) {
        return false;
    }

    std::wstring executable = ddprocess::get_process_fullpath(process);
    if (!executable.empty()) {
        info.executable = executable;
        size_t separator = executable.find_last_of(L"\\/");
        info.name = separator == std::wstring::npos ? executable : executable.substr(separator + 1);
    }
    info.user = get_process_user(process);
    info.started = get_process_started(process);
    info.command_line = get_process_command_line(process);
    return true;
}

void ddmain_cli::print_process_info(
    const std::wstring& file_name,
    DWORD process_id,
    const printable_process_info& info)
{
    ddcout(ddconsole_color::cyan) << L"File:         ";
    ddcout(ddconsole_color::blue) << file_name << L"\n";
    ddcout(ddconsole_color::cyan) << L"Process:      ";
    ddcout(ddconsole_color::blue) << info.name << L"\n";
    ddcout(ddconsole_color::cyan) << L"PID:          ";
    ddcout(ddconsole_color::blue) << std::to_wstring(process_id) << L"\n";
    ddcout(ddconsole_color::cyan) << L"User:         ";
    ddcout(ddconsole_color::blue) << info.user << L"\n";
    ddcout(ddconsole_color::cyan) << L"Session:      ";
    ddcout(ddconsole_color::blue) << info.session << L"\n";
    ddcout(ddconsole_color::cyan) << L"Started:      ";
    ddcout(ddconsole_color::blue) << info.started << L"\n";
    ddcout(ddconsole_color::cyan) << L"Executable:   ";
    ddcout(ddconsole_color::blue) << info.executable << L"\n";
    ddcout(ddconsole_color::cyan) << L"Command Line: ";
    ddcout(ddconsole_color::blue) << info.command_line << L"\n\n";
}

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

bool ddmain_cli::is_cmd_for_current_cli(const std::vector<std::wstring>& cmds)
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
    auto cmd = ddstr::format("-helper \"%s\" \"%s\"", ddstr::utf16_ansi(target).c_str(), ddstr::utf16_ansi(out_file_path).c_str());
    auto helper_porcess = ddsub_process::create_instance(ddstr::utf16_ansi(hepler_exe_path), cmd);
    s32 wait_times = 6;
    s32 wait_duration = 10000; // 10s
    for (s32 i = 0; i < wait_times; ++i) {
        helper_porcess->ignore_output();
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

    std::map<DWORD, printable_process_info> process_details;
    ddcout(ddconsole_color::cyan) << L"Matched processes:\n\n";
    while (true) {
        std::string line;
        if (!out_file->read_linea(line)) {
            break;
        }

        size_t separator = line.find('\t');
        if (separator == std::string::npos) {
            continue;
        }
        DWORD process_id = static_cast<DWORD>(std::strtoul(line.substr(0, separator).c_str(), nullptr, 10));
        std::string file_name_utf8 = line.substr(separator + 1);
        ddstr::trim(file_name_utf8, std::vector<char>{ '\r', '\n' });
        std::wstring file_name = ddstr::utf8_16(file_name_utf8);

        auto detail = process_details.find(process_id);
        if (detail == process_details.end()) {
            printable_process_info info;
            if (!query_process_info(process_id, info)) {
                continue;
            }
            detail = process_details.emplace(process_id, std::move(info)).first;
        }
        print_process_info(file_name, process_id, detail->second);
    }
    return true;
}
}
