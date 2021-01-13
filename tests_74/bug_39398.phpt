--TEST--
PDO Common: Bug #39398 (Booleans are not automatically translated to integers)
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
$db->exec("CREATE TABLE cubrid_test (cubrid_test INT)");

$boolean = 1;
$stmt = $db->prepare('INSERT INTO cubrid_test VALUES (:boolean)');
$stmt->bindValue(':boolean', isset($boolean), PDO::PARAM_INT);
$stmt->execute();

var_dump($db->query("SELECT * FROM cubrid_test")->fetchAll(PDO::FETCH_ASSOC));
?>
===DONE===
--EXPECT--
array(1) {
  [0]=>
  array(1) {
    ["cubrid_test"]=>
    string(1) "1"
  }
}
===DONE===
