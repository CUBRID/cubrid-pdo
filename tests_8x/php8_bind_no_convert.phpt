--TEST--
PDO CUBRID PHP8: bound variables are not converted in place (typed property reference)
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
$db->exec("CREATE TABLE cubrid_test (id INT, name VARCHAR(20))");

class Row {
    public int $id = 7;
}

$row = new Row;
$stmt = $db->prepare("INSERT INTO cubrid_test VALUES (?, ?)");
$stmt->bindParam(1, $row->id, PDO::PARAM_INT);
$stmt->bindValue(2, "typed");
$stmt->execute();
$row->id = 8;
$stmt->execute();
var_dump($row->id);

$id = 9;
$stmt->bindParam(1, $id, PDO::PARAM_INT);
$stmt->bindValue(2, NULL);
$stmt->execute();
var_dump($id);

var_dump($db->query("SELECT id, name FROM cubrid_test ORDER BY id")->fetchAll(PDO::FETCH_NUM));
$db->exec("DROP TABLE cubrid_test");
print "done!\n";
?>
--EXPECTF--
int(8)
int(9)
array(3) {
  [0]=>
  array(2) {
    [0]=>
    string(1) "7"
    [1]=>
    string(5) "typed"
  }
  [1]=>
  array(2) {
    [0]=>
    string(1) "8"
    [1]=>
    string(5) "typed"
  }
  [2]=>
  array(2) {
    [0]=>
    string(1) "9"
    [1]=>
    NULL
  }
}
done!
