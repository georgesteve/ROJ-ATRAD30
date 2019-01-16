#!/Program Files (x86)/Microsoft Visual Studio/Shared/Python36_64/Scripts

import pymysql

conn = pymysql.connect(host='localhost', port=3306, user='root', passwd='gfajardo', db='prueba1roj')

cur = conn.cursor()
cur.execute("SELECT * FROM `temp at interrupt prueba1`")

print(cur.description)
print()

for row in cur:
    print(row)

cur.close()
conn.close()
