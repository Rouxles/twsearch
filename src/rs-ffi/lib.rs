use std::{
    ffi::{c_char, CStr, CString},
    ptr::null_mut,
};

use twsearch::scramble::{random_scramble_for_event, Event};

/// # Safety
///
/// This function can panic. If you are working in pure Rust, use [`twsearch::scramble::random_scramble_for_event`] instead.
///
/// Returns:
/// - A null pointer for *any* error.
/// - A valid scramble (in the form of C string) otherwise.
#[no_mangle]
pub unsafe extern "C" fn ffi_random_scramble_for_event(event_raw_cstr: *mut c_char) -> *mut c_char {
    // TODO: we can't avoid leaking the return value, but we could give a function to free all past returned values.
    match ffi_random_scramble_for_event_internal(event_raw_cstr) {
        Ok(scramble_raw_cstr) => scramble_raw_cstr,
        Err(_) => null_mut(),
    }
}

fn ffi_random_scramble_for_event_internal(event_raw_cstr: *mut c_char) -> Result<*mut c_char, ()> {
    let event_cstr = unsafe { CStr::from_ptr(event_raw_cstr) };
    let event_str = event_cstr.to_str().map_err(|_| ())?;
    let event = Event::try_from(event_str).map_err(|_| ())?;
    let result_str = random_scramble_for_event(event)
        .map_err(|_| ())?
        .to_string();
    Ok(CString::new(result_str).unwrap().into_raw())
}

#[test]
fn ffi_test() {
    let dylib_path = test_cdylib::build_current_project();
    let lib = unsafe { libloading::Library::new(dylib_path).unwrap() };
    let func: libloading::Symbol<unsafe extern "C" fn(event_raw_cstr: *mut c_char) -> *mut c_char> =
        unsafe { lib.get(b"ffi_random_scramble_for_event").unwrap() };
    for event_id in ["222", "pyram"] {
        let event_raw_cstr = CString::new((event_id).to_owned()).unwrap().into_raw();
        let scramble_raw_cstr = unsafe { func(event_raw_cstr) };
        let scramble_cstr = unsafe { CStr::from_ptr(scramble_raw_cstr) };
        let scramble_str = scramble_cstr.to_str().map_err(|_| ()).unwrap();
        let alg = scramble_str.parse::<cubing::alg::Alg>().unwrap();
        assert!(alg.nodes.len() >= 11);
        assert!(alg.nodes.len() <= 15); // TODO: are there any states that can't be reached in exactly 11 moves for our scramble generators (plus tips for pyra)?
    }
}
