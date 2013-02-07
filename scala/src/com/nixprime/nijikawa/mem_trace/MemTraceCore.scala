package com.nixprime.nijikawa.mem_trace

import scala.language.implicitConversions

import com.nixprime.nijikawa.Simulator
import com.nixprime.nijikawa.common.{MemRequestType, MemResponse, Address, MemRequest}
import collection.mutable

class MemTraceCore(private val sim: Simulator, private val traceReader: MemTraceReader,
                   private val requestFn: MemRequest => Unit) {
  // Parameters
  var superscalarWidth: Int = 1
  var robSize: Int = 1

  // State
  var robHead: Int = 0
  var robTail: Int = 0
  var robInsns: Int = 0
  var curMem: Option[MemTraceRecord] = None
  var precIssued: Long = 0
  var rob = new Array[Long](0)
  var mshrs = new mutable.HashMap[Address, Mshr]
  implicit def orderedCycleMemResponse(elem: (Long, MemResponse)): Ordered[(Long, MemResponse)] =
      new Ordered[(Long, MemResponse)] {
    def compare(other: (Long, MemResponse)) = other._1.compare(elem._1)
  }
  var responseQueue = new mutable.PriorityQueue[(Long, MemResponse)]()

  // Stats
  var insnsRetired: Long = 0

  class Mshr(val addr: Address) {
    var issued = false
    var robIndices = List[Int]()
  }

  def init() {
    rob = new Array[Long](robSize)
    curMem = traceReader.next()
  }

  def tick() {
    tickRetire()
    deliverReadResponses()
    tickIssue()
  }

  def scheduleReadResponse(cycle: Long, response: MemResponse) {
    responseQueue += ((cycle, response))
  }

  private def tickIssue() {
    var remainingIssue = superscalarWidth
    while (remainingIssue > 0 && robInsns < robSize) {
      shouldIssueMem() match {
        case false => {
          // Issue non-memory instruction
          rob(robTail) = sim.now
          precIssued += 1
        }
        case true => {
          // Issue memory instruction
          curMem match {
            case Some(record) => issueMem(record, robTail)
            case None => throw new Error("Issuing invalid memory instruction")
          }
          precIssued = 0
          curMem = traceReader.next()
        }
      }
      robTail += 1
      if (robTail == robSize) {
        robTail = 0
      }
      robInsns += 1
      remainingIssue -= 1
    }
  }

  private def tickRetire() {
    var remainingRetire = superscalarWidth
    while (remainingRetire > 0 && robInsns > 0 && rob(robHead) <= sim.now) {
      robHead += 1
      if (robHead == robSize) {
        robHead = 0
      }
      robInsns -= 1
      remainingRetire -= 1
      insnsRetired += 1
    }
  }

  private def deliverReadResponses() {
    while (!responseQueue.isEmpty && responseQueue.head._1 <= sim.now) {
      val (_, response) = responseQueue.dequeue()
      receiveReadResponse(response)
    }
  }

  private def issueMem(record: MemTraceRecord, robIndex: Int) {
    record.isWrite match {
      case true => {
        requestWrite(record.addr)
        rob(robIndex) = sim.now
      }
      case false => {
        val mshr = getMshr(record.addr)
        mshr.robIndices ::= robIndex
        rob(robIndex) = Long.MaxValue
        issueMshr(mshr)
      }
    }
  }

  private def getMshr(addr: Address): Mshr = {
    mshrs.get(addr) match {
      case Some(mshr) => mshr
      case None => {
        val mshr = new Mshr(addr)
        mshrs += ((addr, mshr))
        mshr
      }
    }
  }

  private def issueMshr(mshr: Mshr) {
    if (!mshr.issued) {
      requestRead(mshr.addr)
      mshr.issued = true
    }
  }

  private def requestRead(addr: Address) {
    requestFn(new MemRequest(MemRequestType.Read, addr, scheduleReadResponse _))
  }

  private def requestWrite(addr: Address) {
    requestFn(new MemRequest(MemRequestType.Write, addr))
  }

  private def receiveReadResponse(response: MemResponse) {
    mshrs.remove(response.addr) match {
      case Some(mshr) => mshr.robIndices.map { case i => rob(i) = sim.now }
      case None => throw new Error("Received unexpected response to 0x" +
          response.addr.value.toHexString)
    }
  }

  private def shouldIssueMem(): Boolean = {
    curMem match {
      case Some(record) => precIssued >= record.prec
      case None => false
    }
  }
}
