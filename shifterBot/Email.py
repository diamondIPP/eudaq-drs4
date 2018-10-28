#!/usr/bin/env python

from smtplib import SMTP_SSL
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from base64 import b64decode
from time import time
from utils import *


class Email:

    def __init__(self):
        self.MyAddress = 'micha.reichmann@gmail.com'
        self.MyUserName = 'micha.reichmann'
        self.Recipients = [self.MyAddress, 'sandiego@phys.ethz.ch', 'dhits@ethz.ch']
        self.Server = self.load_server()
        self.TStart = time()
        self.Reloads = 0

    @staticmethod
    def __get_pw():
        with open('xzdf') as f:
            return b64decode(f.readline())

    def prepare_message(self, subject, msg):
        message = MIMEMultipart()
        message['From'] = self.MyAddress
        message['To'] = ', '.join(self.Recipients)
        message['Subject'] = subject
        message.attach(MIMEText(msg, 'plain'))
        return message

    def load_server(self):
        server = SMTP_SSL('smtp.gmail.com', 465)
        server.ehlo()
        server.login(self.MyAddress, self.__get_pw())
        return server

    def send_message(self, subject, msg):
        text = self.prepare_message(subject, msg).as_string()
        self.Server.sendmail(self.MyUserName, self.Recipients, text)

    def reload_server(self):
        if (time() - self.TStart) / (55 * 60) - 1 > self.Reloads:
            info('reloading server ...\n')
            self.Server = self.load_server()
            self.Reloads += 1


if __name__ == '__main__':
    z = Email()
