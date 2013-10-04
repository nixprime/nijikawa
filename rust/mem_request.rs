use std::u64;

pub enum MemRequestType {
    Read,
    Write,
}

pub struct MemRequest {
    addr: u64,
    req_type: MemRequestType,
    response_receiver: Option<@mut MemResponseReceiver>,
}

pub struct MemResponse {
    addr: u64,
}

pub trait MemRequestReceiver {
    fn receive_request(&mut self, request: ~MemRequest);
}

pub trait MemResponseReceiver {
    fn receive_response(&mut self, cycle: i64, response: ~MemResponse);
}

impl MemRequest {
    pub fn respond(&self, cycle: i64) {
        match self.response_receiver {
            Some(receiver) => receiver.receive_response(
                    cycle, ~MemResponse { addr: self.addr }),
            None => ()
        }
    }
}
