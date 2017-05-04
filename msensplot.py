import serial
from tkinter import *
import math

port = 'COM30'
baud = 115200
sensor_resolution = 18
canv_dimension = sensor_resolution*10
pixel_count = sensor_resolution * sensor_resolution

ser = serial.Serial(port,baud)
print(ser.name)

def getX(index):
    return index // sensor_resolution

def getY(index):
    return index - getX(index) * sensor_resolution

def getPicture():
    a = []
    v = 0
    pv = 0
    c = 0
    i = 0

    while not (v == pv == 255):
        c = ser.read(1)
        v = int.from_bytes(c, byteorder='big')
        pv = v

    ser.read(1)

    while (i<pixel_count):
        c = ser.read(1)
        v = int.from_bytes(c, byteorder='big')
        val = {'index' : i, 'x' : getX(i), 'y' : getY(i), 'value' : v}
        a.append(val)
        i = i + 1

    return a


master = Tk()

w = Canvas(master, width = canv_dimension,height = canv_dimension)
w.pack()


def draw():
    for zi in getPicture():
        cval = zi['value'] - 1;
        cv = math.floor(cval/63 * 255)
        col = '{:02X}'.format(cv)
        color = '#'+col.format('%02x')+col.format('%02x')+col.format('%02x')
        #print(color)
        w.create_rectangle(zi['x'] * 10, zi['y'] * 10, (zi['x'] + 1) * 10, (zi['y'] + 1) * 10, fill=color, width=0)
    master.after(100,draw)
    pass

master.after(100,draw)

mainloop()
ser.close()

