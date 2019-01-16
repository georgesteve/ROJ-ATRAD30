#!/Program Files (x86)/Microsoft Visual Studio/Shared/Python36_64/python
# -*- coding: UTF-8 -*-# enable debugging



# Intentaba compilar este código desde el localhost y aparecía el siguiente error_
#Recordar ubicación de python librerías C:\Users\gfajardo\AppData\Roaming\Python


import cgitb


cgitb.enable()    
print("Content-Type: text/html;charset=utf-8")
print()    
print("Hello World!")

import sys
print(sys.version)


import pymysql
conn = pymysql.connect(host='localhost', port=3306, user='root', passwd='gfajardo', db='prueba1roj')
