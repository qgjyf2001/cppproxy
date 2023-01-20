use core::slice;
use std::{os::raw::{c_uchar,c_uint}};
extern crate libc;
use crate::filter;
#[repr(C)]
struct Ret {
    ptr: *const c_uchar,
    len: c_uint,
    cap: c_uint
}
#[no_mangle]
extern "C" fn filter(sockfd:c_uint,content:*const c_uchar,len:c_uint,reason:*mut c_uint)->Ret {
    let mut reason_:i32=filter::NONE;
    let mut result:Vec<u8>;
    unsafe {
        let bytes=slice::from_raw_parts(content, len as usize);
        result=crate::INSTANCE.do_filter(sockfd as i32,bytes.to_vec(),&mut reason_);
        *reason=reason_ as c_uint;
    }
    let ptr_=result.as_mut_ptr();
    let len_=result.len();
    let cap_=result.capacity();
    std::mem::forget(result);
    Ret{ptr:ptr_,len:len_ as c_uint,cap:cap_ as c_uint}
    
}
#[no_mangle]
extern "C" fn rust_module_free(obj:Ret) {
    unsafe {
        Vec::<u8>::from_raw_parts(obj.ptr as *mut u8, obj.len as usize, obj.cap as usize);
    }
}
pub trait Cfunc {
    fn do_filter(&self,sockfd:i32,content:Vec<u8>,reason:& mut i32)->Vec<u8>;
}