//
// Created by Alexander Gapak on 6/15/23.
//

#pragma once

#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Gql {

struct AccountInfo {
    std::string code;
    std::string data;
    std::string balance;

    explicit AccountInfo(const json& j);
};

class Client {

private:
    const std::string endpoint;

public:
    explicit Client(std::string endpoint);

    json send_request(std::string request);
    AccountInfo get_account_info(std::string_view account);
};

}
