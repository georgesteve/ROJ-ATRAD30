#!/usr/bin/env python



import time
import datetime
import glob
from time import strftime
import pandas as pd
import numpy as np
import mysql.connector as mariadb



def ejecutaScript():
    
        print ('Ejecutando Script...')


        conn = mariadb.connect(host='localhost', port=3306, user='pi', password='raspberry', db='Prueba1ROJ')
        cur=conn.cursor()
        #cur.execute("SELECT * FROM `temp at interrupt prueba1`")


        #print(cur.description)
        #print()



        colnames = ['Power','Temperature']
        data = pd.read_csv('example.csv', names=colnames)
        df=pd.DataFrame(data)

        i=df.shape
        print(i[0])

        element=df.iloc[i[0]-1] #obtiene propiedades de df
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


        cur.execute("INSERT INTO `temp at interrupt prueba1` (`Date`, `Time`, `Temperature`) VALUES (%s,%s,%s)",(dateWrite,timeWrite,temperature))


        # Guardar los cambios.
        conn.commit()



        # Cerrar cursor
        cur.close()




        # Cerrar conexion
        conn.close()

        time.sleep(5) #delay of 10 seconds
        print("script terminado")
        

while True:
    ejecutaScript()
