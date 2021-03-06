<?

namespace rude;

require_once ('defines.php');
class lexer
{
	private $source;

	private $tokens;


	private $conditions = array();
	private $operators  = array();
	private $loops      = array();

	private $dictionary            = array();
	private $dictionary_conditions = array('if', 'else');
	private $dictionary_operators  = array('++', '--', '*=', '+=', '/=', '=', '%=', '+', '-', '/', '*', '&', '&&', '%', '|', '||', '!', '==', '!=', '>', '>=', '<', '<=', '?', 'instanceof', '~', '<<', '>>', '>>>', '^');
	private $dictionary_loops      = array('do', 'while', 'for');


	private $total_conditions = 0;
	private $total_operators  = 0;
	private $total_loops      = 0;

	private $total_max_depth  = 0;
	
	public function __construct($source)
	{
		$this->source = $source;
		$this->source = explode(PHP_EOL, $this->source);        // explode source file by lines
		$this->source = array_filter($this->source);            // erase all empty values
		$this->source = array_diff($this->source, array("\t")); // erase standalone tab chars
		$this->source = implode(PHP_EOL, $this->source);        // implode source file again

		$this->tokens = $this->tokenize($this->source);

	//	$this->count_conditions();
//		$this->count_operators();
//		$this->count_loops();

//		$this->count_max_depth();

		$this->dictionary = array_merge($this->dictionary_operators, $this->dictionary_conditions, $this->dictionary_loops);
	}

	public static function tokenize(&$string)
	{
		return token_get_all('<? ' . $string . ' ?>');
	}

	public function count_continue_and_break()
    {
        $total_continue_and_break = 0;

		$continue_and_break[] = array('continue',      $this->count_tokens('continue'));
   		$continue_and_break[] = array('break',    $this->count_tokens('break'));

		$case_list[] = array('case', $this->count_tokens('case',  RUDE_RULE_STATEMENT_CASE));


        foreach ($continue_and_break as $loop)
		{
			if (isset($loop[1]) && intval($loop[1]))
			{
				$total_continue_and_break+= intval($loop[1]);
			}
		}
		foreach ($case_list as $case)
		{
		    if (isset($case[1]) && intval($case[1]))
			{
				$total_continue_and_break-= intval($case[1]);
			}

		}

		$continue_and_break[] = array('Всего', $total_continue_and_break);



		return $total_continue_and_break;

    }

	public function count_loops()
	{
		$this->total_loops = 0;

		$this->loops[] = array('for',      $this->count_tokens('for'));
   		$this->loops[] = array('while',    $this->count_tokens('while',    RUDE_RULE_LOOP_WHILE));
		$this->loops[] = array('do while', $this->count_tokens('do while', RUDE_RULE_LOOP_DO_WHILE));


        foreach ($this->loops as $loop)
		{
			if (isset($loop[1]) && intval($loop[1]))
			{
				$this->total_loops += intval($loop[1]);
			}
		}

		$this->loops[] = array('Всего', $this->total_loops);



		return $this->total_loops;
	}
	
	public function count_conditions()
	{
		$this->total_conditions = 0;
		$this->conditions = array();



		$this->conditions[] = array('if',      $this->count_tokens('if',      RUDE_RULE_STATEMENT_IF));
		$this->conditions[] = array('else',    $this->count_tokens('else',    RUDE_RULE_STATEMENT_ELSE));
		$this->conditions[] = array('else if', $this->count_tokens('else if', RUDE_RULE_STATEMENT_ELSE_IF));
		$this->conditions[] = array('?:',      $this->count_tokens('?:',      RUDE_RULE_STATEMENT_TERNARY));
		$this->conditions[] = array('case',    $this->count_tokens('case',    RUDE_RULE_STATEMENT_CASE));

		foreach ($this->conditions as $condition)
		{
			if (isset($condition[1]) && intval($condition[1]))
			{
				$this->total_conditions += intval($condition[1]);

			}
		}

		$this->conditions[] = array('Всего', $this->total_conditions);

		return $this->total_conditions;
	}

	public function count_returns()
	{
		$this->total_conditions = 0;
		$this->conditions = array();

		$this->conditions[] = array('return',      $this->count_tokens('return',      RUDE_RULE_STATEMENT_RETURN));

		foreach ($this->conditions as $condition)
		{
			if (isset($condition[1]) && intval($condition[1]))
			{
				$this->total_conditions += intval($condition[1]);
			}
		}

		$this->conditions[] = array('Всего', $this->total_conditions);

		return $this->total_conditions;
	}

	public function source_count($item, $odd_list = array())
	{
		$count = substr_count($this->source, $item);

		foreach ($odd_list as $odd)
		{
			$count -= substr_count($this->source, $odd);
		}

		return $count;
	}

