import time
import sys
import threading
import json
import serial
import log
import setting

class env(object):
    def __init__(
            self,
            port=setting.PORT,
            baud=setting.BAUD,
            timeout=setting.TIMEOUT):
        self.ser = serial.Serial(port, baud, timeout=timeout)
        self.logger = log.setup_custom_logger(__name__)
        self.report = None
        self.fan0 = pid(1,1,1)
        self.fan1 = pid(1,1,1)
        self.fan2 = pid(1,1,1)
        self.run_thread = threading.Thread(target=self.run_loop, args=())
        self.logger.info('env class initialized.')

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.logger.info('env class destoried toghther with thread.')

    def decode(self, str):
        try:
            str = str[:-2]
            str = str.replace("'", "\"")
            str = json.loads(str)
            return str
        except ValueError:
            self.logger.error('Exception ValueError: ' + str)
            return None

    def get_report(self, print = True):
        #self.ser.write(b'get\n')
        tmp = self.decode(self.ser.readline().decode())
        if tmp == None:
            self.logger.error(tmp)
            return tmp
        elif not print: return tmp
        else:
            self.logger.info('room temp: %2.1fC, humidity: %2.1f%%, stamp:%d'\
                    %(tmp['temperature'], tmp['humidity'], tmp['timestamp']))
            self.logger.info('slot 0: %2.1fC, slot 1: %2.1fC, slot 2: %2.1fC'\
                    %(tmp['temp'][0] ,tmp['temp'][1], tmp['temp'][2]))
            self.logger.info('self protect: %s' % tmp['protect'])
            self.logger.debug('fan 0: %2x, fan1: %2x, fan2: %2x'\
                    %(tmp['set'][0], tmp['set'][1], tmp['set'][2]))
            return tmp

    def run(self):
        self.run_thread.start()

    def run_loop(self):
        while(True):
            self.report = self.get_report()
            if(self.report != None):
                pid0 = self.fan0.gen_u(\
                        (setting.TEMPERATURE_TARGET\
                        + self.report['temperature']\
                        - self.report['temp'][0])\
                        / (setting.TEMPERATURE_TARGET\
                        + self.report['temperature']))
                pid1 = self.fan1.gen_u(\
                        (setting.TEMPERATURE_TARGET\
                        + self.report['temperature']\
                        - self.report['temp'][1])\
                        / (setting.TEMPERATURE_TARGET\
                        + self.report['temperature']))
                pid2 = self.fan2.gen_u(\
                        (setting.TEMPERATURE_TARGET\
                        + self.report['temperature']\
                        - self.report['temp'][2])\
                        / (setting.TEMPERATURE_TARGET\
                        + self.report['temperature']))
                pid0 = self.normalization(pid0)
                pid1 = self.normalization(pid1)
                pid2 = self.normalization(pid2)
                ctrl = bytes('set ' + pid0+pid1+pid2 + '\n', encoding = "utf8")
                self.ser.write(ctrl)
                tmp = self.decode(self.ser.readline().decode())
            else:
                self.logger.error('self.report == None')

    def normalization(self, num):
        num = -1 * num
        if num > 1:
            num = 1
        s = '%02x'%(num * (0xff - 0x60) + 0x60)
        if num < 0:
            s = '00'
        return s


class pid(object):
    def __init__(self, kp=setting.KP, ki=setting.KI, kd=setting.KD):
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.e = [0,0,0]
        self.u = 0

    def gen_u(self, delta):
        self.e.insert(0, delta)
        self.e = self.e[:3]
        self.u = self.u\
                + self.kp * (self.e[0] - self.e[1])\
                + self.ki * self.e[0]\
                + self.kd * (self.e[0] - 2 * self.e[1] + self.e[2])
        return self.u

