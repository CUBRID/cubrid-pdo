--TEST--
Crash when calling a method of a class that inherits PDOStatement
--SKIPIF--
<?php
if (!extension_loaded('pdo')) die('skip');
?>
--FILE--
<?php

error_reporting(E_ERROR);
debug_backtrace(2);

class MyStatement extends PDOStatement
{
}

$obj = new MyStatement;
var_dump($obj->foo());

?>
--EXPECTF--
Fatal error: Uncaught Error: Call to undefined method MyStatement::foo() in %s
Stack trace:
#0 {main}
  thrown in %s on line %d 
