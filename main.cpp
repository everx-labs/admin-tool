#include <iostream>

#include "fmt/core.h"
#include "json-block.h"
#include "nlohmann/json.hpp"

#include "crypto/vm/cellslice.h"
#include "crypto/vm/boc.h"
#include "common/util.h"

#include "td/utils/logging.h"
#include "td/utils/port/path.h"
#include "td/utils/PathView.h"

#include "fift/Fift.h"
#include "fift/words.h"
#include "block/block.h"
#include "vm/atom.h"

#include "generated/generated.hpp"
#include "gql-client/gql-client.hpp"

using json = nlohmann::json;

#define HARD_CODE "hard_code"
#define FIF_UNWRAP(E, C) (static_cast<td::FifRes<int>&&>(E)).unwrap_fift_error(C)

namespace fift {
    
Ref<vm::Cell> base64_boc_to_cell(const std::string& base64_boc) {
    auto bytes = td::str_base64_decode(
        td::Slice(base64_boc), 
        true
    );

    vm::BagOfCells boc;
    auto res = boc.deserialize(td::Slice{bytes});

    if (res.is_error()) {
        throw IntError{(PSLICE() << "cannot deserialize bag-of-cells " << res.error()).c_str()};
    }
    
    if (res.ok() <= 0 || boc.get_root_cell().is_null()) {
        throw IntError{"cannot deserialize bag-of-cells "};
    }

    return boc.get_root_cell();
}


td::RefInt256 hex_to_ref_int(const std::string& hex) {
    auto i = td::RefInt256{true};
    auto s = hex.substr(2);

    auto r = i.unique_write().parse_hex(s.data(), (int) s.size());
    if (r != (int) s.size()) { throw IntError{"error parse hex"}; }

    return i;
}

void interpret_cfg_param_to_json(vm::Stack& stack) {
    stack.check_underflow(2);
    auto param = stack.pop_smallint_range(INT32_MAX, INT32_MIN);

    vm::BagOfCells boc;
    boc.add_root(stack.pop_cell());

    auto res = boc.import_cells();
    if (res.is_error()) {
        throw IntError{(PSLICE() << "cannot serialize bag-of-cells " << res.error()).c_str()};
    }

    auto to_bind = td::str_base64_encode(boc.serialize_to_string());
    auto result = config_param_boc_to_json(to_bind, param);

    stack.push_string(json::parse(result.str)["p" + std::to_string(param)].dump());
}

void interpret_json_to_cfg_param(vm::Stack& stack) {
    stack.check_underflow(2);
    auto param = stack.pop_smallint_range(INT32_MAX, INT32_MIN);
    auto json_str = stack.pop_string();

    json json_wrapped = {{"p" + std::to_string(param), json::parse(json_str)}};
    auto cfg_param_boc = config_param_json_to_boc(json_wrapped.dump(), param);
    auto cfg_param_cell = base64_boc_to_cell(cfg_param_boc.str);

    stack.push_cell(cfg_param_cell);
}


using pseudo_stk_dict = std::vector<std::pair<std::string, vm::StackEntry>>;

td::Ref<vm::Tuple> pseudo_stk_dict_pack(const pseudo_stk_dict& d) {
    auto tuple = Ref<vm::Tuple>{true};

    for (const auto& p : d) {
        auto v = Ref<vm::Tuple>{true}.write();
        v.emplace_back(vm::Atom::find(p.first, true));
        v.emplace_back(p.second);
        tuple.write().emplace_back(v);
    }

    return tuple;
}

void interpret_get_account(vm::Stack& stack) {
    stack.check_underflow(3);
    auto url = stack.pop_string();

    block::StdAddress addr;

    td::RefInt256 x = stack.pop_int_finite();
    if (td::sgn(x) < 0) {
        throw IntError{"non-negative integer expected"};
    }

    CHECK(x->export_bytes(addr.addr.data(), 32, false));

    addr.workchain = stack.pop_smallint_range(INT8_MAX, INT8_MIN);

    auto client = Gql::Client(url);
    auto resp = client.get_account_info(addr.rserialize(true));

    pseudo_stk_dict to_stack = {
        std::make_pair(".code", base64_boc_to_cell(resp.code)),
        std::make_pair(".data", base64_boc_to_cell(resp.data)),
        std::make_pair(".balance", hex_to_ref_int(resp.balance)),
    };

    stack.push_tuple(pseudo_stk_dict_pack(to_stack));
}

void init_words_admin_tool(Dictionary& d) {
    using namespace std::placeholders;

    d.def_stack_word("cfg>$j ", interpret_cfg_param_to_json); // c i -- $
    d.def_stack_word("$j>cfg ", interpret_json_to_cfg_param); // $ i -- c

    // wc addr url -- t
    d.def_stack_word("~>acc", interpret_get_account);
}

}

