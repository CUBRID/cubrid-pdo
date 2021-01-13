--TEST--
PDO Common: PECL Bug #5809 (PDOStatement::execute(array()) changes param)
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
require_once 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
require_once 'pdo_test.inc';
$db = PDOTest::factory();

$db->exec("CREATE TABLE cubrid_test (id int NOT NULL, PRIMARY KEY (id))");
$db->exec("INSERT INTO cubrid_test (id) VALUES (1)");

$values = array(1);
var_dump($values);
$stmt = $db->prepare('SELECT * FROM cubrid_test WHERE id = ?');
$stmt->execute($values);
var_dump($values);

--EXPECT--
array(1) {
  [0]=>
  int(1)
}
array(1) {
  [0]=>
  int(1)
}
