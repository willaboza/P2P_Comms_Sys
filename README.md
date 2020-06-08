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

  8. Add backoff to re-transmissions of messages by adding a field to the pending table to avoid, or spread out, collisions. Backoff for re-transmissions is in units of milliseconds (BACKOFF = 0 means the message can be sent).
  
     If valid bit for message in pending table is TRUE and the backoff value is 0 this indicates that the message in the pending table is ready for transmission. 
    
     Once the checksum of a message has been sent, and the transmission attempts remaining field has been decremented (See Step 7 above), the new backoff value needs to be calculated. There are two ways in which the new backoff value is calculated.
     
       * The first is using a binomial backoff calculated using the formula *backoff = base # + 2<sup>(n-1)</sup>base #*. Binomial backoff is used when the random transmission command is OFF. An example of the expected backoff values after each attempt, using a base number of 500 ms, is shown in the table below.
  
         | TX Attempt | Backoff Value (s) |
         | :--------: | :---------------: |
         |      0     |          0        |
         |      1     |          1        |
         |      2     |          1.5      |
         |      3     |          2.5      |
  
       * When the random transmission command is ON then a randomized backoff transmission is calculated using the formula *backoff = base # + rand(2<sup>(n-1)</sup>base #)*. The random function is first seeded with the device address, which should be unique. An example of the expected backoff values after each attempt, using a base number of 500 ms, is shown in the table below.

         | TX Attempt | Backoff Value (s) |
         | :--------: | :---------------: |
         |      0     |     0 to 1        |
         |      1     |     0.5 to 1      |
         |      2     |     0.5 to 2.5    |
         |      3     |     0.5 to 4.5    |
         
     A 1 kHz timer is added to handle the transmission of valid messages, processed in tickIsr, when its backoff equals 0.

9. After checksum byte is sent flash the on-board red LED on for a period of time and then turn it off when elapsed.

10. Convert UART1 to 9-bit mode so when it is set the first byte, or destination address, of the message has been received. DE control on the RS-485 will need to be enabled while transmitting the packet and turned off once the last byte, or checksum, has been transmitted.

    UART1's Line Control register has sticky parity and requires:
        
       * Setting the PEN when initializing UART1.
       * When the PEN, EPS, and SPS bits are set the parity bit is transmitted as a 0.
       * When the PEN and SPS bits are set but the EPS bit is cleared then the parity bit is transmitted as a 1.
       
   This is used to synch the receiver.
   
11. Receiving of Data.

12. Taking Action After Data is Received.

13. Taking Action when a Data Request Message is Received.

14. Handle Reset Address Command.

15. Poll Requests and Responses.

16. On Receiving a Set Address Command Store New Address in EEPROM.

17. Add Blink Green LED Functionality when TX Data.

18. When a broadcast or unicast message is received with the ACK bit set then an ACK message will be sent back to the source address. 

19. When an ACK message is received take value in data field of packet and compare it with the sequence IDs of those messages in the devices pending table. If a match is found then mark the valid bit for that message as FALSE.

20. If re-transmission time expires then need to re-transmit message, decrement transmission attempts remaining, and calculate next transmission time of message.