namespace td {

template <class T = td::Unit>
class FifRes : public Result<T> {
public:
    void unwrap_fift_error(std::string_view command = "") {
        if (this->is_error()) {
            LOG(ERROR) << fmt::format("error interpreting command \"{}\": {}",
                                      command, this->error().message().str());
            std::exit(2);
        }
    }
};

}

void usage(const char* prog) {
    std::cerr << "admin-tool – everscale network management kit\n\n";
    std::cerr << fmt::format("usage: {} <command> [args]\n\n", prog);

    std::cerr << "commands:\n"
                 "\texport-cfg\texport blockchain config to json\n"
                 "\timport-cfg\timport blockchain config from json\n"
                 "\textmsg-cfg\tprepare ext message for config smc\n"
                 "\tdataof-cfg\tshow basic information of config smc\n"
                 "\n"
                 "\textmsg-chk\tprepare ext message for extmsg-test\n"
                 "\tdataof-chk\tshow basic information of extmsg-test\n";

    std::cerr << "\narguments:"
                 "\n\t-v <lvl>\tset verbosity level\n"
                 "\t-V        \tshow tool build information\n"
                 "\t-h        \tshow help information (usage)\n";

    std::exit(2);
}

int main(int argc, char* const argv[]) {
    int new_verbosity_level = VERBOSITY_NAME(INFO), i = 0;

    while ((i = getopt(argc, argv, "v:Vh")) != -1) {
        switch (i) {
            case 'v':
                new_verbosity_level = VERBOSITY_NAME(FATAL) + td::to_integer<int>(td::Slice(optarg));
                break;
            case 'V':
                std::cout << "admin-tool build information: [ version: v0.1.0 ]\n";
                std::exit(0);
            case 'h':
            default:
                usage(argv[0]);
        }
    }
    
    SET_VERBOSITY_LEVEL(new_verbosity_level);

    if (argv[optind] == nullptr) {
        std::cerr << "positional argument \"command\" must be set\n";
        usage(argv[0]);
        std::exit(2);
    }

    auto command = std::string(argv[optind]);
    LOG(DEBUG) << fmt::format("command: {}", command);

    fift::Fift::Config config;

    std::string current_dir;
    std::vector<std::string> source_include_path;

    auto r_current_dir = td::realpath(".");
    if (r_current_dir.is_ok()) {
        current_dir = r_current_dir.move_as_ok();
        source_include_path.push_back(current_dir);
    }

    config.source_lookup = fift::SourceLookup(std::make_unique<fift::OsFileLoader>());
    for (auto& path : source_include_path) {
        config.source_lookup.add_include_path(path);
    }

    fift::init_words_common(config.dictionary);
    fift::init_words_vm(config.dictionary, true);
    fift::init_words_ton(config.dictionary);
    fift::init_words_admin_tool(config.dictionary);

    optind++; // command -> real first arg
    fift::import_cmdline_args(config.dictionary, command,
                              argc - optind, argv + optind);

    fift::Fift fift(std::move(config));

    fift.interpret_istream(FiftDecl::is_FIFT_FIF,    HARD_CODE, false);
    fift.interpret_istream(FiftDecl::is_ASM_FIF,     HARD_CODE, false);
    fift.interpret_istream(FiftDecl::is_COLOR_FIF,   HARD_CODE, false);
    fift.interpret_istream(FiftDecl::is_LISTS_FIF,   HARD_CODE, false);
    fift.interpret_istream(FiftDecl::is_TONUTIL_FIF, HARD_CODE, false);
    fift.interpret_istream(FiftDecl::is_GETOPT_FIF,  HARD_CODE, false);

    if (command == "export-cfg") {
        FIF_UNWRAP(fift.interpret_istream(FiftDecl::is_EXPORT_CFG_FIF,
                                          HARD_CODE, false), command);
        return 0;
    }

    if (command == "import-cfg") {
        FIF_UNWRAP(fift.interpret_istream(FiftDecl::is_IMPORT_CFG_FIF,
                                          HARD_CODE, false), command);
        return 0;
    }

    if (command == "extmsg-cfg") {
        FIF_UNWRAP(fift.interpret_istream(FiftDecl::is_EXTMSG_CFG_FIF,
                                          HARD_CODE, false), command);
        return 0;
    }

    if (command == "dataof-cfg") {
        FIF_UNWRAP(fift.interpret_istream(FiftDecl::is_DATAOF_CFG_FIF,
                                          HARD_CODE, false), command);
        return 0;
    }

    if (command == "extmsg-chk") {
        FIF_UNWRAP(fift.interpret_istream(FiftDecl::is_EXTMSG_CHK_FIF,
                                          HARD_CODE, false), command);
        return 0;
    }

    if (command == "dataof-chk") {
        FIF_UNWRAP(fift.interpret_istream(FiftDecl::is_DATAOF_CHK_FIF,
                                          HARD_CODE, false), command);
        return 0;
    }

    std::cerr << "invalid value passed to positional argument \"command\"\n";
    return 2;
}
