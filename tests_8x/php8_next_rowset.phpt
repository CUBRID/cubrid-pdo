--TEST--
PDO CUBRID PHP8: nextRowset() with a different column count
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
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

$stmt = $db->query("SELECT 1 FROM db_root; SELECT 2, 3 FROM db_root");
var_dump($stmt->columnCount());
var_dump($stmt->fetchAll(PDO::FETCH_NUM));
var_dump($stmt->nextRowset());
var_dump($stmt->columnCount());
var_dump($stmt->fetchAll(PDO::FETCH_NUM));
var_dump($stmt->nextRowset());
print "done!\n";
?>
--EXPECTF--
int(1)
array(1) {
  [0]=>
  array(1) {
    [0]=>
    string(1) "1"
  }
}
bool(true)
int(2)
array(1) {
  [0]=>
  array(2) {
    [0]=>
    string(1) "2"
    [1]=>
    string(1) "3"
  }
}
bool(false)
done!
