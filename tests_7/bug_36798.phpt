--TEST--
PDO Common: Bug #36798 (Error parsing named parameters with queries containing high-ascii chars)
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

@$db->exec("SET NAMES 'LATIN1'"); // needed for PostgreSQL
$db->exec("CREATE TABLE cubrid_test (id INTEGER)");
$db->exec("INSERT INTO cubrid_test (id) VALUES (1)");

$stmt = $db->prepare("SELECT 'Ã' as cubrid_test FROM cubrid_test WHERE id = :id");
$stmt->execute(array(":id" => 1));

$row = $stmt->fetch(PDO::FETCH_NUM);
var_dump( $row );

?>
--EXPECT--	
array(1) {
  [0]=>
  string(1) "Ã"
}
