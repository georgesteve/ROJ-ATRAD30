<?php
//Step1 -

$username="root";
$password="gfajardo";
$database="prueba1roj";
$server="localhost";

 $db = mysqli_connect($server,$username,$password,$database)
 or die('Error connecting to MySQL server.');
?>

<html>
   <head>
      <title>Sensor Data</title>
   </head>
<body>
   <h1>Temperature readings</h1>

   <table border="1" cellspacing="1" cellpadding="1">
		<tr>
			<td>&nbsp;Date&nbsp;</td>
			<td>&nbsp;Time&nbsp;</td>
			<td>&nbsp;Temperature&nbsp;</td>
		</tr>



<?php
//Step2
$query = "SELECT * FROM `temp at interrupt prueba1`";
mysqli_query($db, $query) or die('Error querying database.');

$result = mysqli_query($db, $query);
$row = mysqli_fetch_array($result);


while ($row = mysqli_fetch_array($result)) {
 printf("<tr><td> &nbsp;%s </td><td> &nbsp;%s&nbsp; </td><td> &nbsp;%s&nbsp; </td></tr>", 
		           $row["Date"], $row["Time"], $row["Temperature"]);
}

mysqli_free_result($result);
		     mysqli_close();

?>

</body>
</html>
