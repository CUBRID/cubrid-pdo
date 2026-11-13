<?php
# PDO test framework utilities

class PDOTest {
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

		// PHP 8 uses PDO::ERRMODE_EXCEPTION by default: the tables may not exist
		$db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_SILENT);

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

	// Driver class to connect with: Pdo\Cubrid on PHP 8.4+ (driver specific
	// PDO subclass), PDO before 8.4.
	static function driver_class() {
		return class_exists('Pdo\\Cubrid', false) ? 'Pdo\\Cubrid' : 'PDO';
	}

	// Schema information with the API of the running PHP version:
	// Pdo\Cubrid::schema() on 8.4+, PDO::cubrid_schema() before 8.4
	// (deprecated on PHP 8.5). Connect with driver_class() to use schema().
	static function schema($db, $type, $table = null, $col = null) {
		if ($db instanceof Pdo\Cubrid) {
			return $db->schema($type, $table, $col);
		}
		return $db->cubrid_schema($type, $table, $col);
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

	static function user_factory($classname = 'PDO', $drop_test_tables = true) {
		$dsn = 'cubrid:host=localhost;port=33000;dbname=demodb';
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
