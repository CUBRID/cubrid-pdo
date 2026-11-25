--TEST--
PDO CUBRID PHP8: beginTransaction(), commit(), rollBack()
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

var_dump($db->beginTransaction());
var_dump($db->inTransaction(), $db->getAttribute(PDO::ATTR_AUTOCOMMIT));
$db->exec("INSERT INTO cubrid_test VALUES (1)");
var_dump($db->rollBack());
var_dump($db->inTransaction(), $db->getAttribute(PDO::ATTR_AUTOCOMMIT));
var_dump($db->query("SELECT COUNT(*) FROM cubrid_test")->fetchColumn());

var_dump($db->beginTransaction());
$db->exec("INSERT INTO cubrid_test VALUES (2)");
var_dump($db->commit());
var_dump($db->getAttribute(PDO::ATTR_AUTOCOMMIT));
var_dump($db->query("SELECT COUNT(*) FROM cubrid_test")->fetchColumn());

try {
    $db->commit();
} catch (PDOException $e) {
    echo $e->getMessage(), "\n";
}

$db->exec("DROP TABLE cubrid_test");
print "done!\n";
?>
--EXPECTF--
bool(true)
bool(true)
bool(false)
bool(true)
bool(false)
bool(true)
string(1) "0"
bool(true)
bool(true)
bool(true)
string(1) "1"
There is no active transaction
done!
