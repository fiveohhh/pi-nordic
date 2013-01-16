from quick2wire.spi import *
from quick2wire.gpio import Pin
import select
import binascii
import time

radio_irq_pin = Pin(13, Pin.In, Pin.Falling)

def main():
    # Initialize device 0 on bus zero
    radio = SPIDevice(0,0)
    
    # set data rate
    radio.transaction(writing([0x26, 0x06]))
    
    # reset/clear
    radio.transaction(writing([0x20, 0x70]))
    
    # Flush buffers
    radio.transaction(writing([0xE1]))
    radio.transaction(writing([0xE2]))

    # Enable Shockburst on all channels
    radio.transaction(writing([0x21, 0x7f]))
    
    # Set pipe 1 rx address
    radio.transaction(writing([0x22, 0x03]))
    radio.transaction(writing([0x2B,0xE7, 0xE7, 0xE7, 0xE7, 0xE7]))

    # Enable pipe 1
    radio.transaction(writing([0x22, 0x02]))

    # Enable dynamic payloads
    radio.transaction(writing([0x3D, 0x04]))
    # On pipe 1
    radio.transaction(writing([0x3C, 0x02]))
    
    # Power up and set to RX
    radio.transaction(writing([0x20, 0x0B]))

    # Wait for crystal (really only needs to be ~ 4ms
    time.sleep(1)

    # Reset/clear
    radio.transaction(writing([0x27, 0x70]))
    
    # Flush buffers
    radio.transaction(writing([0xE1]))
    radio.transaction(writing([0xE2]))
    
    
    epoll = select.epoll()
    epoll.register(radio_irq_pin, select.EPOLLIN | select.EPOLLET)
    while True:
        events = epoll.poll()
        for fileno, event in events:
            if fileno == radio_irq_pin.fileno():
                # while(not(status & (1 << 6))):
                status = radio.transaction(duplex([0xFF]))[0][0]
                pipe = status >> 1 & 0x07
                print("Data arrived on pipe: " + str (pipe))
                res = radio.transaction(duplex([0x60, 0xFF]))
                length = radio.transaction(duplex([0x60, 0xFF]))[0][1]
                print("Length: " + str(length))
                txData = [0x61]
                for i in range(length + 1):
                    txData.append(0xff)
                print("Data: " + str(binascii.hexlify(radio.transaction(duplex(txData))[0][1:length+1])))
                radio.transaction(writing([0x27, 0x70]))
                radio.transaction(writing([0xE1]))
                radio.transaction(writing([0xE2]))




if __name__ == "__main__":
    main()

