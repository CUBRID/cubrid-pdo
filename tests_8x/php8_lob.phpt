--TEST--
PDO CUBRID PHP8: several LOB parameters, NULL LOB, LOB stream as zval
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
$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
// pdo_test.inc stringifies fetches; since PHP 8 that also turns LOB streams into strings
$db->setAttribute(PDO::ATTR_STRINGIFY_FETCHES, false);
$db->exec("CREATE TABLE cubrid_test (id INT, b BLOB, b2 BLOB, c CLOB)");

$b = fopen('php://memory', 'w+');
fwrite($b, str_repeat("\x01\x02", 5000));
rewind($b);
$c = fopen('php://memory', 'w+');
fwrite($c, str_repeat("blob2", 3000));
rewind($c);

// PDO::PARAM_LOB binds a BLOB; CLOB columns are filled by SQL here
$stmt = $db->prepare("INSERT INTO cubrid_test VALUES (?, ?, ?, CHAR_TO_CLOB(REPEAT('clob', 3000)))");
$stmt->bindValue(1, 1, PDO::PARAM_INT);
$stmt->bindParam(2, $b, PDO::PARAM_LOB);
$stmt->bindParam(3, $c, PDO::PARAM_LOB);
var_dump($stmt->execute());
// the stream of the user is not closed by the driver
var_dump(is_resource($b), is_resource($c));

$stmt->bindValue(1, 2, PDO::PARAM_INT);
$stmt->bindValue(2, NULL, PDO::PARAM_LOB);
$stmt->bindValue(3, NULL, PDO::PARAM_LOB);
var_dump($stmt->execute());
$db->exec("UPDATE cubrid_test SET c = NULL WHERE id = 2");
$stmt = null;

$stmt = $db->query("SELECT id, b, b2, c FROM cubrid_test ORDER BY id");
$row = $stmt->fetch(PDO::FETCH_NUM);
var_dump(is_resource($row[1]), is_resource($row[2]), is_resource($row[3]));
var_dump(stream_get_contents($row[1]) === str_repeat("\x01\x02", 5000));
var_dump(stream_get_contents($row[2]) === str_repeat("blob2", 3000));
var_dump(stream_get_contents($row[3]) === str_repeat("clob", 3000));
$row = $stmt->fetch(PDO::FETCH_NUM);
var_dump($row[1], $row[2], $row[3]);

// PDO::PARAM_STR reads the lob into a string
$stmt = $db->prepare("SELECT c FROM cubrid_test WHERE id = 1");
$stmt->execute();
$stmt->bindColumn(1, $clob, PDO::PARAM_STR);
$stmt->fetch(PDO::FETCH_BOUND);
var_dump(strlen($clob));

// the lob stream keeps its statement alive
$stmt = $db->query("SELECT b FROM cubrid_test WHERE id = 1");
$meta = $stmt->getColumnMeta(0);
var_dump($meta['type'], $meta['pdo_type'] === PDO::PARAM_LOB);
$lob = $stmt->fetchColumn();
$stmt = null;
var_dump(strlen(stream_get_contents($lob)));
fclose($lob);

$db->exec("DROP TABLE cubrid_test");
print "done!\n";
?>
--EXPECTF--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
NULL
NULL
NULL
int(12000)
string(4) "blob"
bool(true)
int(10000)
done!
