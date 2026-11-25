--TEST--
PDO CUBRID PHP8: PDO::cubrid_schema() is deprecated by PHP 8.5 but still works
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
if (PHP_VERSION_ID < 80500) die('skip PHP 8.5 deprecation');
require_once 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
require_once 'pdo_test.inc';
$db = PDOTest::factory();
var_dump(count($db->cubrid_schema(PDO::CUBRID_SCH_PRIMARY_KEY, 'game')));
print "done!\n";
?>
--EXPECTF--
Deprecated: Method PDO::cubrid_schema() is deprecated in %s on line %d
int(3)
done!
