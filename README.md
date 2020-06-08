# Peer-to-Peer Communication System

  A peer-to-peer communication system is implemented by building nodes capable of interfacing with a PC, using TeraTerm, to receive text commands from a user. The entered commands will send transmissions, on a 2-wire RS-485 Bus, between each node.
  
  When a node receives either a broadcast transmission or one with a matching destination address for the device it will then extract information from the asynchronous data stream. An acknowledgment will be sent back to the source address if one was requested to notify them of successful receipt of the data packet.
  
## Project Steps:

  1. On power up toggle on-board green LED on and off.
  
  2. Setup UART0 TX to interface with a PC using TeraTerm (Non-Blocking).
  
  3. Setup UART0 Rx to interface with a PC using TeraTerm (Non-Blocking).
  
  4. Use UART1 Tx to send data between nodes over the RS-485 (Non-Blocking).
  
  5. Use UART1 Rx to send data between nodes over the RS-485 (Non-Blocking).
  
  6. Build a transmit table for queuing messages to be sent over RS-485. The transmit table contains fields for the destination address, sequence number, command, channel, size of data, data, checksum, transmissions to go, and a valid bit. ***(Note: Set valid bit as TRUE only after all other fields for entry in transmit table have been filled.)***
  
     For each message to transmit an open row in the transmit table, indicated by a valid bit value of 0, will be found and then populated.
  
  7. Once the valid bit is set to True the data packet will be marked for transmission over the RS-485 bus. A message is transmitted by sending a byte at a time. The first byte will be written to the UART1_DATA_REG to "prime the pump" and the current transmit phase will be incremented by 1. Once sent, the next byte to be transmitted will wait for an interrupt on the UART1 Transmit FIFO to send.
  
     Once an interrupt has occured on UART1 then the function uart1Isr is called which does the following.
  
       * Check if UART1 TX FIFO is empty, then transmit next byte in message. 
       * Increment transmit phase by 1.
       * If transmitting checksum set transmit phase to 0, and decrement the number of transmission attempts   remaining for the data packet. 
       * If the number of transmission attempts equals 0 after decrementing, and an acknowledgment is required, set valid bit to FALSE for removal of message from queue. Next, display a message for user to alert them of failure to transmit the message. 
       * If the number of transmission attempts equals 0 after decrementing, and ***NO*** acknowledgment is needed, then set valid bit to FALSE so it can be removed from the queue.
       
     The order of transmitting each byte of the data packet is shown in the table below.
     
     | Phase # | Byte to TX |
     | :-----: | :--------: |
     |    0    | Destination Address|
     |    1    | Source Address |
     |    2    | Sequence ID |
     |    3    | Command |
     |    4    | Channel |
     |    5    | Size |
     | 6+(n-1) | Data |
     |   6+n   | Checksum |

  8. 
