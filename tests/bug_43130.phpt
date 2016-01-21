--TEST--
PDO Common: Bug #43130 (Bound parameters cannot have - in their name)
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

if ($db->getAttribute(PDO::ATTR_DRIVER_NAME) == 'mysql')
	$db->setAttribute(PDO::ATTR_EMULATE_PREPARES, 1);

$db->exec("CREATE TABLE cubrid_test (a varchar(100), b varchar(100), c varchar(100))");

for ($i = 0; $i < 5; $i++) {
	$db->exec("INSERT INTO cubrid_test (a,b,c) VALUES('test".$i."','".$i."','".$i."')");
}

$stmt = $db->prepare("SELECT a FROM cubrid_test WHERE b=:id-value");
$stmt->bindParam(':id-value', $id);
$id = '1';
$stmt->execute();
var_dump($stmt->fetch(PDO::FETCH_COLUMN));
?>
--EXPECTF--
Warning: PDOStatement::execute(): SQLSTATE[HY093]: Invalid parameter number: parameter was not defined in %s on line %d

Warning: PDOStatement::execute(): SQLSTATE[HY093]: Invalid parameter number in %s on line %d
bool(false)
