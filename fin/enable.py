#!/usr/bin/env python

#http://localhost/enable.py/?en=1

import cgi
import cgitb
cgitb.enable()
import sys
import time




print "Content-type: text/html\n\n"
print "<html>\n<body>"
print "<div style=\"width: 100%; font-size: 40px; font-weight: bold; text-align: center;\">"
print "HABILITACION DE ATRAD"
print "</div>\n</body>\n</html>"


i=0

while i<4:
    i=i+1
    print("gg")
    time.sleep(2)




# Create instance of FieldStorage 
form = cgi.FieldStorage() 
# Get data from client
en = form.getvalue('en') 

type(en)


archivo = open("/home/pi/Desktop/enable.txt","w") 
archivo.write(en)
archivo.close()

print (en)
