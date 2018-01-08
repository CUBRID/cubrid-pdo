--TEST--
Trying instantiate a PDORow object manually
--FILE--
<?php

new PDORow;

?>
--EXPECTF--
Fatal error: Uncaught PDOException: You may not create a PDORow manually in %s
Stack trace:
#0 {main}
  thrown in %s on line %d
