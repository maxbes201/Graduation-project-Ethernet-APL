# first of all import the used librarys 
import socket # should already been included in python 3
import serial # # if not installed, use: py pip install pyserial

# Serial port setup over RS-485
serial_port = serial.Serial('COM5') # conncet on COM5
serial_port.baudrate = 9600 # set baudrate to 9600
serial_port.bytesize = 8 # total set bytes
serial_port.parity = 'E' # parity bit
serial_port.stopbits = 1 # number of stop bits
serial_port.timeout = 1.0 # timeout for Modbus device response

# TCP client setup
server_ip = '127.0.0.1' # local loopback IP address
server_port = 8888 # server port to conncet to

#connect to Modbus RTU over TCP server
s = socket.socket() # open TCP socket
s.connect((server_ip, server_port)) # connect to TCP server
print(f"Connected to server at {server_ip}:{server_port}")

try:
    for i in range(3): # Expecting 3 requests
        # Step 1: Revieve RTU request frame from TCP server
        rtu_request = s.recv(256) # recive Ethernet messeage from TCP server and unpack it
        if not rtu_request:
            print("No RTU request recieved from server")
            break

        function_code = rtu_request[1]
        print(f"\nRecieved RTU request form server (Fucntion code {function_code:#04x}): {rtu_request}")

        # Step 2: Forward RTU request over RS-485
        serial_port.reset_input_buffer()
        serial_port.write(rtu_request) # send Modbus RTU frame to connected SA11
        print("Sent RTU request to SA11")

        # Step 3: Read RTU response form SA11
        rtu_response = serial_port.read(256) # read RTU response meassage from SA11
        if rtu_response:
            print(f"Recieved RTU response form SA11 Fucntion code {function_code:#04x}): {rtu_response}")
        else:
            print("No response recieved from SA11")

        # Step 4: Send RTU response form SA11 back to the TCP server
        s.sendall(rtu_response) # Wrap RTU frame in Ethernet packet and sent to TCP server
        print("Sent RTU response back to TCP server")

finally:
    # Clean up
    s.close()
    serial_port.close()
    print("Close TCP and serial conncetion")