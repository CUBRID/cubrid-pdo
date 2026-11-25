--TEST--
PDO CUBRID PHP8: persistent connection with PDO attributes, liveness check closes its handle
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
require_once 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
require_once 'pdo_test.inc';
$dsn = 'cubrid:host=localhost;port=33000;dbname=demodb';

// integer keys are PDO attributes: the driver must not put them into the connection url
$db = new PDO($dsn, 'dba', '', array(PDO::ATTR_PERSISTENT => true));
var_dump($db->getAttribute(PDO::ATTR_PERSISTENT));
var_dump($db->query("SELECT 1 FROM db_root")->fetchColumn());
$db = null;

// every reuse of the persistent handle runs check_liveness()
for ($i = 0; $i < 2100; $i++) {
    $db = new PDO($dsn, 'dba', '', array(PDO::ATTR_PERSISTENT => true));
    $db = null;
}

$db = new PDO($dsn, 'dba', '', array(PDO::ATTR_PERSISTENT => true));
$stmt = $db->prepare("SELECT ? + 1 FROM db_root");
$stmt->bindValue(1, 41, PDO::PARAM_INT);
$stmt->execute();
var_dump($stmt->fetchColumn());
print "done!\n";
?>
--EXPECTF--
bool(true)
string(1) "1"
string(2) "42"
done!
