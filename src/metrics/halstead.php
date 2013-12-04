<?

namespace rude;

require_once (getcwd() . DIRECTORY_SEPARATOR . 'src' . DIRECTORY_SEPARATOR . 'etc' . DIRECTORY_SEPARATOR . 'lexer.php');

class halstead
{
	private $file_data;
	private $file_count;

	private $lexer;

	private $n1;  // число уникальных операторов программы, включая символы-разделители, имена процедур и знаки операций (словарь операторов)
	private $n2;  // число уникальных операндов программы (словарь операндов)
	private $N1;  // общее число операторов в программе
	private $N2;  // общее число операндов в программе
	private $n1_; // теоретическое число уникальных операторов
	private $n2_; // теоретическое число уникальных операндов
	private $n;   // `n  = n1+n2`						— словарь программы,
	private $N;   // `N  = N1+N2`						— длина программы
	private $n_;  // `n_ = n1_+n2_`						— теоретический словарь программы
	private $N_;  // `N_ = n1*log2(n1) + n2*log2(n2)`	— теоретическая длина программы (для стилистически корректных программ отклонение N от N' не превышает 10%)
	private $V;   // `V  = N*log2n`						— объем программы
	private $V_;  // `V_ = N_*log2n_`					— теоретический объем программы, где n' — теоретический словарь программы
	private $L;   // `L  = V_/V`						— уровень качества программирования, для идеальной программы L = 1
	private $L_;  // `L_ = (2 n2)/ (n1*N2)`				— уровень качества программирования, основанный лишь на параметрах реальной программы без учета теоретических параметров
	private $EC;  // `EC = V/2L_`						— сложность понимания программы
	private $D;   // `D  = 1/L_`						— трудоемкость кодирования программы
	private $y_;  // `y_ = V/2D`						— уровень языка выражения
	private $I;   // `I  = V/D`							— информационное содержание программы, данная характеристика позволяет определить умственные затраты на создание программы
	private $E;   // `E  = N_ * log2(n/L)`				— оценка необходимых интеллектуальных усилий при разработке программы, характеризующая число требуемых элементарных решений при написании программы

	public function __construct($file_data, $file_count)
	{
		$this->file_data = $file_data;
		$this->file_count = $file_count;

		$this->file_data = regex::erase_comments($this->file_data);
		$this->file_data = regex::erase_strings($this->file_data);

//		regex::get_goto_whith_label($this->file_data);
//		regex::get_label_list($this->file_data,'label1');

		$this->lexer = new lexer($this->file_data);

//		?><!--<pre>--><?// print_r($this->file_tokens) ?><!--</pre>--><?//
	}

	public function get_metrics()
	{
//		$this->CL  = $this->lexer->total_conditions();
//		$this->cl  = round($this->lexer->total_conditions() / ($this->lexer->total_loops() + $this->lexer->total_conditions()), 2);
//		$this->CLI = $this->lexer->total_max_depth();


		$this->n1  = $this->unique_operators();
		$this->n2  = $this->unique_operands();
		$this->N1  = $this->operators();
		$this->N2  = $this->operands();
		$this->n1_ = $this->n1;
		$this->n2_ = $this->n2;
		$this->n   = $this->n1 + $this->n2;													// `n  = n1+n2`
		$this->N   = $this->program_size();													// `N  = N1+N2`
		$this->n_  = $this->n1_ + $this->n2_;												// `n_ = n1_+n2_`
		$this->N_  = @floor($this->n1 * log($this->n1, 2) + $this->n2 * log($this->n2, 2));	// `N_ = n1*log2(n1) + n2*log2(n2)`
		$this->V   = @floor($this->N  * log($this->n, 2));									// `V  = N*log2n`
		$this->V_  = @floor($this->N_ * log($this->n_, 2));									// `V_ = N_*log2n_`
		$this->L   = @round(($this->V_ ) / ($this->V ), 2);					// `L  = V_/V`
		$this->L_  = @round((2 * $this->n2) / ($this->n1 * $this->N2), 2);					// `L_ = (2 n2)/ (n1*N2)`
		$this->EC  = @floor($this->V / (2 * $this->L_ ));				// `EC = V/2L_`
		$this->D   = @round(1 / ($this->L_ ), 2);						// `D  = 1/L_`
		$this->y_  = @round($this->V / (2 * $this->D ), 2);				// `y_ = V/2D`
		$this->I   = @round($this->V / $this->D);											// `I  = V/D`
		$this->E   = @round($this->N_ * log($this->n / $this->L, 2), 2);					// `E  = N_ * log2(n/L)`


		$metrics = array();


		$metrics[] = array('n<sub>1</sub>',   $this->n1,   'число уникальных операторов программы, включая знаки операций (словарь операторов)');
		$metrics[] = array('n<sub>2</sub>',   $this->n2,   'число уникальных операндов программы (словарь операндов)');
		$metrics[] = array('N<sub>1</sub>',   $this->N1,   'общее число операторов в программе');
		$metrics[] = array('N<sub>2</sub>',   $this->N2,   'общее число операндов в программе');
		$metrics[] = array('n<sub>1</sub>\'', $this->n1_,  'теоретическое число уникальных операторов');
		$metrics[] = array('n<sub>2</sub>\'', $this->n2_,  'теоретическое число уникальных операндов');
		$metrics[] = array('n',               $this->n,    'словарь программы');
		$metrics[] = array('N',               $this->N,    'длина программы');
		$metrics[] = array('n\'',             $this->n_,   'теоретический словарь программы');
		$metrics[] = array('N\'',             $this->N_,   'теоретическая длина программы (для стилистически корректных программ отклонение N от N\' не превышает 10%)');
		$metrics[] = array('V',               $this->V,    'объем программы');
		$metrics[] = array('V\'',             $this->V_,   'теоретический объем программы');
		$metrics[] = array('L',               $this->L,    'уровень качества программирования, для идеальной программы L = 1');
		$metrics[] = array('L\'',             $this->L_,   'уровень качества программирования, основанный лишь на параметрах реальной программы без учета теоретических параметров');
		$metrics[] = array('E<sub>c</sub>',   $this->EC,   'сложность понимания программы');
		$metrics[] = array('D',               $this->D,    'трудоемкость кодирования программы');
		$metrics[] = array('y\'',             $this->y_,   'уровень языка выражения');
		$metrics[] = array('I',               $this->I,    'информационное содержание программы, данная характеристика позволяет определить умственные затраты на создание программы');
		$metrics[] = array('E',               $this->E,    'оценка необходимых интеллектуальных усилий при разработке программы, характеризующая число элементарных решений при написании программы');
//
		return $metrics;
	}

	public function program_size()
	{
		return count($this->lexer->tokens());
	}

	public function unique_operators()
	{
		$operators = $this->lexer->count_operators();

//		$counter = 0;
//
//
//		foreach ($operators as $operator)
//		{
//
//			if (isset($operator[1]) && intval($operator[1] != 0))
//			{
//				$counter++;
//			}
//		}

		return $operators;
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

        $operands = array_unique($this->lexer->get_variables());
		//$operands = array_unique($operands);
		?><pre><?print_r($operands)?></pre><?

//		sort($operands);

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