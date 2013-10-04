pub fn cons_vec<T>(n: uint, cons: &fn() -> T) -> ~[T] {
    range(0, n).map(|_| { cons() }).collect()
}
