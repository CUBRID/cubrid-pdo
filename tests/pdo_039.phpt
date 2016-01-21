--TEST--
PDO::attributes
--SKIPIF--
<?php
if (!extension_loaded("pdo")) die("skip");
require_once 'pdo_test.inc';
PDOTest::skip();
?>
--FILE--
<?php

require_once 'pdo_test.inc';
$db = PDOTest::factory();

printf("---- PDO Attributes ----\n");
printf("\nautocommit: %d\ntimeout: %d\nlock_timeout: %d\nisolation_level: %d\nmax_string_length: %d\nserver_version: %s\nclient_version: %s\n",
        $db->getAttribute(PDO::ATTR_AUTOCOMMIT), $db->getAttribute(PDO::ATTR_TIMEOUT),$db->getAttribute(PDO::CUBRID_ATTR_LOCK_TIMEOUT),
        $db->getAttribute(PDO::CUBRID_ATTR_ISOLATION_LEVEL), $db->getAttribute(PDO::CUBRID_ATTR_MAX_STRING_LENGTH),
        $db->getAttribute(PDO::ATTR_SERVER_VERSION), $db->getAttribute(PDO::ATTR_CLIENT_VERSION)
        );

$db->setAttribute(PDO::ATTR_TIMEOUT, 1);
$db->setAttribute(PDO::CUBRID_ATTR_LOCK_TIMEOUT, 4);
$db->setAttribute(PDO::ATTR_AUTOCOMMIT, 0);
$db->setAttribute(PDO::CUBRID_ATTR_ISOLATION_LEVEL, 2);

printf("\n---- PDO Attributes after set ----\n");
printf("\nautocommit: %d\ntimeout: %d\nlock_timeout: %d\nisolation_level: %d",
        $db->getAttribute(PDO::ATTR_AUTOCOMMIT), $db->getAttribute(PDO::ATTR_TIMEOUT),
        $db->getAttribute(PDO::CUBRID_ATTR_LOCK_TIMEOUT),$db->getAttribute(PDO::CUBRID_ATTR_ISOLATION_LEVEL));
?>
--EXPECTF--
---- PDO Attributes ----

autocommit: %d
timeout: -1
lock_timeout: -1
isolation_level: %d
max_string_length: %d
server_version: %s
client_version: %s

---- PDO Attributes after set ----

autocommit: 0
timeout: 1
lock_timeout: 4
isolation_level: 2
