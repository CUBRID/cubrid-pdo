--TEST--
PDO CUBRID PHP8: PDO::ATTR_AUTOCOMMIT is converted to bool
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

var_dump($db->setAttribute(PDO::ATTR_AUTOCOMMIT, false));
var_dump($db->getAttribute(PDO::ATTR_AUTOCOMMIT));
$db->exec("INSERT INTO cubrid_test VALUES (1)");
// switching autocommit on commits the pending work
var_dump($db->setAttribute(PDO::ATTR_AUTOCOMMIT, true));
var_dump($db->getAttribute(PDO::ATTR_AUTOCOMMIT));

// int values are accepted too
var_dump($db->setAttribute(PDO::ATTR_AUTOCOMMIT, 0));
var_dump($db->getAttribute(PDO::ATTR_AUTOCOMMIT));
$db->exec("INSERT INTO cubrid_test VALUES (2)");
// disconnect without commit: rolled back
$db = null;

$db = PDOTest::factory('PDO', false);
var_dump($db->query("SELECT id FROM cubrid_test ORDER BY id")->fetchAll(PDO::FETCH_COLUMN));
var_dump($db->setAttribute(PDO::ATTR_AUTOCOMMIT, 1));
var_dump($db->getAttribute(PDO::ATTR_AUTOCOMMIT));

try {
    $db->setAttribute(PDO::ATTR_AUTOCOMMIT, array());
} catch (TypeError $e) {
    echo get_class($e), ": ", $e->getMessage(), "\n";
}
var_dump($db->getAttribute(PDO::ATTR_AUTOCOMMIT));

$db->exec("DROP TABLE cubrid_test");
print "done!\n";
?>
--EXPECTF--
bool(true)
bool(false)
bool(true)
bool(true)
bool(true)
bool(false)
array(1) {
  [0]=>
  string(1) "1"
}
bool(true)
bool(true)
TypeError: Attribute value must be of type bool%s
bool(true)
done!
