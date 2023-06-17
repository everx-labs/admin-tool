//
// Created by Alexander Gapak on 6/15/23.
//

#include "gql-client.hpp"

#include <sstream>

#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <utility>

#include <fmt/core.h>

using namespace Gql;

AccountInfo::AccountInfo(const json &j) {
    j.at("code").get_to(this->code);
    j.at("data").get_to(this->data);
    j.at("balance").get_to(this->balance);
}

Client::Client(std::string endpoint): endpoint(std::move(endpoint)) {}

json Client::send_request(std::string query) {
    query.erase(std::remove(query.begin(), query.end(), '\n'), query.end());
    query.erase(std::remove(query.begin(), query.end(), ' '), query.end());

    curlpp::Easy req;

    req.setOpt<curlpp::options::Url>(this->endpoint);
    req.setOpt(curlpp::options::HttpHeader({"Content-Type: application/json"}));

    json json_data = json {{"query", query}, {"variables", json::object()}};

    std::string data = json_data.dump();
    req.setOpt<curlpp::options::PostFields>(data);
    req.setOpt<curlpp::options::PostFieldSize>((int32_t) data.size());

    std::stringstream res;

    req.setOpt(cURLpp::Options::WriteStream(&res));
    req.perform();

    return json::parse(res.str());
}

AccountInfo Client::get_account_info(std::string_view account) {
    auto query = fmt::format(R"(
        query {{
          blockchain {{
            account(address: "{}") {{
              info {{ code, data, balance(format: HEX) }}
            }}
          }}
        }}
    )", account);

    auto result = send_request(query);
    return AccountInfo(result["data"]["blockchain"]["account"]["info"]);
}
