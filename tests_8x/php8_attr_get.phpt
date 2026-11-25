--TEST--
PDO CUBRID PHP8: getAttribute() / setAttribute() of driver attributes
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

var_dump(preg_match('/^\d+\.\d+\.\d+$/', $db->getAttribute(PDO::ATTR_CLIENT_VERSION)));
var_dump(is_string($db->getAttribute(PDO::ATTR_SERVER_VERSION)));
var_dump($db->getAttribute(PDO::ATTR_SERVER_INFO) === $db->getAttribute(PDO::ATTR_SERVER_VERSION));
var_dump(is_int($db->getAttribute(PDO::CUBRID_ATTR_MAX_STRING_LENGTH)));

var_dump($db->setAttribute(PDO::CUBRID_ATTR_ISOLATION_LEVEL, PDO::TRAN_SERIALIZABLE));
var_dump($db->getAttribute(PDO::CUBRID_ATTR_ISOLATION_LEVEL) === PDO::TRAN_SERIALIZABLE);
var_dump($db->setAttribute(PDO::CUBRID_ATTR_LOCK_TIMEOUT, 3));
var_dump($db->getAttribute(PDO::CUBRID_ATTR_LOCK_TIMEOUT));

$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
try {
    $db->getAttribute(PDO::ATTR_PREFETCH);
} catch (PDOException $e) {
    echo $e->getMessage(), "\n";
}
var_dump($db->setAttribute(PDO::ATTR_PREFETCH, 1));
print "done!\n";
?>
--EXPECTF--
int(1)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
int(3)
SQLSTATE[IM001]: Driver does not support this function: driver does not support that attribute
bool(false)
done!
