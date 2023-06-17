use std::ffi::CString;
use std::ffi::CStr;
use std::os::raw::c_char;
use ton_block::{ConfigParams, Deserializable};

use ton_block_json::{parse_config, serialize_config_param};
use ton_types::{base64_encode, boc, BuilderData, Cell, IBitstring, SliceData, UInt256};

use serde_json::{Map, Value};

fn load_cfg_param_to_str(index: u32, data: &Cell) -> String {
    let params = &ConfigParams::with_address_and_root(
        UInt256::new(),
        data.to_owned()
    );

    return serialize_config_param(params, index).unwrap();
}

fn string_to_map(input_string: &str) -> Map<String, Value> {
    let json_value: Value = serde_json::from_str(input_string).unwrap();

    if let Value::Object(map) = json_value {
        return map;
    } else {
        panic!("Invalid JSON format");
    }
}


fn load_cfg_param_to_str_from_boc(boc: &String, index: u32) -> String {
    let cell = Cell::construct_from_base64(&boc).unwrap();
    return load_cfg_param_to_str(index, &cell);
}

fn load_cfg_param_to_boc_from_str(str: &String, index: u32) -> String {
    let map = string_to_map(str);
    let cfg = parse_config(&map).unwrap().config_params;

    let mut key = BuilderData::new();
    let key = key.append_i32(index as i32).unwrap();
    let key = SliceData::load_builder(key.to_owned()).unwrap();

    let v = cfg.get(key).unwrap().unwrap().reference(0).unwrap();
    let boc = boc::write_boc(&v).unwrap();

    return base64_encode(boc);
}

#[no_mangle]
pub extern "C" fn _c_config_param_boc_to_json(input: *const c_char, param: i32) -> *mut c_char {
    let input_str = unsafe {
        assert!(!input.is_null());
        CStr::from_ptr(input)
    };

    let input_string = input_str.to_str().unwrap();
    let output_string = load_cfg_param_to_str_from_boc(
        &input_string.to_string(),
        param as u32
    );

    let output_cstring = CString::new(output_string).unwrap();
    output_cstring.into_raw()
}

#[no_mangle]
pub extern "C" fn _c_config_param_json_to_boc(input: *const c_char, param: i32) -> *mut c_char {
    let input_str = unsafe {
        assert!(!input.is_null());
        CStr::from_ptr(input)
    };

    let input_string = input_str.to_str().unwrap();
    let output_string = load_cfg_param_to_boc_from_str(
        &input_string.to_string(),
        param as u32
    );

    let output_cstring = CString::new(output_string).unwrap();
    output_cstring.into_raw()
}

#[no_mangle]
pub extern "C" fn _c_free_rust_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe { let _ = CString::from_raw(ptr); }
    }
}
