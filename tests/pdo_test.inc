<?php
# PDO test framework utilities

class PDOTest {
	// create an instance of the PDO driver, based on
	// the current environment
	static function factory($classname = 'PDO', $drop_test_tables = true) {
		$dsn = 'cubrid:host=test-db-server;port=33000;dbname=demodb';
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
		return $db;
	}

	static function skip() {
		try {
			$db = PDOTest::factory();
		} catch (PDOException $e) {
			die("skip " . $e->getMessage());
		}
	}

	static function test_factory($file) {
		$data = file_get_contents($file);
		$data = preg_replace('/^.*--REDIRECTTEST--/s', '', $data);
		$config = eval($data);
		foreach ($config['ENV'] as $k => $v) {
			putenv("$k=$v");
		}
		return self::factory();
	}
}
?>
