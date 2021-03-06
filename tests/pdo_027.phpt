--TEST--
PDO Common: PDO::FETCH_LAZY
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

$db->exec('create table cubrid_test (id int, name varchar(10))');
$db->exec("INSERT INTO cubrid_test (id,name) VALUES(1,'test1')");
$db->exec("INSERT INTO cubrid_test (id,name) VALUES(2,'test2')");

foreach ($db->query("SELECT * FROM cubrid_test", PDO::FETCH_LAZY) as $v) {
	echo "lazy: " . $v->id.$v->name."\n";
}
echo "End\n";
?>
--EXPECT--
lazy: 1test1
lazy: 2test2
End
