--TEST--
PDO Common: PDOStatement::columnCount
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

$db->exec('CREATE TABLE cubrid_test(id INT NOT NULL PRIMARY KEY, val VARCHAR(10), val2 VARCHAR(16))');
$db->exec("INSERT INTO cubrid_test VALUES(1, 'A', 'A')"); 
$db->exec("INSERT INTO cubrid_test VALUES(2, 'B', 'B')"); 
$db->exec("INSERT INTO cubrid_test VALUES(3, 'C', 'C')");

foreach (array('SELECT id, val FROM cubrid_test', 'SELECT id, val, val2 FROM cubrid_test', 'SELECT COUNT(*) FROM cubrid_test') as $sql) {

	$stmt = $db->query($sql);
	$res = $stmt->columnCount();
    echo "Counted $res columns after $sql.\n";
	$stmt = null;
}

?>
--EXPECT--
Counted 2 columns after SELECT id, val FROM cubrid_test.
Counted 3 columns after SELECT id, val, val2 FROM cubrid_test.
Counted 1 columns after SELECT COUNT(*) FROM cubrid_test.
