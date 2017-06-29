import threading
import time
import serial
import json

connected = False
port = '/dev/ttyACM0'
baud = 9600


def handle_data(data):
    print(data)

def write_to_port(ser):
    while True:
        time.sleep(11)
        #ser.write(b'testing\n')
def read_from_port(ser):
    while True:
        reading = ser.readline().decode()
        reading = reading[:-2]
        try:
            print(json.loads(json.dumps(reading)))
        except json.decoder.JSONDecodeError:
            print(reading)
            print('json converting error.')

serial_port = serial.Serial(port, baud, timeout=10)
thread_write = threading.Thread(target=write_to_port, args=(serial_port,))
thread_read = threading.Thread(target=read_from_port, args=(serial_port,))
thread_write.start()
thread_read.start()

