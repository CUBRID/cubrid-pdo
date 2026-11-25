--TEST--
PDO CUBRID PHP8: PDO::quote() escapes without enclosing quotes
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

var_dump($db->quote("it's"));
var_dump($db->quote(""));
$q = $db->quote("O'Reilly");
var_dump($db->query("SELECT '$q' FROM db_root")->fetchColumn());
print "done!\n";
?>
--EXPECTF--
string(5) "it''s"
string(0) ""
string(8) "O'Reilly"
done!
