#!/usr/bin/env python

from smtplib import SMTP_SSL
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from base64 import b64decode
from utils import *
from os.path import dirname, realpath, join


class Email:

    def __init__(self):
        self.Dir = dirname(realpath(__file__))

        self.MyAddress = 'micha.reichmann@gmail.com'
        self.MyUserName = 'micha.reichmann'
        self.Recipients = [self.MyAddress, 'sandiego@phys.ethz.ch', 'dhits@ethz.ch', 'kagan.1@osu.edu']
        self.Server = self.load_server()

    def __get_pw(self):
        with open(join(self.Dir, 'xzdf')) as f:
            return b64decode(f.readline())

    def prepare_message(self, subject, msg):
        message = MIMEMultipart()
        message['From'] = self.MyAddress
        message['To'] = ', '.join(self.Recipients)
        message['Subject'] = subject
        message.attach(MIMEText(make_html(msg), 'html'))
        return message

    def load_server(self):
        server = SMTP_SSL('smtp.gmail.com', 465)
        server.ehlo()
        server.login(self.MyAddress, self.__get_pw())  # maybe activate less secure apps in email acccount
        return server

    def send_message(self, subject, msg):
        self.reload_server()
        text = self.prepare_message(subject, msg).as_string()
        self.Server.sendmail(self.MyUserName, self.Recipients, text)

    def reload_server(self):
        info('reloading mail server ...\n')
        self.Server = self.load_server()


if __name__ == '__main__':
    z = Email()
