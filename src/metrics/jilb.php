<?

namespace rude;

require_once ('/srv/http/rude-metrics/src/etc/lexer.php');

define('RUDE_RULE_STATEMENT_IF',      1);
define('RUDE_RULE_STATEMENT_ELSE',    2);
define('RUDE_RULE_STATEMENT_ELSE_IF', 3);
define('RUDE_RULE_STATEMENT_TERNARY', 4);

define('RUDE_RULE_LOOP_WHILE',        50);
define('RUDE_RULE_LOOP_DO_WHILE',     51);

define('RUDE_RULE_OPERATOR_PLUS',     100);
define('RUDE_RULE_OPERATOR_MINUS',    101);
define('RUDE_RULE_OPERATOR_PIPE',     102);
define('RUDE_RULE_OPERATOR_PIPEPIPE', 103);

class jilb
{
	private $file_data;

	private $lexer;

	private $CL  = 0; // СL  - абсолютная сложность программы, характеризующаяся количеством операторов условия
	private $cl  = 0; // cl  - относительная сложность программы, характеризующаяся насыщенностью программы операторами условия, т.е. cl определяется как отношение CL к общему числу операторов
	private $CLI = 0; // CLI - характеристика максимального уровня вложенности операторов

	public function __construct($file_data)
	{
		$this->file_data = $file_data;

		$this->file_data = regex::erase_comments($this->file_data);
		$this->file_data = regex::erase_strings($this->file_data);

		$this->lexer = new lexer($this->file_data);
	}

	public function get_conditions()
	{
		return $this->lexer->conditions();
	}

	public function get_loops()
	{
		return $this->lexer->loops();
	}

	public function get_metrics()
	{
		$this->CL  = $this->lexer->total_conditions();
		$this->cl  = @round($this->lexer->total_conditions() / ($this->lexer->total_loops() + $this->lexer->total_conditions()), 2);
		$this->CLI = $this->lexer->total_max_depth();


		$metrics = array();

		$metrics[] = array('CL',  $this->CL,  'абсолютная сложность программы, характеризующаяся количеством операторов условия');
		$metrics[] = array('cl',  $this->cl,  'относительная сложность программы, характеризующаяся насыщенностью программы операторами условия');
//		$metrics[] = array('CLI', $this->CLI, 'характеристика максимального уровня вложенности операторов');

		return $metrics;
	}

	public function get_source()
	{
		return $this->file_data;
	}

	public function get_dictionary()
	{
		return $this->lexer->dictionary();
	}
}