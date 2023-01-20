mod bind;
mod filter;
use once_cell::sync::Lazy;
static mut INSTANCE:Lazy<Box<dyn bind::Cfunc>>=Lazy::new(||{
    Box::new(Test{})
});
pub struct Test {
    
}
impl bind::Cfunc for Test {
    fn do_filter(&self,sockfd:i32,content:Vec<u8>,reason:& mut i32)->Vec<u8> {
        let content_=String::from_utf8(content).unwrap();
        let mut result:Vec<&str>=content_.split(" ").collect();
        result.insert(result.len()-1, "rust");
        let ret=result.join(" ");
        *reason=filter::NONE;
        ret.as_bytes().to_vec()
    }
}