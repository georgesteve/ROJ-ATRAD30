# ROJ-ATRAD30
Repository with code needed to make the comunication between Radar-Arduino-Cloud
  Programs needed to the instalation:
  
  
    PC:
      Bitname WAMP(Or whatever WAMP in fact) 
        *Remember scripts in UTF-8 to make a correct comunicattion with Apache2 and python
        *C:\Bitnami\wampstack-5.6.22-0\apache2\conf\httpd.conf
          >Search for the line: "Options Indexes FollowSymLinks" and add "ExecCGI"
          <Search for the line: "#Add Handler cgi-script .cgi". Uncomment it and add a ".py" in the end.
      Enviorement Python3 and C++
      
      phpmyadmin .: http://10.10.10.31/phpmyadmin root/gfajardo
                    http://10.10.10.39/phpmyadmin root/contraseña
                    http://10.10.10.35/phpmyadmin root/password
    Raspberry:
      Raspbian
      Download MySQLdb module for Python 
      
      
     
  Web compiler: http://www.cubicfactory.com/jseditor/welcome/130/edit
  Tutorial LAMP: https://websiteforstudents.com/install-apache2-mariadb-and-php-7-2-with-phpmyadmin-on-ubuntu-16-04-18-04-18-10-lamp-phpmyadmin/
