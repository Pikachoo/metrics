<?

namespace rude;

require_once (getcwd() . DIRECTORY_SEPARATOR . 'src' . DIRECTORY_SEPARATOR . 'etc' . DIRECTORY_SEPARATOR . 'lexer.php');

class halstead
{
	private $file_data;
	private $file_count;

	private $lexer;

	private $ev;  // число уникальных операторов программы, включая символы-разделители, имена процедур и знаки операций (словарь операторов)
	private $p;

	public function __construct($file_data, $file_count)
	{
		$this->file_data = $file_data;
		$this->file_count = $file_count;

		$this->file_data = regex::erase_comments($this->file_data);
		$this->file_data = regex::erase_strings($this->file_data);

		$this->lexer = new lexer($this->file_data);

//		?><!--<pre>--><?// print_r($this->file_tokens) ?><!--</pre>--><?//
	}

	public function get_metrics()
	{
//		$this->CL  = $this->lexer->total_conditions();
//		$this->cl  = round($this->lexer->total_conditions() / ($this->lexer->total_loops() + $this->lexer->total_conditions()), 2);
//		$this->CLI = $this->lexer->total_max_depth();

		$this->ev  = $this->lexer->count_conditions()+$this->lexer->count_loops();
		$this->p  = $this->unique_operands();


		$metrics = array();



		return $metrics;
	}

	public function count_operators_GOTO()
	{
	    $goto_label_list = regex::get_goto_label($this->file_data);
	    foreach($goto_label_list as $label)
	    {
	        $label = trim($label);

	    }




	}

	public  function count_return()
	{

	}

	public function program_size()
	{
		return count($this->lexer->tokens());
	}

	public function unique_operators()
	{
		$operators = $this->lexer->operators();

//		?><!--<pre>--><?// print_r($operators) ?><!--</pre>--><?//

		$counter = 0;
		foreach ($operators as $operator)
		{
			if (isset($operator[1]) && intval($operator[1] != 0))
			{
				$counter++;
			}
		}

		return $counter;
	}

	public function unique_operands()
	{
		$operands = array();

		foreach ($this->lexer->operands() as $operand)
		{
			preg_match_all('/[a-zA-Z0-9]/xsm', $operand, $matches);

			if ($matches[0])
			{
				$operands[] = $operand;
			}
		}

		$operands = array_unique($operands);

		sort($operands);

		return count($operands);
	}

	public function operators()
	{
		return $this->lexer->count_operators();
	}

	public function operands()
	{
		$operands = array();

		foreach ($this->lexer->operands() as $operand)
		{
			preg_match_all('/[a-zA-Z0-9]/xsm', $operand, $matches);

			if ($matches[0])
			{
				$operands[] = $operand;
			}
		}

		sort($operands);

//		?><!--<pre>--><?// print_r($operands) ?><!--</pre>--><?//

return count($operands);
}
}