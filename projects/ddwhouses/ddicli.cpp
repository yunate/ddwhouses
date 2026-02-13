#include "ddwhouses/stdafx.h"

#include "ddicli.h"
#include "helper/ddhelper_cli.h"
#include "ddmain_cli.h"

namespace NSP_DD {
std::vector<std::unique_ptr<ddicli>> ddicli::get_all_cli_tools()
{
    std::vector<std::unique_ptr<ddicli>> all_tools;
    all_tools.push_back(std::make_unique<ddhelper_cli>());
    all_tools.push_back(std::make_unique<ddmain_cli>());
    return all_tools;
}
}
