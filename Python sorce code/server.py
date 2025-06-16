# first of all import the used librarys 
import socket # should already been included in python 3
import struct # should already been included in python 3
from modbus_crc import add_crc # if not installed, use: py pip install modbus-crc

#  === add functions ===

# Function: parse RTU float response
def parse_floats_from_response(response):
    if len(response) == 13 and response[1] == 0x04 and response[2] == 0x08:
        float_bytes = response[3:11]
        try:
            value1 = struct.unpack('>f', float_bytes[0:4])[0] # big-endian floating-point
            value2 = struct.unpack('>f', float_bytes[4:8])[0] 
            print(f"[0x04] Temperature : {value1:.6f}") # print temperature in degrees Celcius
            print(f"[0x04] pH value: {value2:.6f}") # print pH value
        except struct.error as e:
            print("Error unpacking floats:", e)
    else:
        print("Unexpected or malformed 0x04 response.")

# Function: parse ASCII device info from 0x2B response
def parse_ascii_from_2b_response(response):
    # Check that the response is long enough to include a valid header
    if len(response) < 9:
        print("Response too short to be valid for 0x2B.")
        return
    
    # Verify that the response corresponds to the correct function and MEI type
    if response[1] != 0x2B or response[2] != 0x0E:
        print("Not a valid MEI (0x2B/0x0E) response.")
        return

    # Extract and label the key header fields for clarity and debugging
    mei_type = response[2]          # MEI Type (should be 0x0E)
    read_dev_id_code = response[3]  # Read Device ID Code
    conformity_level = response[4]  # Conformity Level (level of supported object types)
    more_follows = response[5]      # Flag indicating if more data follows
    next_obj_id = response[6]       # ID of the next object to be read (if any)
    num_objects = response[7]       # Number of objects included in this response

    # Print header field information for debugging and traceability
    print(f"[0x2B] MEI Type: {mei_type}")
    print(f"[0x2B] Read Device ID Code: {read_dev_id_code}")
    print(f"[0x2B] Conformity Level: {conformity_level}")
    print(f"[0x2B] More Follows: {more_follows}")
    print(f"[0x2B] Next Object ID: {next_obj_id}")
    print(f"[0x2B] Number of Objects: {num_objects}")

    # Start reading object data from the 9th byte (index 8)
    index = 8
    for obj in range(num_objects):
        # Ensure there are enough bytes remaining for object ID and length
        if index + 2 > len(response):
            print("Incomplete object header.")
            return
        
        # Read the object ID and length of the associated data
        obj_id = response[index]
        obj_len = response[index + 1]
        index += 2

        # Verify that the full object data is available
        if index + obj_len > len(response):
            print("Incomplete object data.")
            return
        
        # Extract the object value bytes
        obj_value_bytes = response[index:index + obj_len]

        # Attempt to decode the object value as ASCII, fallback to hex on error
        try:
            obj_value = obj_value_bytes.decode('ascii')
        except UnicodeDecodeError:
            obj_value = obj_value_bytes.hex()  
        print(f"Object ID {obj_id}: {obj_value}") # prints Object ID's in terminal
        index += obj_len # Move the index forward to the next object

# Function: get sensor data from SA11
def send_sensor_data_request(client_socket):
    print("Get sensor data from SA11...")
    request = add_crc(b'\x01\x04\x00\x04\x00\x04') # read 4 input regusters for Temperature and main sensor value
    client_socket.sendall(request) # Send the RTU frame to the connected client
    
    response = client_socket.recv(256) # Recive the RTU resoponse over Ethernet
    print("Received RTU response (hex):", response.hex())
    print("Parsed:", parse_floats_from_response(response)) # Parse floating-point values

# Function: get basic SA11 device info (includes product code)
def send_basic_device_id(client_socket):
    print("Sending basic device info request...")
    request = add_crc(b'\x01\x2B\x0E\x01\x00') # basic ID info (code 0x01), Object = 0
    client_socket.sendall(request)

    response = client_socket.recv(256)
    print("Received:", response.hex())
    print("Parsed:", parse_ascii_from_2b_response(response)) # Parse ASCII values

# Fucntion: get extended SA11 device info (includes serial number)
def send_extended_device_id_request(client_socket, max_object_id = 0x128):
            start_id = 0x00 # start where the extended info is expected
            while start_id <= max_object_id:
                request = add_crc(bytes([0x01, 0x2B, 0x0E, 0x03, start_id])) # extended ID info (code 0x03)
                print(f"Sendiding 0x2B (Extended) request starting at Object ID {start_id}")
                client_socket.sendall(request)

                response =  client_socket.recv(256)
                print("Received:", response.hex())
                print("Parsed:", parse_ascii_from_2b_response(response)) # Parse ASCII values

                # Stop if no more objects are expected
                more_follow = response[5]
                next_id = response[6]
                if more_follow == 0 or next_id <= start_id:
                    break
                start_id = next_id

#  === Start main flow ===

# Create a TCP server socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # open socket for TCP using IPV4
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
print ("Socket successfully created")

host = ''   # Listen to all interface
port = 8888 # local port 

server_socket.bind((host, port))         
server_socket.listen(5) # put the socket into listening mode     
print (f"TCP RTU server listening on port {port}...")            

# a forever loop until we interrupt it or an error occurs 
while True: 
    # Accept client conncetion
    client_socket, addr = server_socket.accept()
    print(f"Connection accepted form {addr}")

    try:
        # Send 0x2B (basic SA11 info)
        send_basic_device_id(client_socket) # send and recieve function for function code 2B, basic info

        # Send 0x2B (extended SA11 info)
        send_extended_device_id_request(client_socket, max_object_id = 0x128) # send and recieve function for function code 2B, extended info

        # Send 0x04 (sensor data)
        send_sensor_data_request(client_socket) # send and receive sensor temperature and main sensor value

    except Exception as e:
        print(f"Error during client handling: {e}")
    
    finally:
        client_socket.close() # close connection with client
        print("Connection closed.\n")

    # Optional: break after one client for testing
    break # comment comment if testing is done

server_socket.close() # uncomment if you use a break above
