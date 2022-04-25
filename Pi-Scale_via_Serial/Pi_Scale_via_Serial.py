import serial
import time

TorRey = serial.Serial('/dev/ttyACM0',115200, timeout=1)
M5Stack = serial.Serial('/dev/ttyUSB0',115200)

while True:
    TorRey.write(b'P')
    while TorRey.inWaiting()==0: pass
    if TorRey.inWaiting()>0:
        answer = TorRey.readline()
        print("Tor-Rey answer: ")
        print(answer.decode('utf-8'))
        TorRey.flushInput()
        weight = float(answer[1:6].decode('utf-8'))
        print("Extracted value: ")
        print(weight)
    msg = "P" + str(weight)
    print("Message to M5Stack: ")
    print(msg)
    M5Stack.write(msg.encode('utf-8'))
    time.sleep(10)
ser.close()