	public function count_operators()
	{
		$this->total_operators = 0;

		$this->operators = array();

		$this->operators[] = array('+',   $this->source_count('+',  array('++', '+=')));

		$this->operators[] = array('-',   $this->source_count('-',  array('--', '-=')));
		$this->operators[] = array('/',   $this->source_count('/',  array('/=')));
		$this->operators[] = array('%',   $this->source_count('%',  array('%=')));
		$this->operators[] = array('&',   $this->source_count('&',  array('&&')));
		$this->operators[] = array('&&',  $this->source_count('&&'));
		$this->operators[] = array('~',   $this->source_count('~'));
		$this->operators[] = array('?',   $this->source_count('?'));
		$this->operators[] = array('*',   $this->source_count('*',  array('*=')));
		$this->operators[] = array('!',   $this->source_count('!',  array('!=', '!==')));
		$this->operators[] = array('=',   $this->source_count('=',  array('===', '==', '-=', '+=', '/=', '%=', '!=', '>=', '<=')));
		$this->operators[] = array('|',   $this->source_count('|',  array('||')));
		$this->operators[] = array('||',  $this->source_count('||'));

		$this->operators[] = array('<',   $this->source_count('<',  array('<<', '<=', '<<<')));
		$this->operators[] = array('>',   $this->source_count('>',  array('>>', '>=', '>>>')));
		$this->operators[] = array('<<',  $this->source_count('<<', array('<<<')));
		$this->operators[] = array('>>',  $this->source_count('>>', array('>>>')));
		$this->operators[] = array('>>>', $this->source_count('>>>'));

		$this->operators[] = array('>=',  $this->source_count('>='));
		$this->operators[] = array('<=',  $this->source_count('<='));
		$this->operators[] = array('==',  $this->source_count('==', array('===')));
		$this->operators[] = array('===', $this->source_count('==='));

		$this->operators[] = array('*=',  $this->source_count('*='));
		$this->operators[] = array('-=',  $this->source_count('-='));
		$this->operators[] = array('+=',  $this->source_count('+='));
		$this->operators[] = array('/=',  $this->source_count('/='));
		$this->operators[] = array('%=',  $this->source_count('%='));
		$this->operators[] = array('!=',  $this->source_count('!=', array('!==')));

		$this->operators[] = array('instanceof', $this->source_count('instanceof'));


		sort($this->operators);


		foreach ($this->operators() as $operator)
		{
			if (isset($operator[1]))
			{
				$this->total_operators += intval($operator[1]);
			}
		}
		return $this->total_operators;
	}

    public function get_variables()
{
    $reserved_words = array
                      ('abstract',  'bool', 'assert',  'boolean',   'break',    'byte',
                       'case',  'catch', 'char',    'class',    'const',   'continue',
                       'default','double', 'do',   'else', 'enum',     'extends',
                       'false', 'final', 'finally', 'float', 'for',   'goto',  'if',
                       'implements', 'import',  'instanceof', 'int',      'interface', 'long',
                       'native',     'new', 'null', 'package',  'private',   'protected',
                       'public',     'return',            'short',  'static',   'strictfp',  'super',
                       'switch',     'synchronized	this', 'throw',      'throws',   'transient',
                       'true',       'try',               'void',       'volatile', 'while',
                       '(Boolean)',  '(int)',             'Object');

    $operand_list = $this->get_operands();


    $variables = array();

    foreach ($operand_list as $operand)
    {
        if (in_array($operand, $reserved_words))
        {
            continue;
        }

        if (is_numeric($operand))
        {
            continue;
        }


        $variables[] = $operand;
    }
	return $variables;
}

    public function get_operands()
	{
		$operands = array();

		foreach ($this->operands() as $operand)
		{
			preg_match_all('/[a-zA-Z0-9]/xsm', $operand, $matches);

			if ($matches[0])
			{
				$operands[] = $operand;
			}
		}

		sort($operands);

		return $operands;
	}

