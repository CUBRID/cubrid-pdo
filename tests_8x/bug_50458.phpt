--TEST--
PDO Common: Bug #50458 (PDO::FETCH_FUNC fails with Closures)
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
$db->exec("CREATE TABLE cubrid_test (a VARCHAR(10))");
$db->exec("INSERT INTO cubrid_test (a) VALUES ('xyz')");
$res = $db->query("SELECT a FROM cubrid_test");
var_dump($res->fetchAll(PDO::FETCH_FUNC, function($a) { return strtoupper($a); }));

?>
===DONE===
--EXPECTF--
array(1) {
  [0]=>
  string(3) "XYZ"
}
===DONE===
