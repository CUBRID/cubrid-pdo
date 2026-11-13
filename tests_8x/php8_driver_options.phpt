--TEST--
PDO CUBRID PHP8: driver options of the constructor
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

// PDO::ATTR_* options are applied by PDO
$db = new PDO($dsn, 'dba', '', array(PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION, PDO::ATTR_CASE => PDO::CASE_UPPER));
var_dump($db->getAttribute(PDO::ATTR_ERRMODE) === PDO::ERRMODE_EXCEPTION);
var_dump($db->query("SELECT 1 AS one FROM db_root")->fetch(PDO::FETCH_ASSOC));

// string keys are connection url properties
$db = new PDO($dsn, 'dba', '', array('queryTimeout' => '5000'));
var_dump($db->query("SELECT 1 FROM db_root")->fetchColumn());

// a url property must be a string
try {
    new PDO($dsn, 'dba', '', array('queryTimeout' => 5000));
} catch (PDOException $e) {
    echo $e->getMessage(), "\n";
}

try {
    new PDO('cubrid:host=localhost;port=33000;dbname=no_such_db_php8', 'dba', '');
} catch (PDOException $e) {
    echo "connect failed\n";
}
print "done!\n";
?>
--EXPECTF--
bool(true)
array(1) {
  ["ONE"]=>
  string(1) "1"
}
string(1) "1"
SQLSTATE[HY000] [-30019] CLIENT, Invalid connection string
connect failed
done!
