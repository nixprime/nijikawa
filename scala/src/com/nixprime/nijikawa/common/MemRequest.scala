package com.nixprime.nijikawa.common

object MemRequestType extends Enumeration {
  type MemRequestType = Value
  val Read = Value("Read")
  val Write = Value("Write")
}
import MemRequestType._

class MemRequest(val reqType: MemRequestType, val addr: Address,
                 val responseFn: Option[(Long, MemResponse) => Unit]) {
  def this(reqType: MemRequestType, addr: Address) = this(reqType, addr, None)
  def this(reqType: MemRequestType, addr: Address, responseFn: (Long, MemResponse) => Unit) =
      this(reqType, addr, Some(responseFn))

  def respond(cycle: Long) {
    responseFn match {
      case Some(fn) => fn(cycle, new MemResponse(addr))
      case None => Unit
    }
  }
}
