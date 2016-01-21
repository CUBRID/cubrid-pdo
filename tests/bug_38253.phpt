--TEST--
PDO Common: Bug #38253 (PDO produces segfault with default fetch mode)
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
require_once 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
require_once 'pdo_test.inc';
$pdo = PDOTest::factory();

$pdo->exec ("create table cubrid_test (id integer primary key, n varchar(255))");
$pdo->exec ("INSERT INTO cubrid_test (id, n) VALUES (1, 'hi')");

$pdo->setAttribute (PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_CLASS);
$stmt = $pdo->prepare ("SELECT * FROM cubrid_test");
$stmt->execute();
var_dump($stmt->fetchAll());

$pdo->exec ("create table cubrid_test2 (id integer primary key, n string)");
$pdo->exec ("INSERT INTO cubrid_test2 (id, n) VALUES (1,'hi')");

$pdo->setAttribute (PDO::ATTR_DEFAULT_FETCH_MODE, PDO::FETCH_FUNC);
$stmt = $pdo->prepare ("SELECT * FROM cubrid_test2");
$stmt->execute();
var_dump($stmt->fetchAll());

?>
--EXPECTF--
Warning: PDOStatement::fetchAll(): SQLSTATE[HY000]: General error: No fetch class specified in %s on line %d

Warning: PDOStatement::fetchAll(): SQLSTATE[HY000]: General error%s on line %d
array(0) {
}

Warning: PDOStatement::fetchAll(): SQLSTATE[HY000]: General error: No fetch function specified in %s on line %d

Warning: PDOStatement::fetchAll(): SQLSTATE[HY000]: General error%s on line %d
array(0) {
}
