--TEST--
PDO Common: PDO::quote()
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

$unquoted = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~';

if (!($quoted = $db->quote($unquoted))) {
    $quoted = $unquoted;
}

$len = strlen($unquoted);

@$db->exec("DROP TABLE cubrid_test");

$db->query("CREATE TABLE cubrid_test (t char($len))");
$db->query("INSERT INTO cubrid_test (t) VALUES('$quoted')");

$stmt = $db->prepare('SELECT * FROM cubrid_test');
$stmt->execute();

print_r($stmt->fetchAll(PDO::FETCH_ASSOC));

$db->exec("DROP TABLE cubrid_test");

?>
--EXPECT--
Array
(
    [0] => Array
        (
            [t] =>  !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
        )

)
