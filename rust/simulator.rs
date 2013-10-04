use std::i64;

struct Simulator {
    priv current_cycle: i64,
}

impl Simulator {
    pub fn new() -> Simulator { Simulator { current_cycle: 0 } }
    pub fn now(&self) -> i64 { self.current_cycle }
    pub fn tick(&mut self) { self.current_cycle += 1; }
}