	private function count_tokens($token_value, $RUDE_RULE = false)
	{
		if (!$RUDE_RULE)
		{
			return lexer::count($this->tokens, $token_value);
		}


		switch ($RUDE_RULE)
		{
			/* ============================================ *\
			 * `while` = count(`while`) - count(`do while`) *
			\* ============================================ */
			case RUDE_RULE_LOOP_WHILE:
				return (lexer::count($this->tokens, 'while') - $this->count_tokens('do while', RUDE_RULE_LOOP_DO_WHILE));
				break;

			/* ======================== *\
			 * `do while` = count(`do`) *
			\* ======================== */
			case RUDE_RULE_LOOP_DO_WHILE:
				return lexer::count($this->tokens, 'do');
				break;

			/* ===================================== *\
			 * `if` = count(`if`) - count(`else if`) *
			\* ===================================== */
			case RUDE_RULE_STATEMENT_IF:
//				return lexer::count($this->tokens, 'if') - $this->count_tokens('else if', RUDE_RULE_STATEMENT_ELSE_IF);
                return lexer::count($this->tokens, 'if');
				break;

			/* ========================================= *\
			 * `else` = count(`else`) - count(`else if`) *
			\* ========================================= */
			case RUDE_RULE_STATEMENT_ELSE:
//				return lexer::count($this->tokens, 'else') - $this->count_tokens('else if', RUDE_RULE_STATEMENT_ELSE_IF);
                return lexer::count($this->tokens, 'else') ;
				break;

			/* ============================ *\
			 * `else if` = count(`else if`) *
			\* ============================ */
			case RUDE_RULE_STATEMENT_ELSE_IF:
				return substr_count($this->source, 'else if');
				break;

			/* ============================== *\
			 * `ternary if else` = count(`?`) *
			\* ============================== */
			case RUDE_RULE_STATEMENT_TERNARY:
				return lexer::count($this->tokens, '?');
				break;
			/* ============================== *\
			 * `case() = count(`case`) *
			\* ============================== */
			case RUDE_RULE_STATEMENT_CASE:
				return lexer::count($this->tokens, 'case');
				break;
			/* ============================== *\
			 * `return = count(`return`) *
			\* ============================== */

			case RUDE_RULE_STATEMENT_RETURN:
				return lexer::count($this->tokens, 'return');
				break;

			/* ============================== *\
			 * `+` = count(`+`) - count(`++`) *
			\* ============================== */
			case RUDE_RULE_OPERATOR_PLUS:
				return substr_count($this->source, '++') - substr_count($this->source, '+');
				break;

			/* ============================== *\
			 * `-` = count(`-`) - count(`--`) *
			\* ============================== */
			case RUDE_RULE_OPERATOR_MINUS:
				return substr_count($this->source, '--') - substr_count($this->source, '-');
				break;

			/* ============================== *\
			 * `|` = count(`|`) - count(`||`) *
			\* ============================== */
			case RUDE_RULE_OPERATOR_PIPE:
				return substr_count($this->source, '||') - substr_count($this->source, '|');
				break;

			/* ============================== *\
			 * `||` = count(`||`)             *
			\* ============================== */
			case RUDE_RULE_OPERATOR_PIPEPIPE:
				return substr_count($this->source, '||');
				break;

			default:
				break;
		}

		return 0;
	}
	
	public static function count($tokens, $token_value)
	{
		$index = 1;
	
		if (in_array($token_value, array('+', '-', '?', '&', '*', '%', '!', '=', '~', '<', '>', '>=')))
		{
			$index = 0;
		}
	
	
		$i = 0;
	
		foreach ($tokens as $token)
		{
		    if (isset($token[$index]))
		    {
                switch ($token[$index])
                {
                    case $token_value:
                        $i++;
                        break;

                    default:
                        break;
                }
			}
		}
	
		return $i;
	}

	public function conditions()
	{
		return $this->conditions;
	}

	public function operators()
	{
		return $this->operators;
	}

	public function operands()
	{
		$operands = array();

		foreach ($this->tokens as $operand)
		{
			if (isset($operand[1]) && !is_int($operand[1]) && !is_array($operand[1]))
			{
				$operands[] = $operand[1];
			}
		}

		return $operands;
	}

	public function loops()
	{
		return $this->loops;
	}

	public function total_conditions()
	{
		return $this->total_conditions;
	}

	public function total_operators()
	{
		return $this->total_operators;
	}

	public function total_loops()
	{
		return $this->total_loops;
	}

	public function total_max_depth()
	{
		return $this->total_max_depth;
	}

	public function tokens()
	{
		return $this->tokens;
	}

	public function dictionary()
	{
//		$dictionary = array();

//		?><!--<pre>--><?// print_r($this->dictionary) ?><!--</pre>--><?//

//		foreach ($this->dictionary as $key => $value)
//		{
//			$dictionary[$key] = array($value, $this->count($this->tokens, $value));
//		}



		return $this->operators;
	}

	public function dictionary_size()
	{
		return count($this->dictionary);
	}

	public function dictionary_operators()
	{
		return $this->dictionary_operators;
	}

	public function dictionary_conditions()
	{
		return $this->dictionary_conditions;
	}

	public function dictionary_loops()
	{
		return $this->dictionary_loops;
	}
}