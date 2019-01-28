#!/Program Files (x86)/Python37-32/python
# -*- coding: UTF-8 -*-# enable debugging
import cgitb
import time
import datetime
import glob
from time import strftime
import pandas as pd
import numpy as np

cgitb.enable()    
print("Content-Type: text/html;charset=utf-8")
print()    
print("Hello World!")

"""import sys
print(sys.version)"""



import pymysql
conn = pymysql.connect(host='localhost', port=3306, user='root', passwd='gfajardo', db='prueba1roj')
cur = conn.cursor()
cur.execute("SELECT * FROM `temp at interrupt prueba1`")

print(cur.description)
print()

#for row in cur:
#    print(row)


 # Agregar una fruta en nuestra base de datos:
#Date='2019-01-25'
#Time='21:18:13'
#Value='27'

# Ejecutar sentencia:






colnames = ['Power','Temperature']
data = pd.read_csv('example.csv', names=colnames)
df=pd.DataFrame(data)

i=df.shape[0] #Obtiene el número de filas,
# si se cambia por 1 se obtiene el número de columnas

#print(i)

element=df.iloc[i-1] #obtiene propiedades de df
#print(element)
Powerfinal=element.Power
Temperaturafinal=element.Temperature
print(Powerfinal)
print(Temperaturafinal)
#import sys
#print(sys.version)





dateWrite = time.strftime('%Y-%m-%d')
timeWrite = time.strftime('%H:%M:%S')
temperature=Temperaturafinal




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