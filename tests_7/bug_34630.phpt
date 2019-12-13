--TEST--
PDO Common: Bug #34630 (inserting streams as LOBs, This bug might be still open on aix5.2-ppc64 and hpux11.23-ia64)
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

$driver = $db->getAttribute(PDO::ATTR_DRIVER_NAME);
$db->exec('CREATE TABLE cubrid_test (id int NOT NULL PRIMARY KEY, val BLOB)');
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

$fp = tmpfile();
fwrite($fp, "I am the LOB data");
rewind($fp);

$insert = $db->prepare("insert into cubrid_test (id, val) values (1, :blob)");
$insert->bindValue(':blob', $fp, PDO::PARAM_LOB);
$insert->execute();
$insert = null;

$db->setAttribute(PDO::ATTR_STRINGIFY_FETCHES, true);
var_dump($db->query("SELECT * FROM cubrid_test")->fetchAll(PDO::FETCH_ASSOC));

?>
--EXPECT--
array(1) {
  [0]=>
  array(2) {
    ["id"]=>
    string(1) "1"
    ["val"]=>
    string(17) "I am the LOB data"
  }
}
