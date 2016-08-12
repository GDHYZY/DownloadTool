# coding=utf-8

import sys
import re
from ftplib import FTP

def ftpsize(hostname,path,user,paswd):

    try:
        ftp = FTP(hostname)
    except:
        print ('ftp get hostname error!')
        return -1

    size = -1
    try:
        ftp.login(user,paswd)
        print (ftp.getwelcome())
        #set binary mode
        ftp.voidcmd('TYPE I')
        size = ftp.size(path)
        ftp.quit()
    except:
        print ('can not login anonymously or connect error or function exec error!')
        ftp.quit()
    print(size)
    return size
