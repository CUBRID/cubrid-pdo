--TEST--
PDO Common: PDO::FETCH_UNIQUE
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

$db->exec('CREATE TABLE cubrid_test(id CHAR(1) NOT NULL PRIMARY KEY, val VARCHAR(10))');
$db->exec("INSERT INTO cubrid_test VALUES('A', 'A')");
$db->exec("INSERT INTO cubrid_test VALUES('B', 'A')");
$db->exec("INSERT INTO cubrid_test VALUES('C', 'C')");

$stmt = $db->prepare('SELECT id, val FROM cubrid_test');

$stmt->execute();
var_dump($stmt->fetchAll(PDO::FETCH_NUM|PDO::FETCH_UNIQUE));

$stmt->execute();
var_dump($stmt->fetchAll(PDO::FETCH_ASSOC|PDO::FETCH_UNIQUE));

?>
--EXPECT--
array(3) {
  ["A"]=>
  array(1) {
    [0]=>
    string(1) "A"
  }
  ["B"]=>
  array(1) {
    [0]=>
    string(1) "A"
  }
  ["C"]=>
  array(1) {
    [0]=>
    string(1) "C"
  }
}
array(3) {
  ["A"]=>
  array(1) {
    ["val"]=>
    string(1) "A"
  }
  ["B"]=>
  array(1) {
    ["val"]=>
    string(1) "A"
  }
  ["C"]=>
  array(1) {
    ["val"]=>
    string(1) "C"
  }
}
