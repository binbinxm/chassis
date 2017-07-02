import sys
import pprint
import time
import serial
import json
import ino
import setting
import log
from apscheduler.schedulers.blocking import BlockingScheduler
import requests

def upload():
    tmp = dict(ino_env.report)
    tmp['timestamp'] = int(time.time())
    r = requests.post(setting.URL, json=tmp)
    logger.info('server response: ' + str(r.status_code))

if __name__ == '__main__':
    logger = log.setup_custom_logger(__name__)
    logger.info('program starting...')
    with ino.env() as ino_env:
        ino_env.run()
        sched = BlockingScheduler()
        sched.add_job(upload, 'interval', seconds=setting.UPLOAD_INTERVAL)
        sched.start()
     
         #while True:
         #   try:
         #       while(True):
         #           time.sleep(3)
         #   except (KeyboardInterrupt, SystemExit):
         #       sys.exit()
            



