--TEST--
PDO CUBRID PHP8: error reporting of the driver
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

try {
    $db->prepare("SELEC 1");
} catch (PDOException $e) {
    var_dump($e->getCode(), $e->errorInfo[0], is_int($e->errorInfo[1]), is_string($e->errorInfo[2]));
}
try {
    $db->exec("SELECT * FROM no_such_table_php8");
} catch (PDOException $e) {
    var_dump(strpos($e->getMessage(), "SQLSTATE[HY000]: General error: ") === 0);
}

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_SILENT);
var_dump($db->exec("SELECT * FROM no_such_table_php8"));
$info = $db->errorInfo();
var_dump($info[0], $info[1] < 0);

$stmt = $db->prepare("SELECT * FROM db_root WHERE 1 = ?");
var_dump($stmt->execute());
var_dump($stmt->errorInfo());
print "done!\n";
?>
--EXPECTF--
string(5) "HY000"
string(5) "HY000"
bool(true)
bool(true)
bool(true)
bool(false)
string(5) "HY000"
bool(true)
bool(false)
array(3) {
  [0]=>
  string(5) "HY000"
  [1]=>
  int(-30017)
  [2]=>
  string(22) "CLIENT, Param not bind"
}
done!
