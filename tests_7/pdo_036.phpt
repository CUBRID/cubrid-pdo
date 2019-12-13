--TEST--
Testing PDORow and PDOStatement instances with Reflection
--FILE--
<?php
$instance = new reflectionclass('pdostatement');
$x = $instance->newInstance();
var_dump($x);
$instance = new reflectionclass('pdorow');
$x = $instance->newInstance();
var_dump($x);
?>
--EXPECTF--
object(PDOStatement)#%d (1) {
  ["queryString"]=>
  NULL
}

Fatal error: Uncaught PDOException: You may not create a PDORow manually in %s
Stack trace:
#0 %s ReflectionClass->newInstance()
#1 {main}
  thrown in %s on line %d
