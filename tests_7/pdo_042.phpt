--TEST--
PDO::cubrid_schema
--SKIPIF--
<?php #vim:ft=php
if (!extension_loaded("pdo")) die("skip");
require_once 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php

require_once 'pdo_test.inc';
$db = PDOTest::factory();

$db->exec('CREATE USER u1');
$db->exec('CREATE TABLE u1.t1(col1 int, col2 varchar(10), col3 double)');
$db->exec('Grant SELECT ON u1.t1 TO dba;');
$db->exec('CREATE synonym dba.s1 for u1.t1');

$pk_list = $db->cubrid_schema(PDO::CUBRID_SCH_ATTR_WITH_SYNONYM, "s1", "col1");
var_dump($pk_list);

$pk_list = $db->cubrid_schema(PDO::CUBRID_SCH_ATTR_WITH_SYNONYM, "dba.s1", "col1");
var_dump($pk_list);

$pk_list = $db->cubrid_schema(PDO::CUBRID_SCH_ATTR_WITH_SYNONYM, "dba.s1", "col%");
var_dump($pk_list);

$pk_list = $db->cubrid_schema(PDO::CUBRID_SCH_ATTR_WITH_SYNONYM, "dba.s%", "col1%");
var_dump($pk_list);

$u1_db = PDOTest::user_factory();
$pk_list = $u1_db->cubrid_schema(PDO::CUBRID_SCH_ATTR_WITH_SYNONYM, "s1", "col1");
var_dump($pk_list);
$u1_db = null;

$db->exec('drop synonym if exists dba.s1');
$db->exec('drop table if EXISTS u1.t1');
$db->exec('DROP USER u1');

?>
--EXPECTF--
array(1) {
  [0]=>
  array(14) {
    ["ATTR_NAME"]=>
    string(4) "col1"
    ["DOMAIN"]=>
    string(1) "8"
    ["SCALE"]=>
    string(1) "0"
    ["PRECISION"]=>
    string(2) "10"
    ["INDEXED"]=>
    string(1) "0"
    ["NON_NULL"]=>
    string(1) "0"
    ["SHARED"]=>
    string(1) "0"
    ["UNIQUE"]=>
    string(1) "0"
    ["DEFAULT"]=>
    string(4) "NULL"
    ["ATTR_ORDER"]=>
    string(1) "1"
    ["CLASS_NAME"]=>
    string(5) "u1.t1"
    ["SOURCE_CLASS"]=>
    string(5) "u1.t1"
    ["IS_KEY"]=>
    string(1) "0"
    ["REMARKS"]=>
    string(0) ""
  }
}
array(1) {
  [0]=>
  array(14) {
    ["ATTR_NAME"]=>
    string(4) "col1"
    ["DOMAIN"]=>
    string(1) "8"
    ["SCALE"]=>
    string(1) "0"
    ["PRECISION"]=>
    string(2) "10"
    ["INDEXED"]=>
    string(1) "0"
    ["NON_NULL"]=>
    string(1) "0"
    ["SHARED"]=>
    string(1) "0"
    ["UNIQUE"]=>
    string(1) "0"
    ["DEFAULT"]=>
    string(4) "NULL"
    ["ATTR_ORDER"]=>
    string(1) "1"
    ["CLASS_NAME"]=>
    string(5) "u1.t1"
    ["SOURCE_CLASS"]=>
    string(5) "u1.t1"
    ["IS_KEY"]=>
    string(1) "0"
    ["REMARKS"]=>
    string(0) ""
  }
}
array(3) {
  [0]=>
  array(14) {
    ["ATTR_NAME"]=>
    string(4) "col1"
    ["DOMAIN"]=>
    string(1) "8"
    ["SCALE"]=>
    string(1) "0"
    ["PRECISION"]=>
    string(2) "10"
    ["INDEXED"]=>
    string(1) "0"
    ["NON_NULL"]=>
    string(1) "0"
    ["SHARED"]=>
    string(1) "0"
    ["UNIQUE"]=>
    string(1) "0"
    ["DEFAULT"]=>
    string(4) "NULL"
    ["ATTR_ORDER"]=>
    string(1) "1"
    ["CLASS_NAME"]=>
    string(5) "u1.t1"
    ["SOURCE_CLASS"]=>
    string(5) "u1.t1"
    ["IS_KEY"]=>
    string(1) "0"
    ["REMARKS"]=>
    string(0) ""
  }
  [1]=>
  array(14) {
    ["ATTR_NAME"]=>
    string(4) "col2"
    ["DOMAIN"]=>
    string(1) "2"
    ["SCALE"]=>
    string(1) "0"
    ["PRECISION"]=>
    string(2) "10"
    ["INDEXED"]=>
    string(1) "0"
    ["NON_NULL"]=>
    string(1) "0"
    ["SHARED"]=>
    string(1) "0"
    ["UNIQUE"]=>
    string(1) "0"
    ["DEFAULT"]=>
    string(4) "NULL"
    ["ATTR_ORDER"]=>
    string(1) "2"
    ["CLASS_NAME"]=>
    string(5) "u1.t1"
    ["SOURCE_CLASS"]=>
    string(5) "u1.t1"
    ["IS_KEY"]=>
    string(1) "0"
    ["REMARKS"]=>
    string(0) ""
  }
  [2]=>
  array(14) {
    ["ATTR_NAME"]=>
    string(4) "col3"
    ["DOMAIN"]=>
    string(2) "12"
    ["SCALE"]=>
    string(1) "0"
    ["PRECISION"]=>
    string(2) "15"
    ["INDEXED"]=>
    string(1) "0"
    ["NON_NULL"]=>
    string(1) "0"
    ["SHARED"]=>
    string(1) "0"
    ["UNIQUE"]=>
    string(1) "0"
    ["DEFAULT"]=>
    string(4) "NULL"
    ["ATTR_ORDER"]=>
    string(1) "3"
    ["CLASS_NAME"]=>
    string(5) "u1.t1"
    ["SOURCE_CLASS"]=>
    string(5) "u1.t1"
    ["IS_KEY"]=>
    string(1) "0"
    ["REMARKS"]=>
    string(0) ""
  }
}
array(0) {
}
array(0) {
}
