# Serial Communication Documentation

## Overview

This document describes the serial communication protocols and pin connections used in the ESP32 Fingerprint Door Lock project. The system uses multiple serial interfaces to communicate with different modules:

1. Fingerprint module communication
2. ASRPRO voice module communication
3. Debug serial output

## Pin Configuration

### ESP32 to External Modules Pin Mapping

| ESP32 Pin | Connected To         | Function                        |
|-----------|----------------------|---------------------------------|
| GPIO3 (RX) | ASRPRO TX           | Receive data from ASRPRO module |
| GPIO1 (TX) | ASRPRO RX           | Send data to ASRPRO module      |
| GPIO16 (RX2)| Fingerprint TX      | Receive data from fingerprint module |
| GPIO17 (TX2)| Fingerprint RX      | Send data to fingerprint module |
| GPIO13    | Open Servo Signal    | Control signal for open servo   |
| GPIO14    | Close Servo Signal   | Control signal for close servo  |
| GPIO18    | Fingerprint Button   | Trigger fingerprint matching    |
| GPIO19    | Open Button          | Trigger open servo action       |
| GPIO21    | Close Button         | Trigger close servo action      |
| GPIO22    | Enroll Button        | Trigger fingerprint enrollment  |

### Module Details

1. **ASRPRO Voice Module**: Used for voice prompts and audio feedback
2. **Fingerprint Module**: 5-pin module with 3 pins connected to ESP32 3.3V power, and RX/TX for communication
3. **Servo Motors**: Two servo motors for physical locking/unlocking mechanism
4. **Buttons**: Four buttons for manual control and triggering functions

## Serial Port Configuration

### Fingerprint Module Serial
- **Baud Rate**: 115200
- **Data Format**: SERIAL_8N1 (8 data bits, No parity, 1 stop bit)
- **Protocol**: Custom 8-byte binary protocol
- **Usage**: Bi-directional communication for fingerprint operations

### ASRPRO Voice Module Serial
- **Baud Rate**: 115200
- **Data Format**: SERIAL_8N1 (8 data bits, No parity, 1 stop bit)
- **Protocol**: Text-based commands
- **Usage**: Sending voice prompts and receiving voice commands

### Debug Serial
- **Baud Rate**: 115200
- **Data Format**: SERIAL_8N1 (8 data bits, No parity, 1 stop bit)
- **Protocol**: Text output
- **Usage**: System debugging and status information

## Fingerprint Module Protocol

### Command Structure
All commands follow an 8-byte format:
```
F5 TYPE P1 P2 P3 0 CHK F5
```

Where:
- **F5**: Start and end byte
- **TYPE**: Command type
- **P1-P3**: Parameters
- **CHK**: Checksum (XOR of bytes 1-5)
- **0**: Reserved byte

### Response Structure
Responses follow the same 8-byte format:
```
F5 TYPE Q1 Q2 Q3 0 CHK F5
```

Where:
- **Q1-Q3**: Response data

### Common Commands

| Command             | Type | P1 | P2 | P3 | Description              |
|---------------------|------|----|----|----|--------------------------|
| Wakeup              | 0x2C | 0  | 0  | 1  | Wake up fingerprint module |
| 1:N Match           | 0x0C | 0  | 0  | 0  | Match fingerprint        |
| Capture 1st         | 0x01 | 0  | 0  | 1  | First fingerprint capture |
| Capture 2nd         | 0x02 | 0  | 0  | 0  | Second fingerprint capture |
| Capture 3rd         | 0x03 | 0  | 0  | 0  | Third fingerprint capture |
| Get User Count      | 0x09 | 0  | 0  | 0  | Get total user count     |
| Handshake 1         | 0xFE | 0  | 0  | 0  | Handshake command 1      |
| Handshake 2         | 0xFD | 0  | 0  | 0  | Handshake command 2      |

### Response Parsing for 1:N Matching
- **TYPE**: 0x0C
- **User ID**: Composed of Q1 (high 8 bits) and Q2 (low 8 bits)
- **Success Condition**: Q1/Q2 not all zero AND Q3 ≠ 0x08
- **Permission Info**: Stored in Q3 field

## ASRPRO Module Communication

### Command Format
- Text-based commands without newline characters
- Commands are sent using `asrproSerial.print()` instead of `println()`

### Common Commands
| Command       | Description             |
|---------------|-------------------------|
| open_door     | Trigger door opening    |
| close_door    | Trigger door closing    |
| match         | Start fingerprint match |
| register      | Start fingerprint registration |
| success       | Match success prompt    |
| error         | Error prompt            |
| final_error   | Final error prompt      |
| success_tm    | Registration success    |

## Hardware Considerations

1. **UART0** (GPIO1/TX, GPIO3/RX) is reserved for debug output
2. Other device communications use non-critical GPIO pins
3. Multiple serial communications must not be configured on the same physical pins
4. Pin usage should be clearly commented in the code
5. Serial buffer should be cleared before sending commands to avoid interference
6. Timeout mechanisms should be implemented to prevent infinite waiting