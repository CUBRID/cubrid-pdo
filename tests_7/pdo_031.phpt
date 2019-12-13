--TEST--
PDO Common: PDOStatement SPL iterator
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
if (!extension_loaded('SPL')) die('skip SPL not available');
require_once 'pdo_test.inc';
if (!class_exists('RecursiveArrayIterator', false)) die('skip Class RecursiveArrayIterator missing');
if (!class_exists('RecursiveTreeIterator', false) && !file_exists(getenv('REDIR_TEST_DIR').'../../spl/examples/recursivetreeiterator.inc')) die('skip Class RecursiveTreeIterator missing');
PDOTest::skip();
?>
--FILE--
<?php
require_once 'pdo_test.inc';
if (!class_exists('RecursiveTreeIterator', false)) require_once(getenv('REDIR_TEST_DIR').'../../spl/examples/recursivetreeiterator.inc'); 

$data = array(
    array('10', 'Abc', 'zxy'),
    array('20', 'Def', 'wvu'),
    array('30', 'Ghi', 'tsr'),
);

$db = PDOTest::factory();

$db->exec('CREATE TABLE cubrid_test(id INT NOT NULL PRIMARY KEY, val VARCHAR(10), val2 VARCHAR(16))');

$stmt = $db->prepare("INSERT INTO cubrid_test VALUES(?, ?, ?)");
foreach ($data as $row) {
    $stmt->execute($row);
}

unset($stmt);

echo "===QUERY===\n";

$stmt = $db->query('SELECT * FROM cubrid_test');

foreach(new RecursiveTreeIterator(new RecursiveArrayIterator($stmt->fetchAll(PDO::FETCH_ASSOC)), RecursiveTreeIterator::BYPASS_KEY) as $c=>$v)
{
	echo "$v [$c]\n";
}

echo "===DONE===\n";
exit(0);
?>
--EXPECT--
===QUERY===
|-Array [0]
| |-10 [id]
| |-Abc [val]
| \-zxy [val2]
|-Array [1]
| |-20 [id]
| |-Def [val]
| \-wvu [val2]
\-Array [2]
  |-30 [id]
  |-Ghi [val]
  \-tsr [val2]
===DONE===
