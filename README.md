# P2P_Comms_Sys

Overview

  A peer-to-peer communication system is implemented by building nodes capable of interfacing with a PC, using TeraTerm, to receive text commands from a user. The entered commands will send transmissions, on a 2-wire RS-485 Bus, between each node.
  
  When a node receives either a broadcast transmission or one with a matching destination address for the device it will then extract information from the asynchronous data stream. An acknowledgment will be sent back to the source address if one was requested to notify them of successful receipt of the data packet.
  
Project Steps

  Step 1:\r\n 
  \tOn power up toggle on-board green LED on and off
  
  Step 2-3:
  Setup UART0 Tx and Rx to interface with a PC through TeraTerm.
  
  Step 4-5: 
  Use UART1 Tx and Rx to send data between nodes over the RS-485.
  
  Step 6: 
  Build a transmit table for queuing messages to be sent over RS-485. The transmit table contains fields for
  the destination address, sequence number, command, channel, size of data, data, checksum, transmissions to
  go, and a valid bit. (Note: Set valid bit as true only after all other fields for entry in transmit table
  have been filled.)
  
  For each message to transmit an open row in the transmit table, indicated by a valid bit value of 0, will 
  be found and then populated.
  
  Step 7:
  Once the valid bit is set to True the data packet will be marked for transmission over the RS-485 bus. A
  message is transmitted by sending a byte at a time. The first byte will be written to the UART1_DATA_REG
  to "prime the pump". Once sent, the next byte to be transmitted will wait for an interrupt on the UART1
  Transmit FIFO to send.
