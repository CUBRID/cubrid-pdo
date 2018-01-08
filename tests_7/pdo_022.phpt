--TEST--
PDO Common: PDOStatement::getColumnMeta
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
require_once 'pdo_test.inc';
PDOTest::skip();

/*
 * Note well: meta information is a nightmare to handle portably.
 * it's not really PDOs job.
 * We've not yet defined exactly what makes sense for getColumnMeta,
 * so no tests make any sense to anyone.  When they do, we can enable
 * this test file.
 * TODO: filter out driver dependent components from this common core
 * test file.
 */
?>
--FILE--
<?php
require_once 'pdo_test.inc';
$db = PDOTest::factory();

$db->exec('CREATE TABLE cubrid_test(id INT NOT NULL PRIMARY KEY, val VARCHAR(10), val2 VARCHAR(16))');
//$db->exec('insert2', "INSERT INTO cubrid_test VALUES(:first, :second, :third)"); 

$data = array(
    array('10', 'Abc', 'zxy'),
    array('20', 'Def', 'wvu'),
    array('30', 'Ghi', 'tsr'),
    array('40', 'Jkl', 'qpo'),
    array('50', 'Mno', 'nml'),
    array('60', 'Pqr', 'kji'),
);


// Insert using question mark placeholders
$stmt = $db->prepare("INSERT INTO cubrid_test VALUES(?, ?, ?)");
foreach ($data as $row) {
    $stmt->execute($row);
}

// Retrieve column metadata for a result set returned by explicit SELECT
$select = $db->query('SELECT id, val, val2 FROM cubrid_test');
$meta = $select->getColumnMeta(0);
var_dump($meta);
$meta = $select->getColumnMeta(1);
var_dump($meta);
$meta = $select->getColumnMeta(2);
var_dump($meta);

// Retrieve column metadata for a result set returned by a function
$select = $db->query('SELECT COUNT(*) FROM cubrid_test');
$meta = $select->getColumnMeta(0);
var_dump($meta);

?>
--EXPECT--
array(16) {
  ["type"]=>
  string(7) "integer"
  ["name"]=>
  string(2) "id"
  ["table"]=>
  string(11) "cubrid_test"
  ["def"]=>
  string(4) "NULL"
  ["precision"]=>
  int(10)
  ["scale"]=>
  int(0)
  ["not_null"]=>
  int(1)
  ["auto_increment"]=>
  int(0)
  ["unique_key"]=>
  int(1)
  ["multiple_key"]=>
  int(0)
  ["primary_key"]=>
  int(1)
  ["foreign_key"]=>
  int(0)
  ["reverse_index"]=>
  int(0)
  ["reverse_unique"]=>
  int(0)
  ["len"]=>
  int(11)
  ["pdo_type"]=>
  int(2)
}
array(16) {
  ["type"]=>
  string(11) "varchar(10)"
  ["name"]=>
  string(3) "val"
  ["table"]=>
  string(11) "cubrid_test"
  ["def"]=>
  string(4) "NULL"
  ["precision"]=>
  int(10)
  ["scale"]=>
  int(0)
  ["not_null"]=>
  int(0)
  ["auto_increment"]=>
  int(0)
  ["unique_key"]=>
  int(0)
  ["multiple_key"]=>
  int(1)
  ["primary_key"]=>
  int(0)
  ["foreign_key"]=>
  int(0)
  ["reverse_index"]=>
  int(0)
  ["reverse_unique"]=>
  int(0)
  ["len"]=>
  int(10)
  ["pdo_type"]=>
  int(2)
}
array(16) {
  ["type"]=>
  string(11) "varchar(16)"
  ["name"]=>
  string(4) "val2"
  ["table"]=>
  string(11) "cubrid_test"
  ["def"]=>
  string(4) "NULL"
  ["precision"]=>
  int(16)
  ["scale"]=>
  int(0)
  ["not_null"]=>
  int(0)
  ["auto_increment"]=>
  int(0)
  ["unique_key"]=>
  int(0)
  ["multiple_key"]=>
  int(1)
  ["primary_key"]=>
  int(0)
  ["foreign_key"]=>
  int(0)
  ["reverse_index"]=>
  int(0)
  ["reverse_unique"]=>
  int(0)
  ["len"]=>
  int(16)
  ["pdo_type"]=>
  int(2)
}
array(16) {
  ["type"]=>
  string(7) "integer"
  ["name"]=>
  string(8) "count(*)"
  ["table"]=>
  string(0) ""
  ["def"]=>
  string(0) ""
  ["precision"]=>
  int(10)
  ["scale"]=>
  int(0)
  ["not_null"]=>
  int(0)
  ["auto_increment"]=>
  int(0)
  ["unique_key"]=>
  int(0)
  ["multiple_key"]=>
  int(1)
  ["primary_key"]=>
  int(0)
  ["foreign_key"]=>
  int(0)
  ["reverse_index"]=>
  int(0)
  ["reverse_unique"]=>
  int(0)
  ["len"]=>
  int(11)
  ["pdo_type"]=>
  int(2)
}

