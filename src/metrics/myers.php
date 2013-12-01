<?

namespace rude;

require_once (getcwd() . DIRECTORY_SEPARATOR . 'src' . DIRECTORY_SEPARATOR . 'etc' . DIRECTORY_SEPARATOR . 'lexer.php');

class myers
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

        $goto_count = $this->count_operators_GOTO();


		$this->lexer = new lexer($this->file_data);
	}

	public function get_metrics()
	{


		$this->ev  = $this->lexer->count_conditions()+$this->lexer->count_loops();
		$this->p  = $this->unique_operands();


      //  $label_list = regex::get_label_list()


		$metrics = array();



		return $metrics;
	}

    public function  get_positions($list_of_name)
    {

        $list_of_name = array_unique($list_of_name);
        $list = array();
        foreach($list_of_name as $name)
        {
            $name_positions_list = $this->find_all_positions($this->file_data,$name);
            foreach($name_positions_list as $position)
            {
                $array = array();

                $array['name'] = $name;
                $array['position'] = $position;

                $list[] = $array;
            }
        }
        return $list;
    }

    public function find_all_positions(&$string, &$find)
    {
        $offset = 0;

        $match = array();

        for ($count = 0; (($pos = strpos($string, $find, $offset)) !== false); $count++)
        {
            $match[] = $pos;
            $offset = $pos + strlen($find);
        }

        return $match;
    }

    public function get_label_and_goto_lists()
   {
        $goto_list = regex::get_goto_whith_label($this->file_data);
        $goto_list = $this->get_positions($goto_list); //получаем в goto_list именнованный массив, [name] = goto label, [position] = pos

        $label_list = array();

        foreach ($goto_list as &$goto)
        {
            $words = regex::get_words_from_string($goto['name']);



            if (!isset($words[1])) { continue; }
            $goto['name'] = $words[1];

            $labels = regex::get_label_list($this->file_data, $words[1]);

            $item = $this->get_positions($labels)[0];
            $item['name'] = trim($item['name'],":");

            $label_list[] = array('name' => $item['name'], 'position' => $item['position']);
        }

        return array('goto_list' => $goto_list, 'label_list' => $label_list);
   }

	public function count_operators_GOTO()
	{

        $array = $this->get_label_and_goto_lists();

        $count = 0;
        foreach ($array['goto_list'] as $goto)
        {

            foreach($array['label_list'] as $label)
            {
                if($goto['name'] == $label['name'])
                {
                    if($goto['position'] < $label['position'])
                    {
                        $count += 2;
                    }
                    else
                    {
                        $count++;
                    }
                }
            }

        }


        ?><pre><? print_r($count) ?></pre><?
        ?><pre><? print_r($array['goto_list']) ?></pre><?
        ?><pre><? print_r($array['label_list']) ?></pre><?


        return $count;

	}

    public function parse_between($begin, $end)
    {
         

    }


    public  function  count_predicate()
    {

    }



}