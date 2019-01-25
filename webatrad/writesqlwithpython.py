#!/Program Files (x86)/Microsoft Visual Studio/Shared/Python36_64/python
# -*- coding: UTF-8 -*-# enable debugging
import cgitb


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




cur.execute("INSERT INTO `temp at interrupt prueba1` (`Date`, `Time`, `Temperature`) VALUES ('2019-01-25', '08:23:01', '29')")

# Guardar los cambios.
conn.commit()



# Cerrar cursor
cur.close()

# Cerrar conexion
conn.close()