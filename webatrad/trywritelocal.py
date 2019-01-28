#!/Program Files (x86)/Python37-32/python
# -*- coding: UTF-8 -*-# enable debugging
import cgi, cgitb
import time
import datetime
import glob
from time import strftime

import os
import urllib



# Create instance of FieldStorage 
form = cgi.FieldStorage() 
# Get data from client
temp = form.getvalue('temp') 





cgitb.enable()    
print("Content-Type: text/html;charset=utf-8")
print()    
#print("Hello World!")

"""import sys
print(sys.version)"""




import pymysql
conn = pymysql.connect(host='localhost', port=3306, user='root', passwd='gfajardo', db='prueba1roj')
cur = conn.cursor()
cur.execute("SELECT * FROM `temp at interrupt prueba1`")

#print(cur.description)
print()

#for row in cur:
#    print(row)


 # Agregar una fruta en nuestra base de datos:
#Date='2019-01-25'
#Time='21:18:13'
#Value='27'

# Ejecutar sentencia:

dateWrite = time.strftime('%Y-%m-%d')
timeWrite = time.strftime('%H:%M:%S')
temperature='23'
temperature=temp


print('Dato escrito correctamente>>')

print(dateWrite)
print(timeWrite)
print(temperature)



print(type(dateWrite))
print(type(timeWrite))
print(type(temperature))




cur.execute("INSERT INTO `temp at interrupt prueba1` (`Date`, `Time`, `Temperature`) VALUES (%s,%s,%s)",(dateWrite,timeWrite,temperature))
# Guardar los cambios.
conn.commit()



# Cerrar cursor
cur.close()

# Cerrar conexion
conn.close()