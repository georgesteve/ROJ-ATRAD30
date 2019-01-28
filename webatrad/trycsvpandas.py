#!/Program Files (x86)/Python37-32/python

# -*- coding: UTF-8 -*-# enable debugging
import cgitb
import pandas as pd
import numpy as np


cgitb.enable()    
print("Content-Type: text/html;charset=utf-8")
print()    



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
