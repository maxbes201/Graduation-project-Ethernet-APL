# STM32 Modbus RTU over TCP Bridge

This project implements a **Modbus RTU over TCP bridge** using an **STM32F407 Discovery Board**. It acts as a TCP client that receives Modbus TCP messages, forwards them over RS485 as Modbus RTU, and returns the RTU response back over TCP.

---

>  **University:** Hogeschool Utrecht  
>  **Project Date:** May 2025  
>  **Author:** Max Besseling  

---

##  Overview

-  **TCP Client:** Connects to a predefined Modbus TCP server.
-  **RS485 UART Bridge:** Translates TCP payload into RTU and vice versa.
-  **Reliable Timing:** Supports strict timing constraints of Modbus RTU (inter-byte and request spacing).
-  **Custom RX Polling:** Implements byte-accurate receive loop with robust timeout handling.
-  **Built on LwIP RAW API:** Low-latency and event-driven Ethernet stack.
-  **MII Ethernet:** Via PHY with hardware reset support.

---

##  STM32 Configuration Summary

| Peripheral     | Settings                                          |
|----------------|---------------------------------------------------|
| UART2 (RS485)  | `9600 baud`, `9-bit`, `Even parity`, `1 stop bit` |
| UART3 (Debug)  | `115200 baud`, `8-bit`, `No parity`               |
| ETH (LwIP)     | MII via Ethernet PHY, static IP (RAW API)         |
| RS485 GPIOs    | PC9 = DE, PC8 = RE (Active High for TX)           |
| Word Length    | `UART_WORDLENGTH_9B` (required for even parity)   |

---

##  System Behavior

### 1. TCP Connection

Upon detecting Ethernet link-up, the STM32 connects as a TCP client

### 2. Message Handling

- A `tcp_recv()` callback receives a Modbus PDU over TCP.
- The payload is forwarded via RS485 using `Modbus_send()`.
- The RTU response is captured using a custom polling loop with **inter-byte timeout**.
- The response is then sent back to the TCP server using `tcp_write()`.

### 3. RS485 Transceiver Control

Transceiver direction is managed via GPIO:

RS485_SetTransmit(); // PC9 & PC8 HIGH
RS485_SetReceive();  // PC9 & PC8 LOW
