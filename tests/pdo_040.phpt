--TEST--
PDO::LOB
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

$fp = fopen('aclocal.m4', 'rb');

$db->exec('CREATE TABLE cubrid_test(id INT PRIMARY KEY, image BLOB)');

$sql_stmt = 'INSERT INTO cubrid_test VALUES(1, ?)';

$stmt = $db->prepare($sql_stmt);
$ret = $stmt->bindParam(1, $fp, PDO::PARAM_LOB);
$ret = $stmt->execute();

$sql_stmt = 'SELECT image FROM cubrid_test WHERE id=1';

$stmt = $db->prepare($sql_stmt);
$stmt->execute();
$result = $stmt->fetch(PDO::FETCH_NUM);

$db->exec('DROP TABLE cubrid_test');
?>
--EXPECTF--
