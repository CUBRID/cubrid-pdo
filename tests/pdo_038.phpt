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

$pk_list = $db->cubrid_schema(PDO::CUBRID_SCH_PRIMARY_KEY, "game");
var_dump($pk_list);

$fk_list = $db->cubrid_schema(PDO::CUBRID_SCH_IMPORTED_KEYS, "game");
var_dump($fk_list);

?>
--EXPECTF--
array(3) {
  [0]=>
  array(4) {
    ["CLASS_NAME"]=>
    string(4) "game"
    ["ATTR_NAME"]=>
    string(12) "athlete_code"
    ["KEY_SEQ"]=>
    string(1) "3"
    ["KEY_NAME"]=>
    string(41) "pk_game_host_year_event_code_athlete_code"
  }
  [1]=>
  array(4) {
    ["CLASS_NAME"]=>
    string(4) "game"
    ["ATTR_NAME"]=>
    string(10) "event_code"
    ["KEY_SEQ"]=>
    string(1) "2"
    ["KEY_NAME"]=>
    string(41) "pk_game_host_year_event_code_athlete_code"
  }
  [2]=>
  array(4) {
    ["CLASS_NAME"]=>
    string(4) "game"
    ["ATTR_NAME"]=>
    string(9) "host_year"
    ["KEY_SEQ"]=>
    string(1) "1"
    ["KEY_NAME"]=>
    string(41) "pk_game_host_year_event_code_athlete_code"
  }
}
array(2) {
  [0]=>
  array(9) {
    ["PKTABLE_NAME"]=>
    string(5) "event"
    ["PKCOLUMN_NAME"]=>
    string(4) "code"
    ["FKTABLE_NAME"]=>
    string(4) "game"
    ["FKCOLUMN_NAME"]=>
    string(10) "event_code"
    ["KEY_SEQ"]=>
    string(1) "1"
    ["UPDATE_RULE"]=>
    string(1) "1"
    ["DELETE_RULE"]=>
    string(1) "1"
    ["FK_NAME"]=>
    string(18) "fk_game_event_code"
    ["PK_NAME"]=>
    string(13) "pk_event_code"
  }
  [1]=>
  array(9) {
    ["PKTABLE_NAME"]=>
    string(7) "athlete"
    ["PKCOLUMN_NAME"]=>
    string(4) "code"
    ["FKTABLE_NAME"]=>
    string(4) "game"
    ["FKCOLUMN_NAME"]=>
    string(12) "athlete_code"
    ["KEY_SEQ"]=>
    string(1) "1"
    ["UPDATE_RULE"]=>
    string(1) "1"
    ["DELETE_RULE"]=>
    string(1) "1"
    ["FK_NAME"]=>
    string(20) "fk_game_athlete_code"
    ["PK_NAME"]=>
    string(15) "pk_athlete_code"
  }
}
