// json-block.h

#pragma once

extern "C" {
    const char* _c_config_param_boc_to_json(const char* input, int32_t param);
    const char* _c_config_param_json_to_boc(const char* input, int32_t param);
    void _c_free_rust_string(const char* ptr);
}

class RustStr {
    public: const char* str;

    explicit RustStr(const char* str) {
        this->str = str;
    }

    ~RustStr() {
        _c_free_rust_string(this->str);
    }
};

RustStr config_param_boc_to_json(std::string const &boc, int32_t param) {
    return RustStr(_c_config_param_boc_to_json(boc.c_str(), param));
}

RustStr config_param_json_to_boc(std::string const &json, int32_t param) {
    return RustStr(_c_config_param_json_to_boc(json.c_str(), param));
}
