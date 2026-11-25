--TEST--
PDO CUBRID PHP8: PDO::cubrid_schema() arguments
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
require_once 'pdo_test.inc';
PDOTest::skip();
?>
--INI--
; PHP 8.5 deprecates driver specific methods of PDO (see php8_pdo_cubrid_class.phpt)
error_reporting=E_ALL & ~E_DEPRECATED
--FILE--
<?php
require_once 'pdo_test.inc';
$db = PDOTest::factory();

// NULL means no pattern: no "Passing null to parameter" deprecation
$tables = $db->cubrid_schema(PDO::CUBRID_SCH_TABLE, NULL);
var_dump(is_array($tables) && count($tables) > 0);
var_dump(count($db->cubrid_schema(PDO::CUBRID_SCH_PRIMARY_KEY, "public.game", NULL)));

try {
    $db->cubrid_schema();
} catch (ArgumentCountError $e) {
    echo get_class($e), ": ", $e->getMessage(), "\n";
}
try {
    $db->cubrid_schema("table");
} catch (TypeError $e) {
    echo get_class($e), ": ", $e->getMessage(), "\n";
}
print "done!\n";
?>
--EXPECTF--
bool(true)
int(3)
ArgumentCountError: PDO::cubrid_schema() expects at least 1 argument, 0 given
TypeError: PDO::cubrid_schema(): Argument #1 ($schema_type) must be of type int, string given
done!
