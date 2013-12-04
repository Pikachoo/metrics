<?

namespace rude;

require_once (getcwd() . DIRECTORY_SEPARATOR . 'src' . DIRECTORY_SEPARATOR . 'etc' . DIRECTORY_SEPARATOR . 'lexer.php');

class chapin
{
	private $file_data;

	private $lexer;

	private $P; // P — вводимые переменные для расчетов и для обеспечения вывода
	private $M; // M — модифицируемые, или создаваемые внутри программы переменные
	private $C; // C — переменные, участвующие в управлении работой программного модуля (управляющие переменные)
	private $T; // T — не используемые в программе («паразитные») переменные

	private $Q; // Q — информационная прочность отдельно взятого модуля по характеру использования переменных (Q = P + 2M + 3C + 0.5T)

	public function __construct($file_data)
	{
		$this->file_data = $file_data;

		$this->file_data = regex::erase_comments($this->file_data);
		$this->file_data = regex::erase_strings($this->file_data);

		$this->lexer = new lexer($this->file_data);

		$this->count_unused();
	}

	public function get_metrics()
	{
		$this->P =$this->count_variables();         // P — вводимые переменные для расчетов и для обеспечения вывода
		$this->M = $this->count_used();              // M — модифицируемые, или создаваемые внутри программы переменные
		$this->C = $this->lexer->count_conditions(); // C — переменные, участвующие в управлении работой программного модуля (управляющие переменные)
		$this->T = $this->count_unused();            // T — не используемые в программе («паразитные») переменные

		$this->Q = $this->P + (2 * $this->M) + (3 * $this->C) + (0.5 * $this->T); // Q = P + 2M + 3C + 0.5T


		$metrics = array();

		$metrics[] = array('P', $this->P, 'вводимые переменные для расчетов и для обеспечения вывода');
		$metrics[] = array('M', $this->M, 'модифицируемые, или создаваемые внутри программы переменные');
		$metrics[] = array('C', $this->C, 'переменные, участвующие в управлении работой программного модуля (управляющие переменные)');
		$metrics[] = array('T', $this->T, 'не используемые в программе («паразитные») переменные');
		$metrics[] = array('Q', $this->Q, 'информационная прочность отдельно взятого модуля по характеру использования переменных (Q = P + 2M + 3C + 0.5T)');

		return $metrics;
	}



	public function get_used()
	{
		$variables = array_count_values($this->lexer->get_variables());


		$used = array();

		foreach ($variables as $key => $value)
		{
			if ($value > 1)
			{
				$used[] = $key;
			}
		}

//		?><!--<pre>--><?// print_r($unused) ?><!--</pre>--><?//

		return $used;
	}

	public function count_used()
	{
		return count($this->get_used());
	}

	public function get_unused()
	{
		$variables = array_count_values($this->lexer->get_variables());


		$unused = array();

		foreach ($variables as $key => $value)
		{
			if ($value == 1)
			{
				$unused[] = $key;
			}
		}

		return $unused;
	}



	public function count_unused()
	{
		return count($this->get_unused());
	}



	public function count_variables()
	{

		return count( array_unique($this->lexer->get_variables()));
	}
}