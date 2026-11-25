--TEST--
PDO CUBRID PHP8: Pdo\Cubrid driver specific subclass (PHP 8.4+)
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
if (PHP_VERSION_ID < 80400) die('skip driver specific PDO subclasses need PHP 8.4');
require_once 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php
$dsn = 'cubrid:host=localhost;port=33000;dbname=demodb';

// PDO::connect() returns the driver specific subclass
$db = PDO::connect($dsn, 'dba', '');
var_dump(get_class($db), $db instanceof PDO);

$db = new Pdo\Cubrid($dsn, 'dba', '');
var_dump(get_class($db));

// driver constants without the CUBRID_ prefix, PDO constants are inherited
var_dump(Pdo\Cubrid::SCH_PRIMARY_KEY === PDO::CUBRID_SCH_PRIMARY_KEY);
var_dump(Pdo\Cubrid::ATTR_ISOLATION_LEVEL === PDO::CUBRID_ATTR_ISOLATION_LEVEL);
var_dump(Pdo\Cubrid::TRAN_SERIALIZABLE === PDO::TRAN_SERIALIZABLE);
var_dump(Pdo\Cubrid::PARAM_INT === PDO::PARAM_INT);

// schema() is Pdo\Cubrid's cubrid_schema(), without the PHP 8.5 deprecation
$pk = $db->schema(Pdo\Cubrid::SCH_PRIMARY_KEY, 'game');
var_dump(count($pk), $pk[0]['KEY_NAME']);
var_dump(count($db->schema(Pdo\Cubrid::SCH_TABLE, 'game')));
var_dump($db->getAttribute(Pdo\Cubrid::ATTR_ISOLATION_LEVEL) > 0);

try {
    $db->schema('x');
} catch (TypeError $e) {
    echo get_class($e), ": ", $e->getMessage(), "\n";
}

// a subclass of Pdo\Cubrid
class MyCubrid extends Pdo\Cubrid {}
$db = new MyCubrid($dsn, 'dba', '');
var_dump(get_class($db), count($db->schema(Pdo\Cubrid::SCH_PRIMARY_KEY, 'game')));

// Pdo\Cubrid accepts cubrid DSNs only
try {
    new Pdo\Cubrid('sqlite::memory:');
} catch (PDOException $e) {
    echo get_class($e), ": ", $e->getMessage(), "\n";
}
print "done!\n";
?>
--EXPECTF--
string(10) "Pdo\Cubrid"
bool(true)
string(10) "Pdo\Cubrid"
bool(true)
bool(true)
bool(true)
bool(true)
int(3)
string(%d) "%s"
int(1)
bool(true)
TypeError: Pdo\Cubrid::schema(): Argument #1 ($schema_type) must be of type int, string given
string(8) "MyCubrid"
int(3)
PDOException: %s
done!
