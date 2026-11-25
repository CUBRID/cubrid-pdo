--TEST--
PDO CUBRID PHP8: binding an array to a collection does not modify the array
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
$db->exec("CREATE TABLE cubrid_test (id INT, s SET(VARCHAR(10)), q SEQUENCE(INT))");

$stmt = $db->prepare("INSERT INTO cubrid_test VALUES (?, ?, ?)");
$seq = array(3, 1, 2);
for ($i = 1; $i <= 2; $i++) {
    $stmt->bindValue(1, $i, PDO::PARAM_INT);
    // a literal array is immutable in PHP 8
    $stmt->bindValue(2, array('a', 'b', 'c'), PDO::PARAM_INT);
    $stmt->bindParam(3, $seq, PDO::PARAM_INT);
    $stmt->execute();
}
var_dump($seq);

// empty collection
$stmt->bindValue(1, 3, PDO::PARAM_INT);
$stmt->bindValue(2, array(), PDO::PARAM_INT);
$stmt->bindValue(3, array(), PDO::PARAM_INT);
$stmt->execute();

var_dump($db->query("SELECT id, s, q FROM cubrid_test ORDER BY id")->fetchAll(PDO::FETCH_NUM));
$db->exec("DROP TABLE cubrid_test");
print "done!\n";
?>
--EXPECTF--
array(3) {
  [0]=>
  int(3)
  [1]=>
  int(1)
  [2]=>
  int(2)
}
array(3) {
  [0]=>
  array(3) {
    [0]=>
    string(1) "1"
    [1]=>
    string(9) "{a, b, c}"
    [2]=>
    string(9) "{1, 2, 3}"
  }
  [1]=>
  array(3) {
    [0]=>
    string(1) "2"
    [1]=>
    string(9) "{a, b, c}"
    [2]=>
    string(9) "{1, 2, 3}"
  }
  [2]=>
  array(3) {
    [0]=>
    string(1) "3"
    [1]=>
    string(2) "{}"
    [2]=>
    string(2) "{}"
  }
}
done!
