--TEST--
PDO Common: PDO::FETCH_GROUP
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

$db->exec('CREATE TABLE cubrid_test(id int NOT NULL PRIMARY KEY, val VARCHAR(10))');
$db->exec("INSERT INTO cubrid_test VALUES(1, 'A')");
$db->exec("INSERT INTO cubrid_test VALUES(2, 'A')");
$db->exec("INSERT INTO cubrid_test VALUES(3, 'C')");

$stmt = $db->prepare('SELECT val, id FROM cubrid_test');

$stmt->execute();
var_dump($stmt->fetchAll(PDO::FETCH_NUM|PDO::FETCH_GROUP));

$stmt->execute();
var_dump($stmt->fetchAll(PDO::FETCH_ASSOC|PDO::FETCH_GROUP));

?>
--EXPECT--
array(2) {
  ["A"]=>
  array(2) {
    [0]=>
    array(1) {
      [0]=>
      string(1) "1"
    }
    [1]=>
    array(1) {
      [0]=>
      string(1) "2"
    }
  }
  ["C"]=>
  array(1) {
    [0]=>
    array(1) {
      [0]=>
      string(1) "3"
    }
  }
}
array(2) {
  ["A"]=>
  array(2) {
    [0]=>
    array(1) {
      ["id"]=>
      string(1) "1"
    }
    [1]=>
    array(1) {
      ["id"]=>
      string(1) "2"
    }
  }
  ["C"]=>
  array(1) {
    [0]=>
    array(1) {
      ["id"]=>
      string(1) "3"
    }
  }
}
