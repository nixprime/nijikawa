package com.nixprime.nijikawa.component

import collection.mutable.ListBuffer
import com.nixprime.nijikawa.Simulator
import com.nixprime.nijikawa.common.MemRequest

class Dram(private val sim: Simulator, val channelBits: Int, val bankBits: Int) {
  var bankLsb: Int = Dram.RowSizeBits + channelBits
  var rowLsb: Int = bankLsb + bankBits

  var clockDivider: Int = 4
  var tCCD: Int = 4
  var tCL: Int = 11
  var tRCD: Int = 11
  var tRP: Int = 11
  var tRAS: Int = 28

  object RowState extends Enumeration {
    type RowState = Value
    val Hit = Value("Hit")
    val Miss = Value("Miss")
    val Conflict = Value("Conflict")
  }

  import RowState._

  class Request(val memRequest: MemRequest) {
    val channel: Int = ((memRequest.addr.value >>> Dram.OffsetBits) & ((1 << channelBits) - 1)).toInt
    val bank: Int = ((memRequest.addr.value >>> bankLsb) & ((1 << bankBits) - 1)).toInt
    val row: Long = (memRequest.addr.value >>> rowLsb)
  }

  class BankState {
    var openRow: Long = Dram.NoOpenRow
    var nextRequest: Long = 0
    var nextConflict: Long = 0
  }

  class ChannelState(numBanks: Int) {
    var waitingRequests = new ListBuffer[Request]()
    var banks = (new Array[Object](numBanks)).map(_ => new BankState())
    var nextRequest: Long = 0
  }

  var channels = (new Array[Object](1 << channelBits)).map(_ => new ChannelState(1 << bankBits))

  def tick() {
    if (sim.now % clockDivider != 0) {
      return
    }
    channels.map { case chan => if (chan.nextRequest <= sim.now) bestRequest(chan) match {
      case Some(req) => issueRequest(req)
      case None => Unit
    }}
  }

  def receiveRequest(memRequest: MemRequest) {
    val req = new Request(memRequest)
    channels(req.channel).waitingRequests.append(req)
  }

  private def bestRequest(chan: ChannelState): Option[Request] = {
    // Filter out requests that can't be issued due to bank inter-request delay
    val issuableRequests = chan.waitingRequests.filter {
      case req => chan.banks(req.bank).nextRequest <= sim.now
    }
    val rowStates = issuableRequests.view.map(rowState)
    // Filter out row conflicts before tRAS
    val issuableRequests2 = issuableRequests.view.zip(rowStates).filter {
      case z => z match {
        case (req, RowState.Conflict) => chan.banks(req.bank).nextConflict <= sim.now
        case _ => true
      }
    }
    val bestReqAndState = issuableRequests2.foldLeft[Option[(Request, RowState)]](None) {
      case (Some(prev), next) => {
        val (_, prevState) = prev
        val (_, nextState) = next
        prevState match {
          case RowState.Hit => Some(prev)
          case _ => nextState match {
            case RowState.Hit => Some(next)
            case _ => Some(prev)
          }
        }
      }
      case (None, next) => Some(next)
    }
    bestReqAndState match {
      case Some((req, _)) => {
        chan.waitingRequests = chan.waitingRequests.filter { _ != req }
        Some(req)
      }
      case None => None
    }
  }

  private def issueRequest(req: Request) {
    val chan = channels(req.channel)
    val bank = chan.banks(req.bank)
    val state = rowState(req)
    var reqDelay: Long = 0
    chan.nextRequest = sim.now + tCCD * clockDivider
    if (state != RowState.Hit) {
      if (state == RowState.Conflict) {
        // Precharge
        reqDelay += tRP
      }
      // Activate
      bank.nextConflict = sim.now + (reqDelay + tRAS) * clockDivider
      reqDelay += tRCD
      bank.openRow = req.row
    }
    // Read/write
    reqDelay += tCCD
    bank.nextRequest = sim.now + reqDelay * clockDivider
    val dataDelay = reqDelay + tCL
    req.memRequest.respond(sim.now + dataDelay * clockDivider)
  }

  private def rowState(req: Request): RowState = {
    channels(req.channel).banks(req.bank).openRow match {
      case req.row => RowState.Hit
      case Dram.NoOpenRow => RowState.Miss
      case _ => RowState.Conflict
    }
  }
}

object Dram {
  val OffsetBits: Int = 6
  val RowSizeBits: Int = 13
  val NoOpenRow: Long = -1
}
