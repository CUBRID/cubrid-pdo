--TEST--
PDO Common: Bug #38394 (Prepared statement error stops subsequent statements)
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
$db->exec("CREATE TABLE cubrid_test (a INT, b INT, c INT)");
$s = $db->prepare("INSERT INTO cubrid_test (a,b,c) VALUES (:a,:b,:c)");

$s->execute(array('a' => 1, 'b' => 2, 'c' => 3));

@$s->execute(array('a' => 5, 'b' => 6, 'c' => 7, 'd' => 8));

$s->execute(array('a' => 9, 'b' => 10, 'c' => 11));

var_dump($db->query("SELECT * FROM cubrid_test")->fetchAll(PDO::FETCH_ASSOC));
?>
===DONE===
--EXPECTF--
array(2) {
  [0]=>
  array(3) {
    ["a"]=>
    string(1) "1"
    ["b"]=>
    string(1) "2"
    ["c"]=>
    string(1) "3"
  }
  [1]=>
  array(3) {
    ["a"]=>
    string(1) "9"
    ["b"]=>
    string(2) "10"
    ["c"]=>
    string(2) "11"
  }
}
===DONE===
