#!/usr/bin/env python

from smtplib import SMTP_SSL
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from base64 import b64decode


class Email:

    def __init__(self):
        self.MyAddress = 'micha.reichmann@gmail.com'
        self.MyUserName = 'micha.reichmann'
        self.Recipients = [self.MyAddress, 'remichae@phys.ethz.ch', 'sandiego@phys.ethz.ch', 'dhits@ethz.ch']
        self.Server = self.load_server()

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


if __name__ == '__main__':
    z = Email()
