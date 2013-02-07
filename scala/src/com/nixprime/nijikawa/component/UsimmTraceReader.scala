package com.nixprime.nijikawa.component

import com.nixprime.nijikawa.common.Address
import java.io.{BufferedReader, FileReader}

class UsimmTraceReader(private val filename: String) extends TraceReader {
  private val fileReader = new FileReader(filename)
  private val fileLineReader = new BufferedReader(fileReader)

  def next(): Option[TraceRecord] = {
    Option(fileLineReader.readLine()) match {
      case Some(line) => {
        line.split(' ') match {
          case Array(prec, dir, addr, pc) => Some(newTraceRecord(dir, addr, prec))
          case Array(prec, dir, addr) => Some(newTraceRecord(dir, addr, prec))
          case _ => None
        }
      }
      case None => None
    }
  }

  private def newTraceRecord(dir: String, addrStr: String, precStr: String): TraceRecord = {
    new TraceRecord(new Address(hexToLong(addrStr)), precStr.toLong, dir match {
      case "R" => false
      case "W" => true
      case _ => throw new Exception("Unknown memory instruction type " + dir)
    })
  }

  private def hexToLong(str: String): Long = {
    // Drop the first two characters ("0x")
    java.lang.Long.parseLong(str.substring(2), 16)
  }
}
