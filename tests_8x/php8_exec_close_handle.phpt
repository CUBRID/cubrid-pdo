--TEST--
PDO CUBRID PHP8: exec() and released statements close their request handle
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
$db->exec("CREATE TABLE cubrid_test (id INT)");

// the broker limits the prepared statements of a connection (MAX_PREPARED_STMT_COUNT)
for ($i = 0; $i < 2500; $i++) {
    $db->exec("INSERT INTO cubrid_test VALUES ($i)");
}
var_dump($db->query("SELECT COUNT(*) FROM cubrid_test")->fetchColumn());

// the slot of a released statement is reused, the statement list does not grow
$stmt = $db->prepare("SELECT id FROM cubrid_test WHERE id = ?");
$stmt = null;
$before = memory_get_usage();
for ($i = 0; $i < 2500; $i++) {
    $stmt = $db->prepare("SELECT id FROM cubrid_test WHERE id = ?");
    $stmt->execute(array($i));
    $row = $stmt->fetch(PDO::FETCH_NUM);
    $stmt = null;
}
$growth = memory_get_usage() - $before;
var_dump($row);
printf("memory: %s\n", $growth < 8192 ? "ok" : "grows $growth bytes");

var_dump($db->exec("DELETE FROM cubrid_test WHERE id < 10"));
$db->exec("DROP TABLE cubrid_test");
print "done!\n";
?>
--EXPECTF--
string(4) "2500"
array(1) {
  [0]=>
  string(4) "2499"
}
memory: ok
int(10)
done!
