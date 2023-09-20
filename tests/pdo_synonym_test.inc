<?php
# PDO test framework utilities

class PDOSynonymTest {
	// create an instance of the PDO driver, based on
	// the current environment
	static function factory($classname = 'PDO', $drop_test_tables = true) {
		$dsn = 'cubrid:host=localhost;port=33000;dbname=demodb';
		$user = 'dba';
		$pass = '';
		$attr = NULL;
                
		if (is_string($attr) && strlen($attr)) {
			$attr = unserialize($attr);
		} else {
			$attr = null;
		}

		if ($user === false) $user = NULL;
		if ($pass === false) $pass = NULL;

		$db = new $classname($dsn, $user, $pass, $attr);

		if (!$db) {
			die("Could not create PDO object (DSN=$dsn, user=$user)\n");
		}

		// clean up any crufty test tables we might have left behind
		// on a previous run
		static $test_tables = array(
			'cubrid_test',
			'cubrid_test2',
			'classtypes'
			);
		if ($drop_test_tables) {
			foreach ($test_tables as $table) {
				$db->exec("DROP TABLE $table");
			}
		}
		
		$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
		$db->setAttribute(PDO::ATTR_CASE, PDO::CASE_LOWER);
		$db->setAttribute(PDO::ATTR_STRINGIFY_FETCHES, true);
		
		# test code for pdo_042.phpt 
		$db->exec('drop synonym if exists dba.s1');
        $db->exec('drop table if EXISTS u1.t1');
        $db->exec('DROP USER u1');
		
		$db->exec('CREATE USER u1');
		$db->exec('CREATE TABLE u1.t1(col1 int, col2 varchar(10), col3 double)');
		$db->exec('Grant SELECT ON u1.t1 TO dba;');
		$db->exec('CREATE synonym dba.s1 for u1.t1');
		return $db;
	}

	static function skip() {
		try {
			$db = PDOSynonymTest::factory();
		} catch (PDOException $e) {
			die("skip " . $e->getMessage());
		}
	}

	static function user_factory($classname = 'PDO', $drop_test_tables = true) {
		$dsn = 'cubrid:host=test-db-server;port=33000;dbname=demodb';
		$user = 'u1';
		$pass = '';
		$attr = NULL;
                
		if (is_string($attr) && strlen($attr)) {
			$attr = unserialize($attr);
		} else {
			$attr = null;
		}

		if ($user === false) $user = NULL;
		if ($pass === false) $pass = NULL;

		$db = new $classname($dsn, $user, $pass, $attr);

		if (!$db) {
			die("Could not create PDO object (DSN=$dsn, user=$user)\n");
		}

		$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
		$db->setAttribute(PDO::ATTR_CASE, PDO::CASE_LOWER);
		$db->setAttribute(PDO::ATTR_STRINGIFY_FETCHES, true);
		
		return $db;
	}
}
?>