


import time
import datetime
import glob
from time import strftime
import pandas as pd
import numpy as np
import mysql.connector as mariadb



def ejecutaScript():
    
        print ('Ejecutando Script...')


        conn = mariadb.connect(host='localhost', port=3306, user='pi', password='raspberry', db='ATRAD')
        cur=conn.cursor()
        #cur.execute("SELECT * FROM `ATRAD1`")


        #print(cur.description)
        #print()



        colnames = ['Temperature','Status','forward_power1','retrun_loss1','forward_power2','retrun_loss2','forward_power3','retrun_loss3']
        data = pd.read_csv('ATRADvalues.csv', names=colnames)
        df=pd.DataFrame(data)

        i=df.shape
        print(i[0])

        element=df.iloc[i[0]-1] #obtiene propiedades de df
        #print(element)

        Temperature=element.Temperature
        Status  =element.Status
        forward_power1 =element.forward_power1
        retrun_loss1 =element.retrun_loss1
        forward_power2 =element.forward_power2
        retrun_loss2 =element.retrun_loss2
        forward_power3 =element.forward_power3
        retrun_loss3 =element. retrun_loss3


   
        #import sys
        #print(sys.version)





        dateWrite = time.strftime('%Y-%m-%d')
        timeWrite = time.strftime('%H:%M:%S')
        
        print( dateWrite)
        print( timeWrite )
        print( Temperature )
        print( Status  )
        print( forward_power1 )
        print( retrun_loss1 )
        print( forward_power2 )
        print( retrun_loss2  )
        print( forward_power3  )
        print( retrun_loss3  )




        cur.execute("INSERT INTO `ATRAD1` (`Date`, `Time`, `Temperature`, `Status`, `forward_power1`, `retrun_loss1`, `forward_power2`, `retrun_loss2`, `forward_power3`, `retrun_loss3`) VALUES (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)",(dateWrite,timeWrite,Temperature,Status,forward_power1,retrun_loss1,forward_power2,retrun_loss2,forward_power3,retrun_loss3))
        #cur.execute("INSERT INTO `ATRAD1` (`Date`, `Time`, `Temperature`, `Status`, `forward_power1`, `retrun_loss1`, `forward_power2`, `retrun_loss2`, `forward_power3`, `retrun_loss3`) VALUES ('', '', '', '', '', '', '', '', '', '')", (dateWrite,timeWrite, '25', '1', '12', '10', '14', '9', '13', '11'))

        # Guardar los cambios.
        conn.commit()



        # Cerrar cursor
        cur.close()




        # Cerrar conexion
        conn.close()

        time.sleep(3) #delay of 10 seconds
        print("script terminado")
        

while True:
    ejecutaScript()
