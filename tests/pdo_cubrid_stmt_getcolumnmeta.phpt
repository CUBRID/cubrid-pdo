--TEST--
PDO CUBRID: PDOStatement::getColumnMeta
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

$result = $db->query('SELECT 1 FROM db_root');

var_dump($result->getColumnMeta(0));

$result = $db->query('SELECT * FROM game limit 1');

var_dump($result->getColumnMeta(0));
var_dump($result->getColumnMeta(1));
var_dump($result->getColumnMeta(4));
var_dump($result->getColumnMeta(6));
?>
--EXPECT--
array(16) {
  ["type"]=>
  string(7) "integer"
  ["name"]=>
  string(1) "1"
  ["table"]=>
  string(0) ""
  ["def"]=>
  string(0) ""
  ["precision"]=>
  int(10)
  ["scale"]=>
  int(0)
  ["not_null"]=>
  int(1)
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
array(16) {
  ["type"]=>
  string(7) "integer"
  ["name"]=>
  string(9) "host_year"
  ["table"]=>
  string(4) "game"
  ["def"]=>
  string(0) ""
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
  string(7) "integer"
  ["name"]=>
  string(10) "event_code"
  ["table"]=>
  string(4) "game"
  ["def"]=>
  string(0) ""
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
  int(1)
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
  string(7) "char(3)"
  ["name"]=>
  string(11) "nation_code"
  ["table"]=>
  string(4) "game"
  ["def"]=>
  string(0) ""
  ["precision"]=>
  int(3)
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
  int(3)
  ["pdo_type"]=>
  int(2)
}
array(16) {
  ["type"]=>
  string(4) "date"
  ["name"]=>
  string(9) "game_date"
  ["table"]=>
  string(4) "game"
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
  int(10)
  ["pdo_type"]=>
  int(2)
}

