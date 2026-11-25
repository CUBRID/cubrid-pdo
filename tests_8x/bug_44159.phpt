--TEST--
Bug #44159 (Crash: $pdo->setAttribute(PDO::STATEMENT_ATTR_CLASS, NULL))
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('pdo')) die('skip');
try {
	$pdo = new PDO("sqlite:".__DIR__."/foo.db");
} catch (Exception $e) {
	die("skip PDP_SQLITE not available");
}
?>
--FILE--
<?php
$pdo = new PDO("sqlite:".__DIR__."/foo.db");

/* PHP 8 throws TypeError instead of a warning (php-src ext/pdo/tests/bug_44159.phpt).
 * PDO::NULL_TO_STRING was dropped by upstream: it is 2, the same as PDO::ATTR_TIMEOUT. */
$attrs = array(PDO::ATTR_STATEMENT_CLASS, PDO::ATTR_STRINGIFY_FETCHES);

foreach ($attrs as $attr) {
    foreach (array(NULL, 1, 'nonsense') as $value) {
        try {
            var_dump($pdo->setAttribute($attr, $value));
        } catch (\Error $e) {
            /* PHP 8.5 prefixes the message with the argument */
            $msg = preg_replace('/^PDO::setAttribute\(\): Argument #2 \(\$value\) /', '', $e->getMessage());
            echo get_class($e), ': ', $msg, \PHP_EOL;
        }
    }
}

$pdo = null;
@unlink(__DIR__."/foo.db");

?>
--EXPECT--
TypeError: PDO::ATTR_STATEMENT_CLASS value must be of type array, null given
TypeError: PDO::ATTR_STATEMENT_CLASS value must be of type array, int given
TypeError: PDO::ATTR_STATEMENT_CLASS value must be of type array, string given
TypeError: Attribute value must be of type bool for selected attribute, null given
bool(true)
TypeError: Attribute value must be of type bool for selected attribute, string given
