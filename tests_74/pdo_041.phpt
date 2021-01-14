--TEST--
PDO::lastInsertId
--SKIPIF--
<?php
if (!extension_loaded("pdo")) die("skip");
require_once 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php

require_once 'pdo_test.inc';
$db = PDOTest::factory();

$db->exec("CREATE TABLE cubrid_test (id INT AUTO_INCREMENT, name varchar(20))");
$db->exec("INSERT INTO cubrid_test VALUES (1, 'A')");
$db->exec("INSERT INTO cubrid_test(name) VALUES ('B')");


$id= $db->lastInsertId('cubrid_test');

var_dump($id);

?>
--EXPECTF--
string(1) "1"
