--TEST--
PDO CUBRID PHP8: PDO::ATTR_TIMEOUT
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

var_dump($db->getAttribute(PDO::ATTR_TIMEOUT));
var_dump($db->setAttribute(PDO::ATTR_TIMEOUT, 5));
var_dump($db->getAttribute(PDO::ATTR_TIMEOUT));
var_dump($db->setAttribute(PDO::ATTR_TIMEOUT, "10"));
var_dump($db->getAttribute(PDO::ATTR_TIMEOUT));

// 0 and negative values other than -1 are rejected
var_dump($db->setAttribute(PDO::ATTR_TIMEOUT, 0));
var_dump($db->setAttribute(PDO::ATTR_TIMEOUT, -5));
var_dump($db->getAttribute(PDO::ATTR_TIMEOUT));
var_dump($db->setAttribute(PDO::ATTR_TIMEOUT, -1));
var_dump($db->getAttribute(PDO::ATTR_TIMEOUT));

try {
    $db->setAttribute(PDO::ATTR_TIMEOUT, "abc");
} catch (TypeError $e) {
    echo get_class($e), ": ", $e->getMessage(), "\n";
}

// the timeout is set to statements of prepare() and exec()
var_dump($db->setAttribute(PDO::ATTR_TIMEOUT, 3));
var_dump($db->query("SELECT 1 FROM db_root")->fetchColumn());
var_dump($db->exec("SELECT 1 FROM db_root"));
print "done!\n";
?>
--EXPECTF--
int(-1)
bool(true)
int(5)
bool(true)
int(10)
bool(false)
bool(false)
int(10)
bool(true)
int(-1)
TypeError: Attribute value must be of type int%s
bool(true)
string(1) "1"
int(1)
done!
