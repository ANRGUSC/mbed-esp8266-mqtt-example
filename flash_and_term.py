#!/usr/bin/env python3
import sys, os, shutil
from pyOCD.board import MbedBoard
import logging
import serial.tools.list_ports_linux as serial_tools
import glob

def serial_ports(unique_id):
    ports = list(serial_tools.comports())
    for p in ports:
        if unique_id in p.hwid:
            return p.device

logging.basicConfig(level=logging.INFO)

#clean compile for LPC1768 board using GCC
os.system("mbed compile -c");

board = MbedBoard.chooseBoard()

target = board.target
flash = board.flash
target.resume()
target.halt()

filename = glob.glob('./BUILD/LPC1768/GCC_ARM/*.bin')

flash.flashBinary(filename[0])
print("pc: 0x%X" % target.readCoreRegister("pc"))

target.resume()
target.reset()

cmd = "./pyterm -b 115200 -p %s" % serial_ports(board.unique_id)

os.system(cmd)