#!/Program Files (x86)/Microsoft Visual Studio/Shared/Python36_64/python
# -*- coding: UTF-8 -*-# enable debugging
import cgitb
import time
import datetime
import glob
from time import strftime
import csv, operator
#import numpy as np



cgitb.enable()    
print("Content-Type: text/html;charset=utf-8")
print()    


with open('example.csv') as File:
    reader = csv.reader(File, delimiter=',', quotechar=',',
                        quoting=csv.QUOTE_MINIMAL)
    

    i=0
    j=0
    a=[]
    for row in reader:
    	
        print(row)
        i=i+1
print(i-1)



