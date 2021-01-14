--TEST--
PDO Common: assert that bindParam does not modify parameter
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

$stmt = $db->prepare('insert into cubrid_test (id, name) values(0, :name)');
$name = NULL;
$before_bind = $name;
$stmt->bindParam(':name', $name);
if ($name !== $before_bind) {
	echo "bind: fail\n";
} else {
	echo "bind: success\n";
}
var_dump($stmt->execute());
var_dump($db->query('select name FROM cubrid_test where id=0')->fetchColumn());

?>
--EXPECT--
bind: success
bool(true)
NULL
